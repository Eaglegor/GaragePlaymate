# TASK-17: Session Persistence

**Epic:** E5 — Random takes and session history  
**Depends on:** TASK_07, TASK_04  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement session save/load layer on top of `SQLiteStore` with automatic pruning to retain only the last 100 sessions per song.

## Requirements (from PDD)

- FR-RAND-03 — persist session: timestamp, songId, take map, failure events
- FR-HIST-01 — last 100 sessions per song, oldest pruned
- FR-HIST-02 — replay loads exact take map
- §15 decision #5 — count-based 100 limit

## Scope

### Create/modify

- `src/storage/SessionRepository.h` / `SessionRepository.cpp` — thin layer over SQLiteStore
- `tests/core/test_session_repository.cpp`

### Do NOT

- Orchestrate playback (TASK_18)
- Build history UI (TASK_27)

## Implementation notes

1. API:
   ```cpp
   class SessionRepository {
   public:
       explicit SessionRepository(SQLiteStore& store);

       int64_t saveSession(const PlaybackSession& session);
       std::vector<PlaybackSession> listSessions(const std::string& songId, int limit = 100);
       std::optional<PlaybackSession> getSession(int64_t sessionId);

       static constexpr int kMaxSessionsPerSong = 100;
   private:
       void pruneOldSessions(const std::string& songId);
   };
   ```

2. `saveSession`:
   - Set `timestampUnixMs` to now if zero
   - Insert via `SQLiteStore::insertSession`
   - Call `pruneOldSessions(songId)`

3. `pruneOldSessions`: `deleteSessionsForSongExceptNewest(songId, 100)` from TASK_07.

4. `listSessions`: ordered by timestamp descending, include take map and failure events.

5. Human-readable take labels for UI: helper `formatTakeLabel(const std::string& filename)` → strip extension or show as-is (FR-HIST-03).

6. Unit tests:
   - Insert 101 sessions for one song → count is 100, oldest gone
   - Round-trip take map and failure events

## Verification

- [ ] 101st session evicts oldest
- [ ] `getSession` returns identical take map
- [ ] Tests pass without audio device
