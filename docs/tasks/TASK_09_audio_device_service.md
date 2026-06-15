# TASK-09: Audio Device Service

**Epic:** E3 — Audio engine  
**Depends on:** TASK_01, TASK_03  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Wrap `juce::AudioDeviceManager` in `AudioDeviceService` with WASAPI default on first launch, optional ASIO, device listing, and graceful ASIO→WASAPI fallback.

## Requirements (from PDD)

- FR-PLAY-06 — WASAPI default, ASIO optional
- FR-PLAY-07 — auto-select system default WASAPI on first launch
- FR-PLAY-08 — ASIO unavailable → fall back to WASAPI with notification
- NFR-UX-01 — zero-config first playback

## Scope

### Create/modify

- `src/audio/AudioDeviceService.h` / `AudioDeviceService.cpp`
- Update `CMakeLists.txt` — add audio sources to app target

### Do NOT

- Implement mixer or track sources (TASK_10–12)
- Wire to Settings UI (TASK_28) — expose API only; accept settings struct from caller
- Open audio device on app launch unless needed for spike — lazy init on first playback is OK

## Implementation notes

1. API:
   ```cpp
   struct AudioDeviceInfo {
       std::string deviceId;
       std::string displayName;
       AudioDriverType driverType;
   };

   class AudioDeviceService {
   public:
       AudioDeviceService();
       juce::AudioDeviceManager& getDeviceManager();

       std::vector<AudioDeviceInfo> listOutputDevices(AudioDriverType type);
       bool applySettings(const AppSettings& settings, juce::String& errorMessage);
       bool initializeDefaultWasapi(juce::String& errorMessage);

       // Callback type for fallback notification
       std::function<void(const juce::String& message)> onFallbackNotification;
   };
   ```

2. `initializeDefaultWasapi()`:
   - Device type: `juce::AudioIODeviceType::Type::wasapi`
   - Use empty device name → JUCE picks default output
   - Buffer size 512, stereo output
   - Call on first playback if no prior settings

3. `applySettings()`:
   - WASAPI: set up WASAPI device type, open `settings.outputDeviceId` or default
   - ASIO (if `GARAGEPLAYMATE_ENABLE_ASIO`): set up ASIO type; on failure call fallback to last WASAPI and invoke `onFallbackNotification`

4. Register audio callback placeholder (empty `AudioIODeviceCallback`) or defer registration to TASK_12 mixer.

5. Thread: all public methods on message thread only.

6. Log device changes via `juce::Logger::writeToLog`.

## Verification

- [ ] On machine without ASIO, `initializeDefaultWasapi()` succeeds
- [ ] `listOutputDevices(Wasapi)` returns at least one device on typical Windows laptop
- [ ] Applying invalid ASIO device triggers fallback callback (manual test or unit test with mock if feasible)
- [ ] Project builds with ASIO enabled and disabled CMake options
