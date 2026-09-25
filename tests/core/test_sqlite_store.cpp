#include <catch2/catch_test_macros.hpp>

#include "storage/SQLiteStore.h"

#include <filesystem>
#include <random>
#include <string>

namespace {

using garageplaymate::SQLiteStore;
using garageplaymate::StoredSession;

// Unique temporary directory removed at scope exit.
struct TempDir {
    std::filesystem::path path;

    TempDir() {
        std::random_device device;
        path = std::filesystem::temp_directory_path() /
               ("garageplaymate-test-" + std::to_string(device()) + std::to_string(device()));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code errorCode;
        std::filesystem::remove_all(path, errorCode);
    }

    std::filesystem::path dbPath() const { return path / "garageplaymate.db"; }
};

StoredSession makeSession(const std::string& songId, int64_t timestampMs) {
    StoredSession stored;
    stored.session.songId = songId;
    stored.timestampMs = timestampMs;

    auto drumsTake = std::make_shared<garageplaymate::TakeFile>();
    drumsTake->filename = "take-01.wav";
    auto bassTake = std::make_shared<garageplaymate::TakeFile>();
    bassTake->filename = "take-03.wav";
    stored.session.takeMap["drums"] = drumsTake;
    stored.session.takeMap["bass"] = bassTake;
    return stored;
}

}  // namespace

TEST_CASE("SQLiteStore creates schema on a fresh database and reopens idempotently", "[sqlite_store]") {
    TempDir tempDir;
    {
        SQLiteStore store(tempDir.dbPath());
        std::string error;
        REQUIRE(store.open(&error));
        CHECK(error.empty());
        store.setSetting("theme", "\"dark\"");
    }

    SQLiteStore reopened(tempDir.dbPath());
    REQUIRE(reopened.open());
    reopened.applySchema();
    CHECK(reopened.getSetting("theme").value() == "\"dark\"");
}

TEST_CASE("SQLiteStore reports failure for an unopenable path", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.path / "missing-dir" / "garageplaymate.db");
    std::string error;
    CHECK_FALSE(store.open(&error));
    CHECK_FALSE(error.empty());
    CHECK_FALSE(store.isOpen());
    CHECK_THROWS_AS(store.getSetting("x"), garageplaymate::StorageError);
}

TEST_CASE("SQLiteStore settings round-trip", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.dbPath());
    REQUIRE(store.open());

    CHECK_FALSE(store.getSetting("audio").has_value());
    store.setSetting("audio", R"({"bufferSize":512})");
    CHECK(store.getSetting("audio").value() == R"({"bufferSize":512})");
    store.setSetting("audio", R"({"bufferSize":256})");
    CHECK(store.getSetting("audio").value() == R"({"bufferSize":256})");
}

TEST_CASE("SQLiteStore setlist CRUD", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.dbPath());
    REQUIRE(store.open());

    const int64_t gigId = store.createSetlist("Friday gig");
    const int64_t practiceId = store.createSetlist("Practice");
    CHECK(gigId != practiceId);
    CHECK_THROWS_AS(store.createSetlist("Practice"), garageplaymate::StorageError);

    auto setlists = store.listSetlists();
    REQUIRE(setlists.size() == 2);
    CHECK(setlists[0].name == "Friday gig");
    CHECK(setlists[0].createdAtMs > 0);

    store.renameSetlist(gigId, "Saturday gig");
    setlists = store.listSetlists();
    CHECK(setlists[0].name == "Saturday gig");

    store.setSetlistSongs(gigId, {"song-b", "song-a", "song-c"});
    CHECK(store.getSetlistSongIds(gigId) == std::vector<std::string>{"song-b", "song-a", "song-c"});

    store.setSetlistSongs(gigId, {"song-c", "song-b", "song-c"});
    CHECK(store.getSetlistSongIds(gigId) == std::vector<std::string>{"song-c", "song-b"});

    store.deleteSetlist(gigId);
    setlists = store.listSetlists();
    REQUIRE(setlists.size() == 1);
    CHECK(setlists[0].id == practiceId);
    CHECK(store.getSetlistSongIds(gigId).empty());
}

TEST_CASE("SQLiteStore session insert and load", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.dbPath());
    REQUIRE(store.open());

    StoredSession stored = makeSession("song-a", 1000);
    stored.isReplay = true;
    garageplaymate::FailureEvent failure;
    failure.trackId = "bass";
    failure.onsetMs = 12000;
    failure.restoreMs = 30000;
    failure.eventData = garageplaymate::VolumeDropFailure{0.2f};
    stored.session.failureEvents.push_back(failure);

    const int64_t sessionId = store.insertSession(stored);
    const auto loaded = store.getSessionById(sessionId);
    REQUIRE(loaded.has_value());
    const StoredSession& session = loaded.value();
    CHECK(session.session.id == sessionId);
    CHECK(session.session.songId == "song-a");
    CHECK(session.timestampMs == 1000);
    CHECK(session.isReplay);
    REQUIRE(session.session.takeMap.size() == 2);
    CHECK(session.session.takeMap.at("drums")->filename == "take-01.wav");
    CHECK(session.session.takeMap.at("bass")->filename == "take-03.wav");
    REQUIRE(session.session.failureEvents.size() == 1);
    const garageplaymate::FailureEvent& loadedFailure = session.session.failureEvents[0];
    CHECK(loadedFailure.trackId == "bass");
    CHECK(loadedFailure.onsetMs == 12000);
    CHECK(loadedFailure.restoreMs == 30000);
    CHECK(std::get<garageplaymate::VolumeDropFailure>(loadedFailure.eventData).failureLevel == 0.2f);

    CHECK_FALSE(store.getSessionById(sessionId + 100).has_value());
}

TEST_CASE("SQLiteStore lists sessions newest first and prunes per song", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.dbPath());
    REQUIRE(store.open());

    for (int64_t i = 1; i <= 5; ++i) {
        store.insertSession(makeSession("song-a", i * 100));
    }
    const int64_t otherSongSession = store.insertSession(makeSession("song-b", 50));

    auto sessions = store.getSessionsForSong("song-a", 3);
    REQUIRE(sessions.size() == 3);
    CHECK(sessions[0].timestampMs == 500);
    CHECK(sessions[2].timestampMs == 300);

    store.deleteSessionsForSongExceptNewest("song-a", 2);
    sessions = store.getSessionsForSong("song-a", 100);
    REQUIRE(sessions.size() == 2);
    CHECK(sessions[0].timestampMs == 500);
    CHECK(sessions[1].timestampMs == 400);
    CHECK(sessions[1].session.takeMap.size() == 2);
    CHECK(store.getSessionById(otherSongSession).has_value());
}

TEST_CASE("SQLiteStore disabled takes toggle", "[sqlite_store]") {
    TempDir tempDir;
    SQLiteStore store(tempDir.dbPath());
    REQUIRE(store.open());

    store.setTakeDisabled("song-a", "drums", "take-02.wav", true);
    store.setTakeDisabled("song-a", "drums", "take-02.wav", true);
    store.setTakeDisabled("song-a", "bass", "take-01.wav", true);
    store.setTakeDisabled("song-b", "bass", "take-01.wav", true);

    auto disabled = store.getDisabledTakes("song-a");
    CHECK(disabled.size() == 2);
    CHECK(disabled.count({"song-a", "drums", "take-02.wav"}) == 1);

    store.setTakeDisabled("song-a", "drums", "take-02.wav", false);
    disabled = store.getDisabledTakes("song-a");
    REQUIRE(disabled.size() == 1);
    CHECK(disabled.begin()->trackId == "bass");
}
