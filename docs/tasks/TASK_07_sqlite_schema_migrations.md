# TASK-07: SQLite Schema and Store

**Epic:** E2 — Song library and persistence foundation  
**Depends on:** TASK_01, TASK_04  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Bootstrap SQLite database with schema from ARCHITECTURE §8 and implement `SQLiteStore` with CRUD operations for settings, setlists, sessions, and disabled takes.

## Requirements (from PDD)

- FR-LIB-03, FR-LIB-04 — setlists and settings in app config
- FR-RAND-03, FR-HIST-01 — session persistence, 100/song cap (cap enforced in TASK_17)

## Scope

### Create/modify

- `cmake/FetchSQLite.cmake` or embed amalgamation
- `src/storage/Schema.sql` — exact schema from ARCHITECTURE §8
- `src/storage/SQLiteStore.h` / `SQLiteStore.cpp`
- `tests/core/test_sqlite_store.cpp` — basic round-trip tests
- Update `CMakeLists.txt`

### Do NOT

- Implement session prune logic (TASK_17) — but provide `deleteSessionsForSongExceptNewest(songId, keepN)` helper
- Wire to UI or services
- Store song manifests in DB (folder is source of truth)

## Implementation notes

1. API surface:
   ```cpp
   class SQLiteStore {
   public:
       explicit SQLiteStore(const std::filesystem::path& dbPath);
       bool open();
       void applySchema();  // run Schema.sql idempotently (CREATE IF NOT EXISTS)

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

       // Sessions
       int64_t insertSession(const PlaybackSession& session);
       std::vector<PlaybackSession> getSessionsForSong(const std::string& songId, int limit);
       std::optional<PlaybackSession> getSessionById(int64_t sessionId);
       void deleteSessionsForSongExceptNewest(const std::string& songId, int keepCount);

       // Disabled takes
       void setTakeDisabled(const std::string& songId, const std::string& trackId,
                            const std::string& takeFilename, bool disabled);
       std::set<DisabledTakeKey> getDisabledTakes(const std::string& songId);
   };
   ```

2. Define small DTO `SetlistInfo { int64_t id; std::string name; int64_t createdAtMs; }`.

3. Use prepared statements; wrap SQLite C API in RAII (`sqlite3_stmt` unique_ptr deleter).

4. DB file path passed in from caller (TASK_08 resolves path).

5. Thread safety: all methods called from message thread only (document in header comment).

6. Unit tests use temp directory DB file; test settings round-trip, setlist CRUD, session insert/load.

## Verification

- [ ] Schema creates all tables on fresh DB
- [ ] Re-opening existing DB does not error (idempotent schema)
- [ ] Unit tests pass
- [ ] No JUCE GUI dependency in SQLiteStore
