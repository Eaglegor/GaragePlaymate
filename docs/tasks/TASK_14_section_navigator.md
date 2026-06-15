# TASK-14: Section Navigator

**Epic:** E4 — Section navigation  
**Depends on:** TASK_04  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Implement pure `SectionNavigator` core logic for current/next/previous section and seek targets — no JUCE, fully unit tested.

## Requirements (from PDD)

- User story: section names on timeline, jump to section
- FR-PLAY-03 — seek to section start
- FR-FAIL-04 — next section boundary for failure restore

## Scope

### Create/modify

- `src/core/SectionNavigator.h` / `SectionNavigator.cpp`
- `tests/core/test_section_navigator.cpp`

### Do NOT

- Wire to transport or UI (TASK_15, TASK_25)
- Parse YAML (uses `Song.sections` vector)

## Implementation notes

1. Class or namespace functions:
   ```cpp
   class SectionNavigator {
   public:
       explicit SectionNavigator(const std::vector<Section>& sections);

       const Section* getCurrentSection(int64_t positionMs) const;
       std::optional<Section> getNextSection(int64_t positionMs) const;
       std::optional<Section> getPreviousSection(int64_t positionMs) const;
       std::optional<int64_t> getStartMsForSection(const std::string& sectionId) const;
       int64_t getNextSectionStartMs(int64_t positionMs) const;  // for failure restore; session end if last
       bool isInLastSection(int64_t positionMs, int64_t sessionDurationMs) const;
   };
   ```

2. `getCurrentSection`: largest `startMs <= positionMs`; if before first section start, return first section.

3. `getNextSection`: first section with `startMs > positionMs`.

4. `getPreviousSection`: largest section with `startMs < currentSection.startMs`.

5. Empty sections vector: return nullopt / nullptr consistently; document behavior.

6. Unit tests:
   - 3 sections at 0, 15300, 45200 ms
   - position 20000 → verse1; next → chorus1
   - position 50000 → chorus1; isInLastSection true when session ends at 180000

## Verification

- [ ] All unit tests pass
- [ ] No JUCE includes in SectionNavigator
- [ ] Linked by core test target
