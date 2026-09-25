#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio/FakeAudioOutput.h"
#include "audio/FakeTrackSource.h"
#include "audio/TransportController.h"

#include <vector>

using Catch::Matchers::WithinAbs;
using garageplaymate::MultitrackMixerEngine;
using garageplaymate::TransportController;
using garageplaymate::TransportState;
using garageplaymate::test::constantSource;
using garageplaymate::test::FakeAudioOutput;

namespace {

struct TransportFixture {
    FakeAudioOutput output{48000.0, 256};
    MultitrackMixerEngine engine;
    TransportController transport;
    std::vector<TransportState> states;
    int64_t lastPosition = -1;
    int reachedEndCount = 0;
    int stopCount = 0;

    explicit TransportFixture(std::vector<int64_t> lengths = {4800, 9600}) {
        std::vector<MultitrackMixerEngine::TrackSlot> tracks;
        int index = 0;
        for (int64_t length : lengths) {
            tracks.push_back({constantSource(length, 0.25f), 1.0f, "track-" + std::to_string(index++)});
        }
        engine.setTracks(std::move(tracks));
        transport.attach(engine, output);
        transport.onStateChanged = [this](TransportState state) { states.push_back(state); };
        transport.onPositionChanged = [this](int64_t position, int64_t) { lastPosition = position; };
        transport.onReachedEnd = [this] { ++reachedEndCount; };
        transport.onStop = [this] { ++stopCount; };
    }
};

}  // namespace

TEST_CASE("Transport play advances position, pause holds, stop rewinds", "[transport]") {
    TransportFixture fixture;
    auto& transport = fixture.transport;
    CHECK(transport.getState() == TransportState::Stopped);

    REQUIRE(transport.play());
    CHECK(transport.getState() == TransportState::Playing);
    CHECK(fixture.output.hasCallback());
    CHECK(transport.getDurationSamples() == 9600);
    CHECK(transport.getDurationMs() == 200);

    const auto& block = fixture.output.pump(4);
    CHECK_THAT(block.getSample(0, 0), WithinAbs(0.5, 1e-6));
    transport.update();
    CHECK(transport.getPositionSamples() == 1024);
    CHECK(fixture.lastPosition == 1024);

    transport.pause();
    CHECK(transport.getState() == TransportState::Paused);
    fixture.output.pump(4);
    transport.update();
    CHECK(transport.getPositionSamples() == 1024);

    REQUIRE(transport.play());
    fixture.output.pump(2);
    CHECK(transport.getPositionSamples() == 1536);

    transport.stop();
    CHECK(transport.getState() == TransportState::Stopped);
    CHECK(transport.getPositionSamples() == 0);
    CHECK(fixture.lastPosition == 0);
    CHECK_FALSE(fixture.output.hasCallback());
    CHECK(fixture.stopCount == 1);
    CHECK(fixture.states == std::vector<TransportState>{TransportState::Playing, TransportState::Paused,
                                                         TransportState::Playing, TransportState::Stopped});
}

TEST_CASE("Transport play from stopped restarts at zero", "[transport]") {
    TransportFixture fixture;
    auto& transport = fixture.transport;
    REQUIRE(transport.play());
    fixture.output.pump(3);
    transport.stop();

    REQUIRE(transport.play());
    CHECK(transport.getPositionSamples() == 0);
    fixture.output.pump(1);
    CHECK(transport.getPositionSamples() == 256);
}

TEST_CASE("Transport seek moves all tracks while playing or paused", "[transport]") {
    TransportFixture fixture({9600, 9600});
    auto& transport = fixture.transport;

    transport.seekToSample(5000);  // ignored while stopped
    CHECK(transport.getPositionSamples() == 0);

    REQUIRE(transport.play());
    fixture.output.pump(1);
    transport.seekToSample(4800);
    CHECK(fixture.lastPosition == 4800);
    fixture.output.pump(1);
    CHECK(transport.getPositionSamples() == 5056);

    transport.pause();
    transport.seekToSample(100);
    CHECK(transport.getPositionSamples() == 100);
    CHECK(transport.getState() == TransportState::Paused);

    transport.seekToSample(1'000'000);
    CHECK(transport.getPositionSamples() == 9600);
}

TEST_CASE("Transport stops and reports the end of the longest take", "[transport]") {
    TransportFixture fixture({1000, 2000});
    auto& transport = fixture.transport;
    REQUIRE(transport.play());

    fixture.output.pump(7);
    transport.update();
    CHECK(fixture.reachedEndCount == 0);
    CHECK(transport.getState() == TransportState::Playing);

    fixture.output.pump(1);  // 2048 samples rendered
    transport.update();
    CHECK(fixture.reachedEndCount == 1);
    CHECK(fixture.stopCount == 1);
    CHECK(transport.getState() == TransportState::Stopped);
    CHECK(transport.getPositionSamples() == 0);
}

TEST_CASE("Transport play fails cleanly without a device or tracks", "[transport]") {
    SECTION("device cannot open") {
        TransportFixture fixture;
        fixture.output.failToOpen = true;
        juce::String error;
        CHECK_FALSE(fixture.transport.play(&error));
        CHECK(error.isNotEmpty());
        CHECK(fixture.transport.getState() == TransportState::Stopped);
    }

    SECTION("no tracks loaded") {
        TransportFixture fixture({});
        juce::String error;
        CHECK_FALSE(fixture.transport.play(&error));
        CHECK(error == "No tracks to play");
    }

    SECTION("not attached") {
        TransportController transport;
        CHECK_FALSE(transport.play());
        transport.pause();
        transport.stop();
        CHECK(transport.getState() == TransportState::Stopped);
    }
}

namespace {

// 10 s session at 48 kHz with sections at 0, 3 s and 6 s.
struct SectionFixture : TransportFixture {
    garageplaymate::SectionNavigator navigator{{
        garageplaymate::Section{"intro", "Intro", 0},
        garageplaymate::Section{"verse", "Verse", 3000},
        garageplaymate::Section{"chorus", "Chorus", 6000},
    }};

    SectionFixture() : TransportFixture({480000, 480000}) {}
};

}  // namespace

TEST_CASE("Transport seeks to a section start in samples", "[transport][sections]") {
    SectionFixture fixture;
    auto& transport = fixture.transport;
    REQUIRE(transport.play());

    CHECK(transport.seekToSectionId("verse", fixture.navigator));
    CHECK(transport.getPositionSamples() == 144000);
    CHECK(fixture.lastPosition == 144000);
    CHECK_FALSE(transport.seekToSectionId("bridge", fixture.navigator));

    fixture.output.pump(1);
    CHECK(transport.getPositionSamples() == 144256);
}

TEST_CASE("Transport next and previous section navigation", "[transport][sections]") {
    SectionFixture fixture;
    auto& transport = fixture.transport;
    REQUIRE(transport.play());

    transport.seekToSample(48000 * 4);  // 4 s, in verse
    transport.seekToNextSection(fixture.navigator);
    CHECK(transport.getPositionMs() == 6000);

    transport.seekToNextSection(fixture.navigator);  // already in last section
    CHECK(transport.getPositionMs() == 6000);

    transport.seekToSample(48000 * 7);  // 1 s into chorus → previous section
    transport.seekToPreviousSection(fixture.navigator);
    CHECK(transport.getPositionMs() == 3000);

    transport.seekToSample(48000 * 5 + 24000);  // 2.5 s into verse → restart verse
    transport.seekToPreviousSection(fixture.navigator);
    CHECK(transport.getPositionMs() == 3000);

    transport.seekToPreviousSection(fixture.navigator);  // at verse start → intro
    CHECK(transport.getPositionMs() == 0);

    transport.seekToPreviousSection(fixture.navigator);  // first section stays at 0
    CHECK(transport.getPositionMs() == 0);
}

TEST_CASE("Transport section seek while paused keeps the transport paused", "[transport][sections]") {
    SectionFixture fixture;
    auto& transport = fixture.transport;
    REQUIRE(transport.play());
    transport.pause();

    transport.seekToSectionMs(6000);
    CHECK(transport.getState() == TransportState::Paused);
    CHECK(transport.getPositionSamples() == 288000);
    fixture.output.pump(2);
    CHECK(transport.getPositionSamples() == 288000);
}

TEST_CASE("Transport section seek is ignored while stopped", "[transport][sections]") {
    SectionFixture fixture;
    fixture.transport.seekToSectionMs(3000);
    CHECK(fixture.transport.getPositionSamples() == 0);
}
