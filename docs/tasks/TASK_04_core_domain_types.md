# TASK-04: Core Domain Types

**Epic:** E2 — Song library and persistence foundation  
**Depends on:** TASK_01  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Define pure C++ domain structs for songs, tracks, takes, sections, sessions, and settings enums — no JUCE or external library dependencies.

## Requirements (from PDD)

- Song data model §6.2
- Session and failure event model §6.4–6.6
- Settings §6.7

## Scope

### Create/modify

- `src/core/SongTypes.h`
- `src/core/SessionTypes.h`
- `src/core/SettingsTypes.h`
- Update `CMakeLists.txt` — add `GaragePlaymateCore` static library or header-only target consumed by main app

### Do NOT

- Implement parsing, scanning, or persistence
- Include any JUCE headers
- Add YAML or SQLite types

## Implementation notes

1. Match structs exactly to ARCHITECTURE §5.1–5.3:
   - `TakeFile`, `TrackDef`, `Section`, `Song`
   - `FailureEvent`, `PlaybackSession`
   - `AudioDriverType`, `AudioLoadingMode`, `AppSettings`
2. Add lightweight summary type for library UI:
   ```cpp
   struct SongSummary {
       std::string id;
       std::string title;
       std::optional<int> bpm;
       int trackCount = 0;
       int totalTakeCount = 0;  // sum of takes across tracks
   };
   ```
3. Add helper free functions in same headers (inline):
   - `int64_t songMaxTakeDurationMs(const Song&)` — max of all take durations (0 if unknown)
   - `const Section* findSectionAtMs(const Song&, int64_t positionMs)` — returns current section
4. Use `#pragma once`, namespace `garageplaymate`.
5. `Song.folderPath`, `TrackDef.folderPath`, `TakeFile.absolutePath` use `std::filesystem::path`.

## Verification

- [ ] Headers compile in isolation with C++20 (`<optional>`, `<filesystem>`, `<map>`, `<vector>`, `<string>`)
- [ ] No JUCE or third-party includes
- [ ] Project still builds after adding core library to CMake
