# TASK-22: Setlist CRUD

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_07  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement setlist create/read/update/delete operations via service layer on top of `SQLiteStore`.

## Requirements (from PDD)

- FR-LIB-03 — multiple setlists; song may be in zero or more
- FR-LIB-04 — persist in app config, not song folders

## Scope

### Create/modify

- `src/services/SetlistService.h` / `SetlistService.cpp`
- `tests/core/test_setlist_service.cpp` (optional if SQLiteStore tests cover enough)

### Do NOT

- Build setlist UI (TASK_24)

## Implementation notes

1. API:
   ```cpp
   class SetlistService {
   public:
       explicit SetlistService(SQLiteStore& store);

       int64_t createSetlist(const std::string& name);
       void deleteSetlist(int64_t id);
       void renameSetlist(int64_t id, const std::string& newName);
       std::vector<SetlistInfo> listSetlists() const;

       void setSongs(int64_t setlistId, const std::vector<std::string>& songIdsOrdered);
       std::vector<std::string> getSongIds(int64_t setlistId) const;

       void addSong(int64_t setlistId, const std::string& songId, int insertIndex = -1);
       void removeSong(int64_t setlistId, const std::string& songId);
       void reorderSong(int64_t setlistId, int oldIndex, int newIndex);
   };
   ```

2. Duplicate setlist names → return error or auto-suffix "(2)" — prefer clear error to UI.

3. `addSong`/`removeSong` maintain contiguous `sort_order`.

4. Song may appear in multiple setlists (no uniqueness constraint across setlists).

## Verification

- [ ] Create setlist, add 3 songs, reorder, list returns correct order
- [ ] Delete setlist removes join rows (CASCADE)
- [ ] Same song in two setlists works
