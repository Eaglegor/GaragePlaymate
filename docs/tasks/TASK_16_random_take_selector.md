# TASK-16: Random Take Selector

**Epic:** E5 — Random takes and session history  
**Depends on:** TASK_04  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement uniform random take selection per track, excluding user-disabled takes, with injectable RNG for tests.

## Requirements (from PDD)

- FR-RAND-01 — uniform random from non-disabled takes on each Play from stopped
- FR-RAND-02 — no mid-song hot-swap (enforced by PlaybackService in TASK_18)
- User story: exclude specific takes from random pool

## Scope

### Create/modify

- `src/core/RandomTakeSelector.h` / `RandomTakeSelector.cpp`
- `tests/core/test_random_take_selector.cpp`

### Do NOT

- Persist disabled takes (TASK_07 SQLiteStore already has API)
- Select takes during replay (TASK_18)

## Implementation notes

1. API:
   ```cpp
   struct DisabledTakeKey {
       std::string songId;
       std::string trackId;
       std::string takeFilename;
       bool operator<(const DisabledTakeKey& o) const;
   };

   using TakeMap = std::map<std::string, std::string>;  // trackId → takeFilename

   class RandomTakeSelector {
   public:
       explicit RandomTakeSelector(std::mt19937* rngForTests = nullptr);

       std::expected<TakeMap, std::string> selectTakes(
           const Song& song,
           const std::set<DisabledTakeKey>& disabled) const;
   };
   ```

2. For each track, build pool = takes not in disabled set for `(song.id, track.id, filename)`.

3. Empty pool → return error `"No available takes for track {id}"`.

4. Uniform index: `std::uniform_int_distribution<size_t>(0, pool.size()-1)`.

5. Use injected RNG in tests with fixed seed → deterministic selection.

6. Unit tests:
   - 3 takes, none disabled → always picks one of three
   - Fixed seed → exact filename
   - All takes disabled → error
   - One take disabled → never selected (run 100 iterations with seed sweep)

## Verification

- [ ] Unit tests pass
- [ ] No JUCE dependency
- [ ] Each track gets exactly one take in result map
