# TASK-23: Settings Service

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_07, TASK_08  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Load and persist `AppSettings` to SQLite, resolve effective data root, and notify listeners when settings change.

## Requirements (from PDD)

- §6.7 — data path, audio, loading mode, failure defaults
- FR-LIB-05 — configurable data root

## Scope

### Create/modify

- `src/services/SettingsService.h` / `SettingsService.cpp`
- Use nlohmann/json or minimal hand-rolled JSON for settings blob in SQLite `settings` key `app_settings`

### Do NOT

- Build settings UI (TASK_28)
- Apply audio device changes (caller invokes AudioDeviceService after save)

## Implementation notes

1. API:
   ```cpp
   class SettingsService {
   public:
       SettingsService(SQLiteStore& store, const std::filesystem::path& exePath);

       AppSettings load();
       void save(const AppSettings& settings);

       std::filesystem::path getEffectiveDataRoot() const;
       std::filesystem::path getDefaultDataRoot() const;

       std::function<void(const AppSettings&)> onSettingsChanged;
   private:
       AppSettings cached_;
       std::filesystem::path exePath_;
   };
   ```

2. On first run (no DB key): return default `AppSettings` with WASAPI, Stream mode, failure off.

3. Serialize `AppSettings` to JSON string for SQLite.

4. `getEffectiveDataRoot()` uses `DataPathResolver::getEffectiveDataRoot(cached_.dataRootOverride)`.

5. `save()` writes to DB and updates cache; fires `onSettingsChanged`.

6. Fetch nlohmann/json via CMake FetchContent if not already added.

## Verification

- [ ] Save/load round-trip preserves all fields
- [ ] Override data root returned by `getEffectiveDataRoot`
- [ ] Fresh DB returns sensible defaults matching PDD
