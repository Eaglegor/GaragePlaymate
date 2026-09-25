#pragma once

#include "SongTypes.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace garageplaymate {

// Pure section lookups on a song timeline (milliseconds). Sections are kept
// sorted by startMs; the last section extends to the end of the session.
//
// With no sections the song behaves as one unnamed section: getCurrentSection()
// returns nullptr, next/previous return nullopt, and isInLastSection() is true.
class SectionNavigator {
public:
    SectionNavigator() = default;
    explicit SectionNavigator(std::vector<Section> sections);

    const std::vector<Section>& getSections() const;

    // Largest startMs <= positionMs; the first section if positionMs is before it.
    const Section* getCurrentSection(int64_t positionMs) const;
    // First section with startMs > positionMs.
    std::optional<Section> getNextSection(int64_t positionMs) const;
    // Section before the current one.
    std::optional<Section> getPreviousSection(int64_t positionMs) const;
    std::optional<int64_t> getStartMsForSection(const std::string& sectionId) const;
    // Next section boundary for failure restore; sessionDurationMs if in the last section.
    int64_t getNextSectionStartMs(int64_t positionMs, int64_t sessionDurationMs) const;
    bool isInLastSection(int64_t positionMs, int64_t sessionDurationMs) const;

private:
    // Index of the current section, or -1 when there are no sections.
    int currentIndex(int64_t positionMs) const;

    std::vector<Section> sections_;
};

}  // namespace garageplaymate
