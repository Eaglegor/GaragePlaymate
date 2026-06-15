# TASK-20: Failure Audio Integration

**Epic:** E6 — Failure simulation  
**Depends on:** TASK_18, TASK_19  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Integrate `FailureSimulator` into playback pipeline so gain automation runs on the audio/mixer path and failure events persist in session history.

## Requirements (from PDD)

- FR-FAIL-01 through FR-FAIL-06
- Acceptance test #5 — 100% probability → track drops before next section, restores at boundary

## Scope

### Create/modify

- `src/audio/MultitrackMixerEngine.cpp` — optional per-track gain override callback
- `src/services/PlaybackService.cpp` — schedule failure on play; pass event to mixer; save in session

### Do NOT

- Build failure UI indicator (TASK_26) — expose `PlaybackState.failureActive` bool

## Implementation notes

1. On new play (not replay unless failure was part of original session — **replay should reproduce failure schedule from stored FailureEvent**):
   - Call `FailureSimulator::maybeSchedule` with settings failure config
   - Store `std::optional<FailureEvent>` in PlaybackService active session

2. Replay mode: load `failureEvents` from stored session; use first event for playback (v1 single failure mode).

3. Mixer integration — add to each TrackSlot:
   ```cpp
   std::function<float(int64_t samplePos, float baseGain)> gainModifier;
   ```
   Or pass `FailureSimulator` + event to mixer; compute per block on audio thread using positionMs from sample counter (**no allocation**).

4. `applyGainMultiplier` called per sample block per affected track only.

5. On session save, append `failureEvents` vector to `PlaybackSession`.

6. `PlaybackState` additions:
   ```cpp
   bool failureOccurredThisSession = false;
   bool failureCurrentlyActive = false;  // between onset and restore
   ```

7. Test: failure probability 1.0, play demo song → verify affected track quieter mid-song (manual or integration test).

## Verification

- [ ] 100% failure probability triggers audible drop on one track
- [ ] Gain restores at next section boundary (compare position to sections in demo fixture)
- [ ] Session in DB contains failure event row
- [ ] Replay session reproduces same failure timing
- [ ] No allocations in mixer callback (code review)
