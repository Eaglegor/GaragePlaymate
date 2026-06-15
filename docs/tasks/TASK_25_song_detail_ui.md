# TASK-25: Song Detail UI

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_18, TASK_24  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build `SongDetailPanel` with section strip, per-track volume sliders, current take indicators, transport buttons, and preload progress bar.

## Requirements (from PDD)

- UI outline §8 — Song detail
- FR-PLAY-02, FR-PLAY-03, FR-PLAY-09
- User story: see which take variant selected per instrument

## Scope

### Create/modify

- `src/ui/SongDetailPanel.h` / `SongDetailPanel.cpp`
- Update `MainWindow` / `AppController` — show detail when song selected (split or stacked layout)

### Do NOT

- Session bar (TASK_26)
- History panel (TASK_27)
- Mute/solo (TASK_33 optional)

## Implementation notes

1. Header: song title, BPM if present, back button to library-only view optional.

2. Section strip: horizontal row of buttons from `Song.sections`; click → `PlaybackService::jumpToSection(id)`; highlight current section from playback position callback.

3. Per track row:
   - Track name label
   - Volume slider 0–100% mapped to linear 0–1 → `PlaybackService::setTrackVolume(trackId, gain)`
   - Take indicator: filename of current take from `PlaybackState.currentTakeMap`
   - Optional: take duration label

4. Transport row: Play/Pause, Stop buttons bound to PlaybackService.

5. Preload progress: `juce::ProgressBar` visible when `PlaybackState.isPreloading`; show `PreloadProgress`.

6. Disable Play during preload; show Cancel button optional (nice-to-have).

7. Subscribe to `PlaybackService::onStateChanged` at 30Hz max for UI refresh.

## Verification

- [ ] Open song → sections and tracks displayed
- [ ] Play shows take filenames after selection
- [ ] Volume slider affects mix during playback
- [ ] Section button seeks (after TASK_15)
- [ ] Preload mode shows progress bar
