# TASK-12: Multitrack Mixer Engine

**Epic:** E3 — Audio engine  
**Depends on:** TASK_09, TASK_10, TASK_11  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build `MultitrackMixerEngine` that mixes N track sources with sample-accurate simultaneous start, per-track gain, and silence after individual tracks end.

## Requirements (from PDD)

- FR-PLAY-01 — simultaneous start at 00:00
- FR-PLAY-02 — per-track gain 0.0–1.0 linear
- FR-PLAY-04 — session length = max take duration
- FR-PLAY-05 — shorter tracks silent when finished
- NFR-PERF-02 — target <15% CPU for 8 tracks (benchmark note)
- NFR-REL-01 — no audio thread allocations

## Scope

### Create/modify

- `src/audio/MultitrackMixerEngine.h` / `MultitrackMixerEngine.cpp`

### Do NOT

- Transport state machine (TASK_13)
- Failure gain automation (TASK_20)
- Random take selection

## Implementation notes

1. Implement `juce::AudioIODeviceCallback`:
   ```cpp
   class MultitrackMixerEngine : public juce::AudioIODeviceCallback {
   public:
       struct TrackSlot {
           std::unique_ptr<ITrackAudioSource> source;
           float gain = 1.f;
           std::string trackId;
       };

       void setTracks(std::vector<TrackSlot> tracks);  // message thread
       int64_t getSessionLengthSamples() const;
       void prepareToPlay(double sampleRate, int maxBlockSize);
       void audioDeviceIOCallbackWithContext(...);
   };
   ```

2. `setTracks`: called on message thread before start; compute `sessionLengthSamples = max(source->getLengthSamples())`.

3. Mix loop: for each track, call `getNextAudioBlock` into temp buffer, accumulate with gain into output. Finished tracks contribute silence automatically via source.

4. `prepareToPlay`: store sample rate; optionally verify all sources share same sample rate.

5. Register as callback on `AudioDeviceService`'s device manager when playback starts (coordinate with TransportController in TASK_13).

6. Optional spike: log CPU time in debug builds for 8 silent WAVs.

7. **Critical:** `setTracks` replaces vector — do on message thread while audio stopped.

## Verification

- [ ] 2 tracks different lengths → mixer runs until longer ends
- [ ] Shorter track produces silence in second half (verify with test WAVs)
- [ ] All tracks start at sample 0 (no stagger)
- [ ] 8-track test plays without glitches on default buffer size
