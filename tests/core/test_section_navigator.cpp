#include <catch2/catch_test_macros.hpp>

#include "core/SectionNavigator.h"

using garageplaymate::Section;
using garageplaymate::SectionNavigator;

namespace {

SectionNavigator threeSections() {
    // Deliberately unsorted: the navigator sorts by startMs.
    return SectionNavigator({
        Section{"chorus1", "Chorus", 45200},
        Section{"intro", "Intro", 0},
        Section{"verse1", "Verse 1", 15300},
    });
}

}  // namespace

TEST_CASE("SectionNavigator finds the current section", "[section_navigator]") {
    const SectionNavigator navigator = threeSections();
    CHECK(navigator.getCurrentSection(0)->id == "intro");
    CHECK(navigator.getCurrentSection(15299)->id == "intro");
    CHECK(navigator.getCurrentSection(15300)->id == "verse1");
    CHECK(navigator.getCurrentSection(20000)->id == "verse1");
    CHECK(navigator.getCurrentSection(50000)->id == "chorus1");
}

TEST_CASE("SectionNavigator returns the first section before it starts", "[section_navigator]") {
    const SectionNavigator navigator({Section{"verse", "Verse", 2000}, Section{"chorus", "Chorus", 8000}});
    CHECK(navigator.getCurrentSection(500)->id == "verse");
    CHECK(navigator.getNextSection(500).value().id == "verse");
    CHECK_FALSE(navigator.getPreviousSection(500).has_value());
}

TEST_CASE("SectionNavigator next and previous sections", "[section_navigator]") {
    const SectionNavigator navigator = threeSections();
    CHECK(navigator.getNextSection(20000).value().id == "chorus1");
    CHECK(navigator.getNextSection(0).value().id == "verse1");
    CHECK_FALSE(navigator.getNextSection(50000).has_value());

    CHECK(navigator.getPreviousSection(20000).value().id == "intro");
    CHECK(navigator.getPreviousSection(50000).value().id == "verse1");
    CHECK_FALSE(navigator.getPreviousSection(1000).has_value());
}

TEST_CASE("SectionNavigator looks up sections by id", "[section_navigator]") {
    const SectionNavigator navigator = threeSections();
    CHECK(navigator.getStartMsForSection("verse1").value() == 15300);
    CHECK_FALSE(navigator.getStartMsForSection("bridge").has_value());
}

TEST_CASE("SectionNavigator next boundary and last section", "[section_navigator]") {
    const SectionNavigator navigator = threeSections();
    constexpr int64_t kDuration = 180000;
    CHECK(navigator.getNextSectionStartMs(20000, kDuration) == 45200);
    CHECK(navigator.getNextSectionStartMs(50000, kDuration) == kDuration);
    CHECK_FALSE(navigator.isInLastSection(20000, kDuration));
    CHECK(navigator.isInLastSection(50000, kDuration));

    // A section starting after the session ends is never reached.
    CHECK(navigator.getNextSectionStartMs(20000, 40000) == 40000);
    CHECK(navigator.isInLastSection(20000, 40000));
}

TEST_CASE("SectionNavigator without sections", "[section_navigator]") {
    const SectionNavigator navigator;
    CHECK(navigator.getCurrentSection(1000) == nullptr);
    CHECK_FALSE(navigator.getNextSection(1000).has_value());
    CHECK_FALSE(navigator.getPreviousSection(1000).has_value());
    CHECK(navigator.getNextSectionStartMs(1000, 60000) == 60000);
    CHECK(navigator.isInLastSection(1000, 60000));
}
