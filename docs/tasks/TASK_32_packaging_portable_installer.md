# TASK-32: Packaging (Portable + Installer)

**Epic:** E8 — Polish and packaging  
**Depends on:** TASK_01, TASK_02, TASK_30  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Produce portable ZIP and Windows installer packages with correct default data paths and bundled LICENSE/NOTICE.

## Requirements (from PDD)

- NFR-DIST-01 — portable ZIP + Windows installer
- §6.7 — portable `./data/` vs installed `Documents/GaragePlaymate/`
- Acceptance test #11

## Scope

### Create/modify

- `packaging/portable/build_portable.ps1` — copy Release exe, LICENSE, NOTICE, empty `data/songs/` skeleton
- `packaging/installer/GaragePlaymate.nsi` or WiX `.wxs` — NSIS recommended for simplicity
- `CMakeLists.txt` — install rules optional
- `README.md` — distribution section

### Do NOT

- Code signing (document as optional follow-up)
- macOS/Linux packages

## Implementation notes

1. **Portable ZIP:**
   - Build with `-DGARAGEPLAYMATE_PORTABLE_BUILD=ON`
   - Script outputs `GaragePlaymate-portable-win64.zip`:
     ```
     GaragePlaymate.exe
     LICENSE
     NOTICE
     data/
       songs/   (empty or include demo-song optional)
     ```

2. **Installer (NSIS):**
   - Build with `GARAGEPLAYMATE_PORTABLE_BUILD=OFF`
   - Install to `$LOCALAPPDATA\Programs\GaragePlaymate` or `$ProgramFiles`
   - First-run creates `%USERPROFILE%\Documents\GaragePlaymate\` if missing
   - Start menu shortcut
   - Include GPL license page

3. Version string: `0.1.0` from CMake `project(... VERSION 0.1.0)`

4. Document manual build steps in README if CI not set up.

5. Both packages share same Release binary; only compile flag differs for default path.

## Verification

- [ ] Portable ZIP extracts and exe launches
- [ ] Portable exe defaults to `./data/` for songs (verify via log or settings display)
- [ ] Installer installs and exe defaults to Documents path
- [ ] LICENSE and NOTICE included in both packages
