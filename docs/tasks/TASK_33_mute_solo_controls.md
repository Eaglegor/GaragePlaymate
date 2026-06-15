# TASK-33: Mute/Solo Controls (Optional)

**Epic:** E8 — Polish (nice-to-have)  
**Depends on:** TASK_12, TASK_25  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Add per-track mute and solo controls if implementation cost is low; otherwise skip this task.

## Requirements (from PDD)

- FR-PLAY-11 (nice-to-have) — mute/solo if trivial atop mixer gains
- §15 decision #4 — not required for v1 release

## Scope

### Create/modify

- `src/audio/MultitrackMixerEngine` — mute flag per track; solo logic (solo mutes others)
- `src/ui/SongDetailPanel` — M and S buttons per track row
- `src/services/PlaybackService` — `setTrackMuted`, `setTrackSoloed` API

### Do NOT

- Implement if requires major mixer refactor — document skip reason in PR/commit message

## Implementation notes

1. Solo logic (standard DAW):
   - If any track soloed, only soloed tracks audible (respect mute on soloed track)
   - If none soloed, mute flags apply normally

2. Store mute/solo in mixer TrackSlot as bools; effective gain = baseVolume * mute * soloRules.

3. UI: small toggle buttons next to volume slider; keyboard shortcuts out of scope.

4. State resets on Stop (optional) or persists during session — prefer persist during session, reset on new Play.

5. **Skip criterion:** If TASK_12 mixer API cannot accept dynamic mute/solo without audio glitches, defer to v1.1.

## Verification

- [ ] Mute silences track during playback
- [ ] Solo silences all non-solo tracks
- [ ] Multiple solos → only those tracks heard
- [ ] Volume slider still works when un-muted
