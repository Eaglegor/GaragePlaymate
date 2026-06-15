# TASK-10: Streaming Track Source

**Epic:** E3 — Audio engine  
**Depends on:** TASK_04, TASK_09  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement `StreamingTrackSource` — disk-streaming WAV playback via JUCE `AudioFormatReader`, conforming to `ITrackAudioSource`.

## Requirements (from PDD)

- FR-PLAY-09 — Stream mode (default): read from disk during playback
- FR-PLAY-01 — start at file position 00:00
- NFR-REL-01 — no allocations on audio thread after prepare

## Scope

### Create/modify

- `src/audio/ITrackAudioSource.h` — interface from ARCHITECTURE §9.2
- `src/audio/StreamingTrackSource.h` / `StreamingTrackSource.cpp`
- Update `CMakeLists.txt`

### Do NOT

- Implement Preload mode (TASK_11)
- Implement mixer (TASK_12)
- Support non-WAV formats

## Implementation notes

1. `ITrackAudioSource` interface — copy from ARCHITECTURE §9.2 exactly.

2. `StreamingTrackSource`:
   - `prepare(path)`: open `juce::AudioFormatReader` via `AudioFormatManager` (register WAV); store reader, length in samples, sample rate, channels; reset read position to 0
   - `getNextAudioBlock(buffer, startSample, numSamples)`: read from reader into buffer; apply linear gain; if read past end, fill silence and set finished flag
   - `setGain(float)`: store atomic or plain float read on audio thread (no lock if single writer message thread / single reader audio thread — document pattern)
   - `seekToSample(pos)`: reposition reader
   - `release()`: destroy reader on message thread

3. Handle mono → stereo upmix (duplicate to L/R) and stereo → mono downmix (average) when output buffer is stereo.

4. Resample: v1 assumes all takes same sample rate as device OR resample in mixer — **document assumption: all WAVs same sample rate as device; mismatch logs warning at prepare time**. Resampling out of scope unless trivial.

5. After `prepare()` completes, audio thread only reads — no `new` in `getNextAudioBlock`.

## Verification

- [ ] Prepare a test WAV → `getLengthSamples()` matches file duration
- [ ] Read full file → `isFinished()` true, output non-silent for non-silent WAV
- [ ] `seekToSample(0)` after partial read restarts from beginning
- [ ] Builds without GUI modules in audio target (if split)
