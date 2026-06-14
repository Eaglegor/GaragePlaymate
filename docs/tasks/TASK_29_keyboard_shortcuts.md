# TASK-29: Keyboard Shortcuts

**Epic:** E8 — Polish and packaging  
**Depends on:** TASK_25, TASK_28, TASK_21  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Add global keyboard shortcuts: Space = Play/Pause, Left/Right = previous/next section, R = rescan library.

## Requirements (from PDD)

- §8 Keyboard shortcuts recommended

## Scope

### Create/modify

- `src/ui/MainWindow.cpp` — `keyPressed` handler or `juce::KeyPressMappingSet`
- Optionally `src/app/AppController.cpp` — central shortcut dispatch

### Do NOT

- Add shortcuts not in PDD without documenting in README

## Implementation notes

1. Register in `MainWindow::keyPressed`:
   - **Space** — if song detail active: toggle Play/Pause via PlaybackService; consume event
   - **Left** — `PlaybackService::jumpToPreviousSection()` when song loaded
   - **Right** — `PlaybackService::jumpToNextSection()`
   - **R** — `LibraryService::rescan()` (no modifier)

2. Ignore shortcuts when `TextEditor` has keyboard focus (settings search, rename dialog).

3. Document shortcuts in Help → About or README.

4. Use `juce::KeyPress::spaceKey`, `leftKey`, `rightKey`, `createFromDescription("r")`.

## Verification

- [ ] Space toggles playback on song detail view
- [ ] Left/Right jump sections during playback
- [ ] R triggers rescan without crashing
- [ ] Typing in search box does not trigger R/Space globally
