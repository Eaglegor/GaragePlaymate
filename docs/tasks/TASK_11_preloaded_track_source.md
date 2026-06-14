# TASK-11: Preloaded Track Source

**Epic:** E3 — Audio engine  
**Depends on:** TASK_10  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `PreloadedTrackSource` that loads entire WAV into memory on the message thread with progress reporting, then plays back in a real-time-safe manner.

## Requirements (from PDD)

- FR-PLAY-09 — Preload into RAM mode
- FR-PLAY-10 — allocation on loader thread before audio starts
- NFR-PERF-03 — preload selected takes only, release on Stop
- NFR-PERF-04 — show progress during preload

## Scope

### Create/modify

- `src/audio/PreloadedTrackSource.h` / `PreloadedTrackSource.cpp`

### Do NOT

- Preload entire library — only per-session selected takes
- Allocate on audio thread

## Implementation notes

1. `PreloadedTrackSource` implements `ITrackAudioSource`.

2. `prepare(path, progressCallback)`:
   - `progressCallback(float fraction0to1)` called on message thread during load
   - Use `AudioFormatReader::read()` in chunks (e.g. 8192 samples) to fill `juce::AudioBuffer<float> audioBuffer`
   - Store `readPosition` atomic int64 for audio thread
   - Return false on read failure

3. `getNextAudioBlock`: copy from internal buffer at `readPosition`; advance position; apply gain; mark finished at end

4. `release()`: free buffer on message thread only (call before destroying or when stopping transport)

5. `estimateMemoryBytes(numChannels, numSamples)`: static helper for UI memory estimate (ARCHITECTURE risk note)

6. Progress API for PlaybackService:
   ```cpp
   struct PreloadProgress {
       int tracksCompleted = 0;
       int tracksTotal = 0;
       float currentTrackFraction = 0.f;
   };
   ```

## Verification

- [ ] Load 10-second WAV → buffer length matches; playback matches stream source output
- [ ] Progress callback fires during load
- [ ] After `release()`, no crash on subsequent prepare
- [ ] No heap allocation in `getNextAudioBlock` (code review / comment assertion)
