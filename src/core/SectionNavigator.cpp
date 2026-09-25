#include "core/SectionNavigator.h"

#include <algorithm>

namespace garageplaymate {

SectionNavigator::SectionNavigator(std::vector<Section> sections) : sections_(std::move(sections)) {
    std::stable_sort(sections_.begin(), sections_.end(),
                     [](const Section& left, const Section& right) { return left.startMs < right.startMs; });
}

const std::vector<Section>& SectionNavigator::getSections() const {
    return sections_;
}

const Section* SectionNavigator::getCurrentSection(int64_t positionMs) const {
    const int index = currentIndex(positionMs);
    return index >= 0 ? &sections_[static_cast<size_t>(index)] : nullptr;
}

std::optional<Section> SectionNavigator::getNextSection(int64_t positionMs) const {
    const auto next = std::upper_bound(sections_.begin(), sections_.end(), positionMs,
                                       [](int64_t position, const Section& section) { return position < section.startMs; });
    if (next == sections_.end()) {
        return std::nullopt;
    }
    return *next;
}

std::optional<Section> SectionNavigator::getPreviousSection(int64_t positionMs) const {
    const int index = currentIndex(positionMs);
    if (index <= 0) {
        return std::nullopt;
    }
    const int64_t currentStart = sections_[static_cast<size_t>(index)].startMs;
    for (int candidate = index - 1; candidate >= 0; --candidate) {
        if (sections_[static_cast<size_t>(candidate)].startMs < currentStart) {
            return sections_[static_cast<size_t>(candidate)];
        }
    }
    return std::nullopt;
}

std::optional<int64_t> SectionNavigator::getStartMsForSection(const std::string& sectionId) const {
    for (const Section& section : sections_) {
        if (section.id == sectionId) {
            return section.startMs;
        }
    }
    return std::nullopt;
}

int64_t SectionNavigator::getNextSectionStartMs(int64_t positionMs, int64_t sessionDurationMs) const {
    const std::optional<Section> next = getNextSection(positionMs);
    if (next.has_value() && next.value().startMs < sessionDurationMs) {
        return next.value().startMs;
    }
    return sessionDurationMs;
}

bool SectionNavigator::isInLastSection(int64_t positionMs, int64_t sessionDurationMs) const {
    return getNextSectionStartMs(positionMs, sessionDurationMs) >= sessionDurationMs;
}

int SectionNavigator::currentIndex(int64_t positionMs) const {
    if (sections_.empty()) {
        return -1;
    }
    const auto after = std::upper_bound(sections_.begin(), sections_.end(), positionMs,
                                        [](int64_t position, const Section& section) { return position < section.startMs; });
    if (after == sections_.begin()) {
        return 0;
    }
    return static_cast<int>(std::distance(sections_.begin(), after)) - 1;
}

}  // namespace garageplaymate
