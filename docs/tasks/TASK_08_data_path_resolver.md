# TASK-08: Data Path Resolver

**Epic:** E2 — Song library and persistence foundation  
**Depends on:** TASK_07  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `DataPathResolver` to compute default and effective data root paths for portable vs installed builds, plus database and songs directory paths.

## Requirements (from PDD)

- FR-LIB-05 — user-configurable data root
- §6.7 — portable `{exe_dir}/data/` vs installed `Documents/GaragePlaymate/`
- Acceptance tests #11

## Scope

### Create/modify

- `src/storage/DataPathResolver.h` / `DataPathResolver.cpp`
- `tests/core/test_data_path_resolver.cpp`
- Update `CMakeLists.txt` if needed

### Do NOT

- Persist user override (TASK_23 SettingsService) — resolver accepts override as parameter
- Create directories on disk (caller may ensure exists)

## Implementation notes

1. API:
   ```cpp
   class DataPathResolver {
   public:
       static std::filesystem::path getDefaultDataRoot();  // uses GARAGEPLAYMATE_PORTABLE_BUILD compile flag
       static std::filesystem::path getEffectiveDataRoot(
           const std::optional<std::filesystem::path>& userOverride);
       static std::filesystem::path getSongsDirectory(
           const std::filesystem::path& effectiveDataRoot);
       static std::filesystem::path getDatabasePath(
           const std::filesystem::path& effectiveDataRoot);
   };
   ```

2. Portable build (`#ifdef GARAGEPLAYMATE_PORTABLE_BUILD`):
   - Default root = directory containing executable + `"data"`
   - Use JUCE `juce::File::getSpecialLocation(juce::File::currentExecutableFile)` or Win32 `GetModuleFileNameW` if avoiding JUCE in storage layer — **prefer minimal Win32 in storage** to keep layer clean; alternatively accept exe path as constructor argument from app layer.

3. Installed build:
   - Default root = `%USERPROFILE%/Documents/GaragePlaymate/`
   - Use `SHGetKnownFolderPath(FOLDERID_Documents)` or JUCE passed from app

4. **Recommended:** `DataPathResolver` takes `std::filesystem::path exePath` in static methods from `Application` layer to avoid JUCE in storage:
   ```cpp
   static std::filesystem::path getDefaultDataRoot(const std::filesystem::path& exePath);
   ```

5. `getSongsDirectory` → `{dataRoot}/songs`
6. `getDatabasePath` → `{dataRoot}/garageplaymate.db`

7. Unit tests: mock portable flag behavior via test-only parameter or compile test target twice; at minimum test `getEffectiveDataRoot` prefers override over default.

## Verification

- [ ] Override path wins over default
- [ ] Songs and DB paths derive correctly from effective root
- [ ] Unit tests pass
- [ ] Document in code comment which thread calls these (any)
