# TASK-26: Session Bar UI

**Epic:** E7 — Application services and UI  
**Depends on:** TASK_20, TASK_25  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Build persistent `SessionBar` showing now playing, current section highlight, elapsed/duration time, and non-intrusive failure indicator.

## Requirements (from PDD)

- UI outline §8 — Session bar
- User story: failure indicator for post-run reflection
- FR-FAIL indicator (non-intrusive)

## Scope

### Create/modify

- `src/ui/SessionBar.h` / `SessionBar.cpp`
- Attach to bottom of `MainWindow` (always visible during playback)

### Do NOT

- History replay UI (TASK_27)

## Implementation notes

1. Layout (horizontal bar ~40px height):
   - Now playing: `"{title}"` or empty when idle
   - Current section name from `SectionNavigator` + position
   - Time: `mm:ss / mm:ss` elapsed and session duration
   - Failure indicator: small icon or amber dot + tooltip "Simulated technical problem" when `failureCurrentlyActive` or `failureOccurredThisSession`

2. Update from `PlaybackService::onStateChanged`.

3. Idle state: muted text "No song playing".

4. Use monospaced font for time display optional.

## Verification

- [ ] During playback, title and time update
- [ ] Section name matches position
- [ ] Failure at 100% probability shows indicator during drop
- [ ] Bar hidden or minimal when app idle (design choice: always show at bottom)
