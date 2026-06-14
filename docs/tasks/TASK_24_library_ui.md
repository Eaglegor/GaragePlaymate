# TASK-24: Library UI

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_03, TASK_21, TASK_22  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build `LibraryPanel` with song list, setlist sidebar, and search/filter; wire to `LibraryService` and `SetlistService` via `AppController`.

## Requirements (from PDD)

- UI outline §8 — Library screen
- FR-LIB-02, FR-LIB-03

## Scope

### Create/modify

- `src/ui/LibraryPanel.h` / `LibraryPanel.cpp`
- `src/app/AppController.h` / `AppController.cpp` — wire services and selection callbacks
- `src/ui/MainWindow.cpp` — host LibraryPanel as primary view

### Do NOT

- Song detail panel (TASK_25)
- Settings panel (TASK_28)

## Implementation notes

1. Layout:
   - Left sidebar (~220px): setlist list + "All Songs" pseudo-entry + Create/Rename/Delete setlist buttons
   - Main area: `juce::ListBox` or `TableListBox` of songs showing title, BPM, track count, take count
   - Top: search `TextEditor` filtering by title (case-insensitive substring)

2. Selecting setlist filters song list to members; "All Songs" shows full library.

3. Double-click or Enter on song → `AppController::openSong(songId)` callback for TASK_25.

4. Subscribe to `LibraryService::onLibraryChanged` → refresh list on message thread.

5. Empty state label: "No songs found. Add folders to {dataRoot}/songs/ and click Rescan in Settings."

6. Use JUCE look-and-feel defaults; no custom skin required.

## Verification

- [ ] Songs from library appear in list after rescan
- [ ] Search filters list live
- [ ] Setlist selection shows subset only
- [ ] Create setlist appears in sidebar
