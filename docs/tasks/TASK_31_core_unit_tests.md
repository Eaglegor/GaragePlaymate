# TASK-31: Core Unit Tests

**Epic:** E8 — Polish and packaging  
**Depends on:** TASK_05, TASK_16, TASK_19, TASK_14, TASK_17  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Consolidate and expand Catch2 unit tests for all pure core/storage logic; ensure CI-ready test target runs headless.

## Requirements (from PDD)

- Acceptance criteria support (§13)
- ARCHITECTURE §14 — Catch2 for core tests

## Scope

### Create/modify

- `cmake/FetchCatch2.cmake` if not present
- `tests/core/` — ensure tests exist for:
  - `test_song_yaml_parser.cpp` (TASK_05)
  - `test_song_folder_scanner.cpp` (TASK_06)
  - `test_section_navigator.cpp` (TASK_14)
  - `test_random_take_selector.cpp` (TASK_16)
  - `test_failure_simulator.cpp` (TASK_19)
  - `test_session_repository.cpp` (TASK_17)
  - `test_data_path_resolver.cpp` (TASK_08)
  - `test_sqlite_store.cpp` (TASK_07)
- `tests/CMakeLists.txt` — single `GaragePlaymateTests` executable
- Root `CMakeLists.txt` — `enable_testing()` + `add_test`

### Do NOT

- Add GUI or audio device integration tests (manual acceptance)

## Implementation notes

1. Catch2 v3 with `Catch2::Catch2WithMain` or separate main.

2. Test executable links: core, storage; no JUCE GUI.

3. Use temp directories for SQLite and scan fixtures (`std::filesystem::temp_directory_path`).

4. Add aggregate test or script documented in README:
   ```
   ctest -C Release --output-on-failure
   ```

5. Fill gaps if earlier tasks skipped tests — this task ensures **minimum 80% of core modules** have at least one meaningful test.

6. Optional: GitHub Actions workflow `.github/workflows/test.yml` — out of scope unless user wants; mention in README only.

## Verification

- [ ] `GaragePlaymateTests` builds and all tests pass
- [ ] `ctest` runs from build directory
- [ ] No test requires audio hardware or GUI
