# TASK-05: Song YAML Parser

**Epic:** E2 — Song library and persistence foundation  
**Depends on:** TASK_04  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Parse `song.yaml` manifests into `Song` domain objects with validation of required fields and unit tests.

## Requirements (from PDD)

- FR-LIB-02 (metadata fields)
- Song data model §6.2 (`song.yaml` minimum fields, optional bpm/timeSignature/sections)

## Scope

### Create/modify

- `cmake/FetchYamlCpp.cmake` or inline FetchContent for yaml-cpp
- `src/storage/SongYamlParser.h` / `SongYamlParser.cpp`
- `tests/core/test_song_yaml_parser.cpp`
- `tests/fixtures/songs/valid-song/song.yaml` — minimal valid manifest
- `tests/fixtures/songs/missing-id/song.yaml` — invalid fixture
- Update root `CMakeLists.txt` — yaml-cpp dependency, test target if TASK_01 enabled tests

### Do NOT

- Scan folders or read WAV files (TASK_06)
- Validate audio alignment (out of scope v1)
- Support MP3 or formats other than manifest parsing

## Implementation notes

1. API:
   ```cpp
   namespace garageplaymate {
   struct ParseError { std::string message; };
   std::expected<Song, ParseError> parseSongYaml(
       const std::filesystem::path& songYamlPath,
       const std::filesystem::path& songFolderPath);
   }
   ```
   Use `std::expected` (C++23) or `std::optional<ParseError>` + optional Song if `expected` unavailable — prefer `std::expected` with `/std:c++latest` or polyfill struct.

2. Required YAML fields: `id`, `title`, `tracks[]` with `id`, `name`, `folder`.
3. Optional: `bpm`, `timeSignature` (array [num, den]), `sections[]` with `id`, `name`, `startMs`.
4. Optional track field: `defaultVolume` (float, default 1.0).
5. Resolve track `folder` relative to song folder path → `TrackDef.folderPath`.
6. Sort `sections` ascending by `startMs` after parse.
7. Reject duplicate track ids or section ids with clear error message.
8. `Song.folderPath` = parent directory of `song.yaml`.
9. Unit tests:
   - Valid fixture → correct id, title, 2 tracks, 3 sections sorted
   - Missing `id` → error
   - Duplicate section id → error

## Verification

- [ ] `cmake --build build --config Release --target GaragePlaymateTests` passes (or equivalent test target name)
- [ ] All parser unit tests green
- [ ] Parser does not link JUCE GUI modules
