# TASK-27: History Panel UI

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_17, TASK_25  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build `HistoryPanel` listing recent sessions for the current song with human-readable take labels and Replay action.

## Requirements (from PDD)

- FR-HIST-01, FR-HIST-02, FR-HIST-03
- UI outline §8 — History panel

## Scope

### Create/modify

- `src/ui/HistoryPanel.h` / `HistoryPanel.cpp`
- Integrate as tab, drawer, or side panel in song detail view

### Do NOT

- Export session reports (v1.1+)

## Implementation notes

1. When song opens, load `SessionRepository::listSessions(songId, 100)`.

2. List row content:
   - Timestamp formatted local time
   - Take map summary: `"drums: take-02, bass: take-01, ..."` using `formatTakeLabel`
   - Failure badge if session has failure events
   - **Replay** button → `PlaybackService::play(songId, sessionId)`

3. Refresh history after each playback ends (listen to PlaybackService end callback).

4. Empty state: "No sessions yet. Press Play to record your first run."

5. Scrollable list, newest first.

## Verification

- [ ] After play/stop, new session appears in list
- [ ] Replay produces same takes (acceptance test #4)
- [ ] 100+ sessions → list shows 100 (oldest pruned in DB)
- [ ] Take labels readable
