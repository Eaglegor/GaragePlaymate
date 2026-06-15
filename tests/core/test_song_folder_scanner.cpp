#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "storage/SongFolderScanner.h"
#include "storage/WavMetadataReader.h"

#include <filesystem>

namespace {

std::filesystem::path fixturesRoot() {
    return std::filesystem::path(TEST_FIXTURES_DIR);
}

const garageplaymate::Song* findSong(const garageplaymate::ScanResult& result, const std::string& id) {
    for (const garageplaymate::Song& song : result.songs) {
        if (song.id == id) {
            return &song;
        }
    }
    return nullptr;
}

const garageplaymate::TrackDef* findTrack(const garageplaymate::Song& song, const std::string& id) {
    for (const garageplaymate::TrackDef& track : song.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

bool hasWarningContaining(const garageplaymate::ScanResult& result, const std::string& substring) {
    for (const garageplaymate::ScanWarning& warning : result.warnings) {
        if (warning.message.find(substring) != std::string::npos ||
            warning.path.find(substring) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("readWavMetadata reads duration from a fixture WAV", "[song_folder_scanner]") {
    const auto wavPath = fixturesRoot() / "songs/scan-test/tracks/drums/take-a.wav";
    REQUIRE(std::filesystem::exists(wavPath));

    const std::optional<garageplaymate::WavMetadata> metadata = garageplaymate::readWavMetadata(wavPath);
    REQUIRE(metadata.has_value());
    CHECK(metadata->sampleRate == 44100);
    CHECK(metadata->channels == 1);
    CHECK(metadata->bitsPerSample == 16);
    CHECK(metadata->durationMs == 200);
}

TEST_CASE("scanLibrary finds scan-test song with take counts and non-WAV warning", "[song_folder_scanner]") {
    const garageplaymate::ScanResult result = garageplaymate::scanLibrary(fixturesRoot());

    const garageplaymate::Song* scanTest = findSong(result, "scan-test");
    REQUIRE(scanTest != nullptr);
    CHECK(scanTest->title == "Scan Test Song");
    REQUIRE(scanTest->tracks.size() == 2);

    const garageplaymate::TrackDef* drums = findTrack(*scanTest, "drums");
    REQUIRE(drums != nullptr);
    REQUIRE(drums->takes.size() == 2);
    CHECK(drums->takes[0].filename == "take-a.wav");
    CHECK(drums->takes[0].durationMs == 200);
    CHECK(drums->takes[1].filename == "take-b.wav");
    CHECK(drums->takes[1].durationMs == 150);

    const garageplaymate::TrackDef* bass = findTrack(*scanTest, "bass");
    REQUIRE(bass != nullptr);
    REQUIRE(bass->takes.size() == 1);
    CHECK(bass->takes[0].filename == "take-1.wav");
    CHECK(bass->takes[0].durationMs == 300);

    CHECK(hasWarningContaining(result, "dummy.mp3"));
    CHECK(hasWarningContaining(result, "non-WAV"));
}

TEST_CASE("scanLibrary only scans songs under songs subdirectory", "[song_folder_scanner]") {
    const garageplaymate::ScanResult result = garageplaymate::scanLibrary(fixturesRoot());

    CHECK(findSong(result, "scan-test") != nullptr);
    CHECK(findSong(result, "valid-song") != nullptr);

    for (const garageplaymate::Song& song : result.songs) {
        const std::filesystem::path relative =
            std::filesystem::relative(song.folderPath, fixturesRoot() / "songs");
        CHECK(!relative.empty());
        CHECK(relative.string().find("..") == std::string::npos);
    }
}

TEST_CASE("scanLibrary warns when songs directory is missing", "[song_folder_scanner]") {
    const auto emptyRoot = fixturesRoot() / "missing-songs-root";
    const garageplaymate::ScanResult result = garageplaymate::scanLibrary(emptyRoot);

    CHECK(result.songs.empty());
    REQUIRE_FALSE(result.warnings.empty());
    CHECK(hasWarningContaining(result, "Songs directory does not exist"));
}

TEST_CASE("scanLibrary warns on invalid song manifests without crashing", "[song_folder_scanner]") {
    const garageplaymate::ScanResult result = garageplaymate::scanLibrary(fixturesRoot());

    CHECK(findSong(result, "missing-title") == nullptr);
    CHECK(hasWarningContaining(result, "title"));
}
