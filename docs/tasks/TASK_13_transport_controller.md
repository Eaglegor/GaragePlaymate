# TASK-13: Transport Controller

**Epic:** E3 — Audio engine  
**Depends on:** TASK_12  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `TransportController` managing Play/Pause/Stop/Seek state, session duration, and position callbacks for UI.

## Requirements (from PDD)

- FR-PLAY-03 — Play, Pause, Stop, Seek to section start
- FR-PLAY-04 — session duration = max selected take duration

## Scope

### Create/modify

- `src/audio/TransportController.h` / `TransportController.cpp`

### Do NOT

- Section navigation logic (TASK_14–15)
- Session persistence (TASK_17)
- UI bindings (TASK_25)

## Implementation notes

1. States: `Stopped`, `Playing`, `Paused`.

2. API:
   ```cpp
   enum class TransportState { Stopped, Playing, Paused };

   class TransportController : public juce::Timer {
   public:
       void attach(MultitrackMixerEngine& engine, AudioDeviceService& devices);

       TransportState getState() const;
       int64_t getPositionSamples() const;
       int64_t getDurationSamples() const;
       double getSampleRate() const;

       void play();    // start device if needed, reset to 0 if stopped
       void pause();
       void stop();    // stop device, reset position to 0, release track sources via callback
       void seekToSample(int64_t samplePos);

       std::function<void(TransportState)> onStateChanged;
       std::function<void(int64_t positionSamples, int64_t durationSamples)> onPositionChanged;
       std::function<void()> onReachedEnd;
   };
   ```

3. Position tracking: maintain atomic `currentSample` updated in audio callback (mixer notifies via lightweight listener or mixer exposes `getPlayhead()`).

4. `onReachedEnd`: fire when position >= duration; auto transition to Stopped.

5. Timer at 30 Hz on message thread poll position → `onPositionChanged` (avoid blocking audio thread).

6. `play()` from Stopped: ensure mixer tracks prepared; start audio device; `play()` from Paused: resume.

7. `stop()`: stop audio device; invoke optional `onStop` for source cleanup.

## Verification

- [ ] Play → position advances; Pause → position holds; Stop → position 0
- [ ] Seek mid-playback jumps all track sources via `seekToSample`
- [ ] `onReachedEnd` fires at max take length
- [ ] State callbacks fire on message thread
