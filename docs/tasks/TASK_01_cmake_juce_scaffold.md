# TASK-01: CMake + JUCE Scaffold

**Epic:** E1 — Project scaffold  
**Depends on:** (none — first task)  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Create a buildable Windows x64 CMake project that fetches JUCE 8 and produces an empty executable named `GaragePlaymate`.

## Requirements (from PDD)

- Technology stack: C++20, JUCE 8, CMake + MSVC 2022 (§9.1)
- Platform: Windows 10/11 x64 (NFR-PORT-01)

## Scope

### Create/modify

- `CMakeLists.txt` (root)
- `cmake/FetchJuce.cmake` — FetchContent or submodule pattern for JUCE 8
- `cmake/CompilerWarnings.cmake` — MSVC warning flags for C++20
- `src/main.cpp` — minimal `main()` returning 0 (placeholder until TASK_03)
- `.gitignore` — build dirs, `.vs/`, `out/`, `build/`

### Do NOT

- Add UI code (TASK_03)
- Add LICENSE/README (TASK_02)
- Add yaml-cpp, SQLite, or Catch2 yet (later tasks)

## Implementation notes

1. Require CMake 3.22+, project language CXX only, `CMAKE_CXX_STANDARD 20`.
2. Fetch JUCE 8 from GitHub (`juce-framework/JUCE`, tag `8.0.0` or latest 8.x stable).
3. Create JUCE GUI application target:
   ```cmake
   juce_add_gui_app(GaragePlaymate
       PRODUCT_NAME "GaragePlaymate"
       COMPANY_NAME "GaragePlaymate"
       NEEDS_MIDI_INPUT FALSE
       NEEDS_MIDI_OUTPUT FALSE
   )
   ```
4. Link minimum modules per ARCHITECTURE §14: `juce_core`, `juce_events`, `juce_data_structures`, `juce_gui_basics`, `juce_gui_extra`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_utils`.
5. Define CMake options (defaults):
   - `GARAGEPLAYMATE_PORTABLE_BUILD` OFF
   - `GARAGEPLAYMATE_ENABLE_ASIO` ON
   - `GARAGEPLAYMATE_BUILD_TESTS` ON
6. When `GARAGEPLAYMATE_ENABLE_ASIO` is ON, add JUCE ASIO define (`JUCE_ASIO=1`).
7. Pass compile definition `GARAGEPLAYMATE_PORTABLE_BUILD=$<BOOL:...>` to target when portable option is set.
8. Set output directory to `bin/` under build tree for clarity.
9. `src/main.cpp` can be a stub; TASK_03 replaces it with JUCE Application entry.

## Verification

- [ ] `cmake -B build -G "Visual Studio 17 2022" -A x64` succeeds
- [ ] `cmake --build build --config Release` succeeds
- [ ] `build/bin/Release/GaragePlaymate.exe` exists (or equivalent VS output path documented in README stub comment)
- [ ] No warnings at `/W4` level for scaffold files
