# TASK-15: Section Seek Integration

**Epic:** E4 — Section navigation  
**Depends on:** TASK_13, TASK_14  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Wire `SectionNavigator` into `TransportController` and expose section seek/jump API for future UI and failure simulator.

## Requirements (from PDD)

- FR-PLAY-03 — seek to section start
- Keyboard shortcuts Left/Right = prev/next section (wired in TASK_29)

## Scope

### Create/modify

- `src/audio/TransportController.h` / `.cpp` — add section seek methods
- `src/services/PlaybackService.h` — skeleton only if not exists; add section jump methods stub
- OR extend TransportController only if PlaybackService doesn't exist yet

### Do NOT

- Build full PlaybackService orchestration (TASK_18)
- Build section UI strip (TASK_25)

## Implementation notes

1. Add to `TransportController` (or small `SectionTransport` helper):
   ```cpp
   void seekToSectionMs(int64_t startMs);
   void seekToSectionId(const std::string& sectionId, const SectionNavigator& nav);
   void seekToNextSection(const SectionNavigator& nav);
   void seekToPreviousSection(const SectionNavigator& nav);
   ```

2. `seekToSectionMs`: convert ms to samples using `sampleRate`; call `seekToSample`; clamp to `[0, durationSamples]`.

3. Seek while Playing: all track sources seek synchronously (sample-accurate).

4. Seek while Paused: update position without starting playback.

5. If `PlaybackService` stub exists, add:
   ```cpp
   void jumpToNextSection();
   void jumpToPreviousSection();
   void jumpToSection(const std::string& sectionId);
   ```
   holding `SectionNavigator` built from current song.

6. Emit `onPositionChanged` immediately after seek.

## Verification

- [ ] Seek to section 2 start → position matches expected samples
- [ ] Next/previous section from mid-song lands on correct `startMs`
- [ ] Seek during playback does not crash or desync tracks (manual test with 2 WAVs)
