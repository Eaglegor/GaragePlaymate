# TASK-19: Failure Simulator Core

**Epic:** E6 — Failure simulation  
**Depends on:** TASK_14  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement pure `FailureSimulator` domain logic: Bernoulli trial, track/time selection, fade/restore schedule tied to section boundaries.

## Requirements (from PDD)

- FR-FAIL-01 — enable + probability per play
- FR-FAIL-02 — random track and onset time in playable window
- FR-FAIL-03 — ramp to failureLevel over fadeMs
- FR-FAIL-04 — restore at next section boundary
- FR-FAIL-05 — no failure in last section
- FR-FAIL-06 — log event in session (persistence in TASK_17/18)

## Scope

### Create/modify

- `src/core/FailureSimulator.h` / `FailureSimulator.cpp`
- `tests/core/test_failure_simulator.cpp`

### Do NOT

- Apply gain in audio callback (TASK_20)
- UI indicator (TASK_26)

## Implementation notes

1. Constants:
   ```cpp
   constexpr int64_t kFailureExcludeTailMs = 5000;
   ```

2. Config struct:
   ```cpp
   struct FailureConfig {
       bool enabled = false;
       float probability = 0.10f;   // 0..1 Bernoulli
       float failureLevel = 0.15f;  // linear gain 0..0.5
       int fadeMs = 300;
   };
   ```

3. API:
   ```cpp
   class FailureSimulator {
   public:
       std::optional<FailureEvent> maybeSchedule(
           const Song& song,
           const TakeMap& takeMap,
           int64_t sessionDurationMs,
           const FailureConfig& config,
           std::mt19937& rng) const;

       float applyGainMultiplier(
           const FailureEvent& event,
           const std::string& trackId,
           int64_t positionMs,
           float baseGain) const;
   };
   ```

4. `maybeSchedule`:
   - Return nullopt if !enabled or Bernoulli fails
   - Use `SectionNavigator` — if only one section or position would be in last section, return nullopt
   - Playable window: from first section end (or 0) to `sessionDurationMs - kFailureExcludeTailMs`, excluding last section time range
   - Pick random track from takeMap keys
   - Pick random onsetMs in window
   - `restoreMs` = next section start after onset (SectionNavigator.getNextSectionStartMs)

5. `applyGainMultiplier`:
   - Before onset: return baseGain
   - Fade down over fadeMs to `failureLevel * baseGain`
   - Hold until restoreMs
   - Fade back to baseGain over fadeMs

6. Unit tests with fixed RNG:
   - probability 0 → never schedules
   - probability 1, multi-section song → event with valid restoreMs > onsetMs
   - last section only song → nullopt

## Verification

- [ ] All unit tests pass
- [ ] No JUCE includes
- [ ] applyGain at onset, mid-failure, after restore returns expected multipliers
