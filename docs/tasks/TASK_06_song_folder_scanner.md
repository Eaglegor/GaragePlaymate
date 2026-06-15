# TASK-06: Song Folder Scanner

**Epic:** E2 — Song library and persistence foundation  
**Depends on:** TASK_05  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Recursively scan `{dataRoot}/songs/` for song folders, parse manifests, enumerate WAV takes per track, and populate `Song` objects with take metadata.

## Requirements (from PDD)

- FR-LIB-01 — scan on startup/rescan (scanner module; wiring in TASK_21)
- FR-LIB-02 — track count, take counts
- Supported audio §6.2 — WAV only; ignore/warn on other extensions
- Acceptance test #13 — non-WAV ignored

## Scope

### Create/modify

- `src/storage/WavMetadataReader.h` / `WavMetadataReader.cpp` — read duration, sample rate, channels from WAV header (no full decode)
- `src/storage/SongFolderScanner.h` / `SongFolderScanner.cpp`
- `tests/core/test_song_folder_scanner.cpp`
- `tests/fixtures/songs/scan-test/` — folder with song.yaml + track subfolders + sample WAV files (can be minimal silent WAVs generated in test or TASK_30)

### Do NOT

- Persist to SQLite (TASK_07, TASK_21)
- Apply disabled-takes filter (TASK_17 / LibraryService)
- Block scan on misaligned takes

## Implementation notes

1. API:
   ```cpp
   struct ScanWarning { std::string path; std::string message; };
   struct ScanResult {
       std::vector<Song> songs;
       std::vector<ScanWarning> warnings;
   };
   ScanResult scanLibrary(const std::filesystem::path& dataRoot);
   ```

2. Scan algorithm:
   - Look for `{dataRoot}/songs/` — if missing, return empty with warning
   - Each immediate subdirectory containing `song.yaml` is a song
   - Parse via `SongYamlParser`
   - For each track, list files in `track.folderPath`; include only `.wav` (case-insensitive)
   - Sort takes by filename ascending
   - Read duration via `WavMetadataReader`; on failure log warning, durationMs=0

3. `WavMetadataReader`: parse RIFF/WAVE fmt chunk for sample rate, channels, bits, data size → durationMs. No JUCE required (pure C++ or minimal header-only).

4. Tolerate unreadable files (cloud lock): catch exceptions, add `ScanWarning`, continue.

5. Ignore nested song folders inside track directories.

6. Unit test: scan `tests/fixtures/songs/scan-test/` → 1 song, correct take counts, warning for `.mp3` dummy file if present.

## Verification

- [ ] Scanner finds songs only under `songs/` subdirectory
- [ ] WAV takes enumerated; non-WAV produces warning not crash
- [ ] Unit test passes without audio device
- [ ] Builds on Windows with MSVC
