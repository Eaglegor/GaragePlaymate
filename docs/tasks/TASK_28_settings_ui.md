# TASK-28: Settings UI

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_09, TASK_21, TASK_23  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build `SettingsPanel` dialog or view for data path, audio device, loading mode, failure simulation defaults, and library rescan.

## Requirements (from PDD)

- §6.7, §8 Settings screen
- FR-LIB-01 Rescan
- FR-PLAY-06–08, FR-PLAY-09
- FR-FAIL-01 defaults

## Scope

### Create/modify

- `src/ui/SettingsPanel.h` / `SettingsPanel.cpp`
- Menu: File or View → Settings opens panel (modal dialog OK for v1)

### Do NOT

- Advanced EQ or plugin settings

## Implementation notes

1. **Data root:**
   - Text field + Browse button (`juce::FileChooser` selectDirectory)
   - Show effective path read-only subtitle
   - Warn that changing path requires rescan

2. **Audio:**
   - Driver combo: WASAPI / ASIO (ASIO hidden if build disabled)
   - Output device combo populated from `AudioDeviceService::listOutputDevices`
   - Buffer size combo: 128, 256, 512, 1024
   - Apply button calls `AudioDeviceService::applySettings`

3. **Loading mode:** Radio Stream / Preload; show RAM estimate formula when Preload selected.

4. **Failure simulation:**
   - Enable checkbox
   - Probability slider 5–30% (allow 0–100% for testing but label recommended range)
   - Failure level slider 0–50%
   - Fade ms spinner 100–1000

5. **Rescan library** button → `LibraryService::rescan()` + toast/alert with warning count.

6. Save / Cancel buttons → `SettingsService::save` on Save.

7. First launch: user never required to open this for playback (NFR-UX-01).

## Verification

- [ ] Change data path + rescan → library updates
- [ ] WASAPI device switch works
- [ ] Settings persist across app restart
- [ ] Failure sliders persist
