#include "storage/SQLiteStore.h"

#include "SchemaSql.h"

#include <sqlite3.h>

#include <chrono>
#include <string_view>

namespace garageplaymate {
namespace {

// Stored in PRAGMA user_version. Add migration steps in applySchema() when bumping.
constexpr int kSchemaVersion = 1;

[[noreturn]] void throwStorageError(sqlite3* db, std::string_view context) {
    std::string message(context);
    if (db != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(db);
    }
    throw StorageError(std::move(message));
}

int64_t nowUnixMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throwStorageError(db, std::string("Failed to prepare '") + sql + "'");
        }
    }

    ~Statement() { sqlite3_finalize(stmt_); }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int index, int64_t value) {
        check(sqlite3_bind_int64(stmt_, index, value), "bind int");
        return *this;
    }

    Statement& bind(int index, double value) {
        check(sqlite3_bind_double(stmt_, index, value), "bind double");
        return *this;
    }

    Statement& bind(int index, const std::string& value) {
        check(sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT),
              "bind text");
        return *this;
    }

    // Returns true while a row is available.
    bool step() {
        const int result = sqlite3_step(stmt_);
        if (result == SQLITE_ROW) {
            return true;
        }
        if (result != SQLITE_DONE) {
            throwStorageError(db_, "Failed to execute statement");
        }
        return false;
    }

    void run() {
        step();
        sqlite3_reset(stmt_);
        sqlite3_clear_bindings(stmt_);
    }

    int64_t columnInt64(int column) const { return sqlite3_column_int64(stmt_, column); }

    double columnDouble(int column) const { return sqlite3_column_double(stmt_, column); }

    std::string columnText(int column) const {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
        return text == nullptr ? std::string() : std::string(text, static_cast<size_t>(sqlite3_column_bytes(stmt_, column)));
    }

private:
    void check(int result, const char* context) {
        if (result != SQLITE_OK) {
            throwStorageError(db_, context);
        }
    }

    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

// Rolls back unless commit() is called.
class Transaction {
public:
    explicit Transaction(sqlite3* db) : db_(db) { exec("BEGIN"); }

    ~Transaction() {
        if (!committed_) {
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit() {
        exec("COMMIT");
        committed_ = true;
    }

private:
    void exec(const char* sql) {
        if (sqlite3_exec(db_, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            throwStorageError(db_, sql);
        }
    }

    sqlite3* db_ = nullptr;
    bool committed_ = false;
};

}  // namespace

void SQLiteStore::DbCloser::operator()(sqlite3* db) const {
    sqlite3_close_v2(db);
}

SQLiteStore::SQLiteStore(const std::filesystem::path& dbPath) : dbPath_(dbPath) {}

SQLiteStore::~SQLiteStore() = default;

bool SQLiteStore::open(std::string* errorMessage) {
    db_.reset();

    const std::u8string pathUtf8 = dbPath_.u8string();
    sqlite3* rawDb = nullptr;
    const int result = sqlite3_open_v2(reinterpret_cast<const char*>(pathUtf8.c_str()), &rawDb,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    std::unique_ptr<sqlite3, DbCloser> db(rawDb);
    if (result != SQLITE_OK) {
        if (errorMessage != nullptr) {
            *errorMessage = "Failed to open database '" + dbPath_.string() + "': " +
                            (rawDb != nullptr ? sqlite3_errmsg(rawDb) : sqlite3_errstr(result));
        }
        return false;
    }

    db_ = std::move(db);
    try {
        execute("PRAGMA foreign_keys = ON");
        applySchema();
    } catch (const StorageError& error) {
        if (errorMessage != nullptr) {
            *errorMessage = error.what();
        }
        db_.reset();
        return false;
    }
    return true;
}

bool SQLiteStore::isOpen() const {
    return db_ != nullptr;
}

void SQLiteStore::applySchema() {
    sqlite3* db = requireDb();

    int64_t currentVersion = 0;
    {
        Statement versionQuery(db, "PRAGMA user_version");
        if (versionQuery.step()) {
            currentVersion = versionQuery.columnInt64(0);
        }
    }
    if (currentVersion > kSchemaVersion) {
        throw StorageError("Database schema version " + std::to_string(currentVersion) +
                           " is newer than supported version " + std::to_string(kSchemaVersion));
    }

    Transaction transaction(db);
    execute(kSchemaSql);
    execute(("PRAGMA user_version = " + std::to_string(kSchemaVersion)).c_str());
    transaction.commit();
}

std::optional<std::string> SQLiteStore::getSetting(const std::string& key) {
    Statement statement(requireDb(), "SELECT value FROM settings WHERE key = ?1");
    statement.bind(1, key);
    if (!statement.step()) {
        return std::nullopt;
    }
    return statement.columnText(0);
}

void SQLiteStore::setSetting(const std::string& key, const std::string& jsonValue) {
    Statement statement(requireDb(),
                        "INSERT INTO settings (key, value) VALUES (?1, ?2) "
                        "ON CONFLICT(key) DO UPDATE SET value = excluded.value");
    statement.bind(1, key).bind(2, jsonValue).run();
}

int64_t SQLiteStore::createSetlist(const std::string& name) {
    sqlite3* db = requireDb();
    Statement statement(db, "INSERT INTO setlists (name, created_at_ms) VALUES (?1, ?2)");
    statement.bind(1, name).bind(2, nowUnixMs()).run();
    return sqlite3_last_insert_rowid(db);
}

void SQLiteStore::deleteSetlist(int64_t id) {
    Statement statement(requireDb(), "DELETE FROM setlists WHERE id = ?1");
    statement.bind(1, id).run();
}

void SQLiteStore::renameSetlist(int64_t id, const std::string& name) {
    Statement statement(requireDb(), "UPDATE setlists SET name = ?2 WHERE id = ?1");
    statement.bind(1, id).bind(2, name).run();
}

std::vector<SetlistInfo> SQLiteStore::listSetlists() {
    Statement statement(requireDb(), "SELECT id, name, created_at_ms FROM setlists ORDER BY created_at_ms, id");
    std::vector<SetlistInfo> setlists;
    while (statement.step()) {
        setlists.push_back(SetlistInfo{statement.columnInt64(0), statement.columnText(1), statement.columnInt64(2)});
    }
    return setlists;
}

void SQLiteStore::setSetlistSongs(int64_t setlistId, const std::vector<std::string>& songIdsOrdered) {
    sqlite3* db = requireDb();
    Transaction transaction(db);

    Statement clear(db, "DELETE FROM setlist_songs WHERE setlist_id = ?1");
    clear.bind(1, setlistId).run();

    // OR IGNORE: a song appears at most once per setlist; the first position wins.
    Statement insert(db, "INSERT OR IGNORE INTO setlist_songs (setlist_id, song_id, sort_order) VALUES (?1, ?2, ?3)");
    int64_t sortOrder = 0;
    for (const std::string& songId : songIdsOrdered) {
        insert.bind(1, setlistId).bind(2, songId).bind(3, sortOrder++).run();
    }
    transaction.commit();
}

std::vector<std::string> SQLiteStore::getSetlistSongIds(int64_t setlistId) {
    Statement statement(requireDb(),
                        "SELECT song_id FROM setlist_songs WHERE setlist_id = ?1 ORDER BY sort_order");
    statement.bind(1, setlistId);
    std::vector<std::string> songIds;
    while (statement.step()) {
        songIds.push_back(statement.columnText(0));
    }
    return songIds;
}

int64_t SQLiteStore::insertSession(const StoredSession& storedSession) {
    sqlite3* db = requireDb();
    const PlaybackSession& session = storedSession.session;
    Transaction transaction(db);

    Statement insertSessionRow(db, "INSERT INTO sessions (song_id, timestamp_ms, is_replay) VALUES (?1, ?2, ?3)");
    insertSessionRow.bind(1, session.songId)
        .bind(2, storedSession.timestampMs)
        .bind(3, static_cast<int64_t>(storedSession.isReplay ? 1 : 0))
        .run();
    const int64_t sessionId = sqlite3_last_insert_rowid(db);

    Statement insertTake(db, "INSERT INTO session_takes (session_id, track_id, take_filename) VALUES (?1, ?2, ?3)");
    for (const auto& [trackId, take] : session.takeMap) {
        if (take == nullptr) {
            continue;
        }
        insertTake.bind(1, sessionId).bind(2, trackId).bind(3, take->filename).run();
    }

    Statement insertFailure(db,
                            "INSERT INTO failure_events (session_id, track_id, onset_ms, restore_ms, failure_level) "
                            "VALUES (?1, ?2, ?3, ?4, ?5)");
    for (const FailureEvent& event : session.failureEvents) {
        double failureLevel = 0.0;
        if (const auto* volumeDrop = std::get_if<VolumeDropFailure>(&event.eventData)) {
            failureLevel = volumeDrop->failureLevel;
        }
        insertFailure.bind(1, sessionId)
            .bind(2, event.trackId)
            .bind(3, event.onsetMs)
            .bind(4, event.restoreMs)
            .bind(5, failureLevel)
            .run();
    }

    transaction.commit();
    return sessionId;
}

std::vector<StoredSession> SQLiteStore::getSessionsForSong(const std::string& songId, int limit) {
    Statement statement(requireDb(),
                        "SELECT id, song_id, timestamp_ms, is_replay FROM sessions WHERE song_id = ?1 "
                        "ORDER BY timestamp_ms DESC, id DESC LIMIT ?2");
    statement.bind(1, songId).bind(2, static_cast<int64_t>(limit));

    std::vector<StoredSession> sessions;
    while (statement.step()) {
        StoredSession storedSession;
        storedSession.session.id = statement.columnInt64(0);
        storedSession.session.songId = statement.columnText(1);
        storedSession.timestampMs = statement.columnInt64(2);
        storedSession.isReplay = statement.columnInt64(3) != 0;
        sessions.push_back(std::move(storedSession));
    }
    for (StoredSession& storedSession : sessions) {
        loadSessionDetails(storedSession);
    }
    return sessions;
}

std::optional<StoredSession> SQLiteStore::getSessionById(int64_t sessionId) {
    Statement statement(requireDb(), "SELECT id, song_id, timestamp_ms, is_replay FROM sessions WHERE id = ?1");
    statement.bind(1, sessionId);
    if (!statement.step()) {
        return std::nullopt;
    }

    StoredSession storedSession;
    storedSession.session.id = statement.columnInt64(0);
    storedSession.session.songId = statement.columnText(1);
    storedSession.timestampMs = statement.columnInt64(2);
    storedSession.isReplay = statement.columnInt64(3) != 0;
    loadSessionDetails(storedSession);
    return storedSession;
}

void SQLiteStore::deleteSessionsForSongExceptNewest(const std::string& songId, int keepCount) {
    Statement statement(requireDb(),
                        "DELETE FROM sessions WHERE song_id = ?1 AND id NOT IN ("
                        "SELECT id FROM sessions WHERE song_id = ?1 "
                        "ORDER BY timestamp_ms DESC, id DESC LIMIT ?2)");
    statement.bind(1, songId).bind(2, static_cast<int64_t>(keepCount < 0 ? 0 : keepCount)).run();
}

void SQLiteStore::setTakeDisabled(const std::string& songId, const std::string& trackId,
                                  const std::string& takeFilename, bool disabled) {
    const char* sql = disabled
                          ? "INSERT OR IGNORE INTO disabled_takes (song_id, track_id, take_filename) VALUES (?1, ?2, ?3)"
                          : "DELETE FROM disabled_takes WHERE song_id = ?1 AND track_id = ?2 AND take_filename = ?3";
    Statement statement(requireDb(), sql);
    statement.bind(1, songId).bind(2, trackId).bind(3, takeFilename).run();
}

std::set<DisabledTakeKey> SQLiteStore::getDisabledTakes(const std::string& songId) {
    Statement statement(requireDb(),
                        "SELECT song_id, track_id, take_filename FROM disabled_takes WHERE song_id = ?1");
    statement.bind(1, songId);
    std::set<DisabledTakeKey> keys;
    while (statement.step()) {
        keys.insert(DisabledTakeKey{statement.columnText(0), statement.columnText(1), statement.columnText(2)});
    }
    return keys;
}

sqlite3* SQLiteStore::requireDb() {
    if (db_ == nullptr) {
        throw StorageError("Database is not open: " + dbPath_.string());
    }
    return db_.get();
}

void SQLiteStore::execute(const char* sql) {
    char* errorText = nullptr;
    if (sqlite3_exec(requireDb(), sql, nullptr, nullptr, &errorText) != SQLITE_OK) {
        std::string message = std::string("Failed to execute SQL: ") + (errorText != nullptr ? errorText : "");
        sqlite3_free(errorText);
        throw StorageError(std::move(message));
    }
}

void SQLiteStore::loadSessionDetails(StoredSession& storedSession) {
    sqlite3* db = requireDb();
    PlaybackSession& session = storedSession.session;

    Statement takes(db, "SELECT track_id, take_filename FROM session_takes WHERE session_id = ?1");
    takes.bind(1, session.id);
    while (takes.step()) {
        auto take = std::make_shared<TakeFile>();
        take->filename = takes.columnText(1);
        session.takeMap[takes.columnText(0)] = std::move(take);
    }

    Statement failures(db,
                       "SELECT track_id, onset_ms, restore_ms, failure_level FROM failure_events "
                       "WHERE session_id = ?1 ORDER BY onset_ms, rowid");
    failures.bind(1, session.id);
    while (failures.step()) {
        FailureEvent event;
        event.trackId = failures.columnText(0);
        event.onsetMs = failures.columnInt64(1);
        event.restoreMs = failures.columnInt64(2);
        event.eventData = VolumeDropFailure{static_cast<float>(failures.columnDouble(3))};
        session.failureEvents.push_back(std::move(event));
    }
}

}  // namespace garageplaymate
