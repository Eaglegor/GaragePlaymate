# TASK-21: Library Service

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_06, TASK_08  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `LibraryService` that scans the effective data root on startup and manual rescan, maintaining an in-memory song cache and exposing summaries for UI.

## Requirements (from PDD)

- FR-LIB-01 — scan on startup and Rescan
- FR-LIB-02 — display title, bpm, track count, take counts
- NFR-UX-01 — songs appear without import dialogs

## Scope

### Create/modify

- `src/services/LibraryService.h` / `LibraryService.cpp`
- Wire scan call from `AppController` on startup (after SettingsService available or with default path)

### Do NOT

- Setlist logic (TASK_22)
- Library UI (TASK_24)

## Implementation notes

1. API:
   ```cpp
   class LibraryService {
   public:
       LibraryService(SongFolderScanner& scanner);

       void setDataRoot(const std::filesystem::path& effectiveDataRoot);
       void rescan();  // synchronous on message thread for v1; optional async later

       const std::vector<Song>& getSongs() const;
       std::vector<SongSummary> getSongSummaries() const;
       const Song* findSongById(const std::string& id) const;

       const std::vector<ScanWarning>& getLastWarnings() const;

       std::function<void()> onLibraryChanged;
   };
   ```

2. `rescan()`: call `SongFolderScanner::scanLibrary(dataRoot)`; replace cache; invoke `onLibraryChanged`.

3. Merge disabled-take flags from SQLite in later integration — for now accept optional `SQLiteStore*` to mark `TakeFile.disabled` on scan post-process.

4. Build `SongSummary` from each `Song`.

5. Log warnings to `juce::Logger`.

## Verification

- [ ] With valid `data/songs/demo-song/`, rescan populates cache
- [ ] `findSongById` returns nullptr for missing id
- [ ] `onLibraryChanged` fires after rescan
- [ ] Missing data root → empty library, warning logged
