# TASK-02: LICENSE, NOTICE, README

**Epic:** E1 — Project scaffold  
**Depends on:** TASK_01  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Add GPL-3.0 licensing, third-party NOTICE file, and a minimal README with build instructions.

## Requirements (from PDD)

- Open source GPL-3.0 (§9.3, §15)
- JUCE GPL path; Steinberg ASIO SDK notice when ASIO enabled
- yaml-cpp, SQLite permissive notices (future deps — include in NOTICE now)

## Scope

### Create/modify

- `LICENSE` — full GPL-3.0 text, copyright year 2026, project name GaragePlaymate
- `NOTICE` — attributions for:
  - JUCE Framework (GPL/commercial dual license — project uses GPL path)
  - Steinberg ASIO SDK (when `GARAGEPLAYMATE_ENABLE_ASIO` is ON)
  - yaml-cpp (MIT)
  - SQLite (public domain)
  - Catch2 (BSL-1.0, for future tests)
- `README.md` — project description (1 paragraph), prerequisites, build steps, run instructions, link to `docs/PRODUCT_DESIGN.md` and `docs/ARCHITECTURE.md`

### Do NOT

- Implement features or change CMake beyond adding a comment referencing LICENSE in `CMakeLists.txt` if desired
- Add CONTRIBUTING or CODE_OF_CONDUCT unless minimal one-liner in README

## Implementation notes

1. README build section must match TASK_01 CMake commands exactly (VS 2022, x64).
2. Document CMake options: `GARAGEPLAYMATE_PORTABLE_BUILD`, `GARAGEPLAYMATE_ENABLE_ASIO`, `GARAGEPLAYMATE_BUILD_TESTS`.
3. Note ASIO requires Steinberg license acceptance for developers enabling ASIO.
4. State v1 is Windows-only; song data format described briefly with link to PDD §6.2.
5. Default data paths (portable vs installed) — reference PDD §6.7; full implementation in TASK_08.

## Verification

- [ ] `LICENSE` contains GPL-3.0 full text
- [ ] `NOTICE` lists JUCE, ASIO, yaml-cpp, SQLite, Catch2
- [ ] `README.md` build instructions are copy-pasteable and consistent with TASK_01 output paths
