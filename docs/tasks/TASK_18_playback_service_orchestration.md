# TASK-18: Playback Service Orchestration

**Epic:** E5 — Random takes and session history  
**Depends on:** TASK_12, TASK_13, TASK_15, TASK_16, TASK_17  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `PlaybackService` that orchestrates take selection, preload/stream preparation, transport control, and session persistence including replay mode.

## Requirements (from PDD)

- FR-RAND-01, FR-RAND-02, FR-RAND-03
- FR-PLAY-09 — Stream vs Preload modes
- FR-HIST-02 — replay bypasses random selection
- FR-PLAY-04, FR-PLAY-05 — session length and variable endings

## Scope

### Create/modify

- `src/services/PlaybackService.h` / `PlaybackService.cpp`
- Wire into `AppController` minimally (optional callback registration only)

### Do NOT

- Failure simulation (TASK_19–20) — leave hook points: `FailureSimulator*` nullable
- Full UI (TASK_25)

## Implementation notes

1. Dependencies injected via constructor:
   - `AudioDeviceService&`
   - `MultitrackMixerEngine&`
   - `TransportController&`
   - `SessionRepository&`
   - `RandomTakeSelector&`

2. API:
   ```cpp
   enum class PlaybackError { NoSong, NoTakes, PreloadFailed, DeviceError };

   struct PlaybackState {
       const Song* currentSong = nullptr;
       TakeMap currentTakeMap;
       TransportState transport;
       int64_t positionMs = 0;
       int64_t durationMs = 0;
       bool isPreloading = false;
       PreloadProgress preloadProgress;
   };

   class PlaybackService {
   public:
       void setSongLibrary(const std::vector<Song>* songs);  // pointer to LibraryService cache
       void setSettings(const AppSettings& settings);
       void setDisabledTakes(const std::set<DisabledTakeKey>& disabled);

       bool play(const std::string& songId, std::optional<int64_t> replaySessionId = {});
       void pause();
       void stop();

       PlaybackState getState() const;
       std::function<void(const PlaybackState&)> onStateChanged;
       std::function<void(PlaybackError, const std::string& detail)> onError;
   };
   ```

3. `play()` flow:
   - Find song by id
   - If `replaySessionId` → load take map from SessionRepository; set `isReplay=true`
   - Else → `RandomTakeSelector::selectTakes`; `isReplay=false`
   - Build track sources (Streaming or Preloaded per `settings.loadingMode`)
   - Preload: async on message thread with progress updates; disable concurrent play
   - Bind mixer, set per-track default volumes from manifest
   - `transport.play()` from sample 0
   - On end/stop → save session (except if user cancelled preload)

4. `stop()` must release preloaded memory (FR-PLAY-09 release on Stop).

5. Changing takes requires stop then play (document in header).

6. Leave `FailureSimulator` integration as TODO comment for TASK_20.

## Verification

- [ ] Play song with 2 tracks → audio output (manual with demo fixtures)
- [ ] Second play with random selector → may differ (probabilistic)
- [ ] Replay session id → identical take filenames
- [ ] Preload mode shows progress via `onStateChanged`
- [ ] Stop releases sources without leak (repeat play 10×)
