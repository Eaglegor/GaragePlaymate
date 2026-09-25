#pragma once

#include "core/SessionTypes.h"

#include <compare>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct sqlite3;

namespace garageplaymate {

struct StorageError : std::exception {
    std::string message;

    explicit StorageError(std::string message) : message(std::move(message)) {}

    const char* what() const noexcept override { return message.c_str(); }
};

struct SetlistInfo {
    int64_t id = 0;
    std::string name;
    int64_t createdAtMs = 0;
};

struct DisabledTakeKey {
    std::string songId;
    std::string trackId;
    std::string takeFilename;

    auto operator<=>(const DisabledTakeKey&) const = default;
};

// A persisted history entry. Loaded sessions carry take filenames only
// (durationMs = 0); callers resolve them against the scanned Song if needed.
struct StoredSession {
    PlaybackSession session;
    int64_t timestampMs = 0;
    bool isReplay = false;
};

// SQLite persistence for settings, setlists, session history and disabled takes.
// Not thread-safe: call from the message thread only. Methods other than open()
// throw StorageError on SQLite failures.
class SQLiteStore {
public:
    explicit SQLiteStore(const std::filesystem::path& dbPath);
    ~SQLiteStore();

    SQLiteStore(const SQLiteStore&) = delete;
    SQLiteStore& operator=(const SQLiteStore&) = delete;

    // Opens (creating if needed) the database file and applies the schema.
    bool open(std::string* errorMessage = nullptr);
    bool isOpen() const;
    void applySchema();

    // Settings — JSON string blob per key
    std::optional<std::string> getSetting(const std::string& key);
    void setSetting(const std::string& key, const std::string& jsonValue);

    // Setlists
    int64_t createSetlist(const std::string& name);
    void deleteSetlist(int64_t id);
    void renameSetlist(int64_t id, const std::string& name);
    std::vector<SetlistInfo> listSetlists();
    void setSetlistSongs(int64_t setlistId, const std::vector<std::string>& songIdsOrdered);
    std::vector<std::string> getSetlistSongIds(int64_t setlistId);

    // Sessions — returned newest first
    int64_t insertSession(const StoredSession& storedSession);
    std::vector<StoredSession> getSessionsForSong(const std::string& songId, int limit);
    std::optional<StoredSession> getSessionById(int64_t sessionId);
    void deleteSessionsForSongExceptNewest(const std::string& songId, int keepCount);

    // Disabled takes
    void setTakeDisabled(const std::string& songId, const std::string& trackId,
                         const std::string& takeFilename, bool disabled);
    std::set<DisabledTakeKey> getDisabledTakes(const std::string& songId);

private:
    struct DbCloser {
        void operator()(sqlite3* db) const;
    };

    sqlite3* requireDb();
    void execute(const char* sql);
    void loadSessionDetails(StoredSession& storedSession);

    std::filesystem::path dbPath_;
    std::unique_ptr<sqlite3, DbCloser> db_;
};

}  // namespace garageplaymate
