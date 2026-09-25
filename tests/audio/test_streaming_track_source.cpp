#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio/StreamingTrackSource.h"
#include "audio/TestAudioUtils.h"

using Catch::Matchers::WithinAbs;
using garageplaymate::StreamingTrackSource;
using garageplaymate::test::rampSample;
using garageplaymate::test::TempDir;
using garageplaymate::test::writeTestWav;

namespace {

constexpr int kTestTimeoutMs = 2000;

struct ReadAheadThread {
    juce::TimeSliceThread thread{"test read-ahead"};

    ReadAheadThread() { thread.startThread(); }
    ~ReadAheadThread() { thread.stopThread(2000); }
};

}  // namespace

TEST_CASE("StreamingTrackSource reports file format after prepare", "[streaming_track_source]") {
    TempDir tempDir;
    const auto wavPath = tempDir.path / "take.wav";
    REQUIRE(writeTestWav(wavPath, 44100.0, 2, 44100));

    ReadAheadThread readAhead;
    StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
    REQUIRE(source.prepare(wavPath));
    CHECK(source.getLengthSamples() == 44100);
    CHECK(source.getNumChannels() == 2);
    CHECK(source.getSampleRate() == 44100.0);
    CHECK_FALSE(source.isFinished());
}

TEST_CASE("StreamingTrackSource fails for a missing or non-WAV file", "[streaming_track_source]") {
    TempDir tempDir;
    const auto textPath = tempDir.path / "notes.wav";
    juce::File(textPath.string()).replaceWithText("not audio");

    ReadAheadThread readAhead;
    StreamingTrackSource source(readAhead.thread);
    CHECK_FALSE(source.prepare(tempDir.path / "missing.wav"));
    CHECK_FALSE(source.prepare(textPath));
    CHECK(source.getLengthSamples() == 0);
}

TEST_CASE("StreamingTrackSource plays the whole file then silence", "[streaming_track_source]") {
    TempDir tempDir;
    const auto wavPath = tempDir.path / "take.wav";
    constexpr int kLength = 10000;
    REQUIRE(writeTestWav(wavPath, 48000.0, 2, kLength));

    ReadAheadThread readAhead;
    StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
    REQUIRE(source.prepare(wavPath));

    constexpr int kBlock = 512;
    juce::AudioBuffer<float> block(2, kBlock);
    int position = 0;
    bool matches = true;
    while (position < kLength + kBlock) {
        source.getNextAudioBlock(block, 0, kBlock);
        for (int i = 0; i < kBlock; ++i) {
            for (int channel = 0; channel < 2; ++channel) {
                const float expected = position + i < kLength ? rampSample(channel, position + i) : 0.0f;
                matches = matches && block.getSample(channel, i) == expected;
            }
        }
        position += kBlock;
    }
    CHECK(matches);
    CHECK(source.isFinished());
}

TEST_CASE("StreamingTrackSource seek restarts playback", "[streaming_track_source]") {
    TempDir tempDir;
    const auto wavPath = tempDir.path / "take.wav";
    REQUIRE(writeTestWav(wavPath, 48000.0, 1, 20000));

    ReadAheadThread readAhead;
    StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
    REQUIRE(source.prepare(wavPath));

    juce::AudioBuffer<float> block(1, 256);
    source.getNextAudioBlock(block, 0, 256);
    source.getNextAudioBlock(block, 0, 256);
    CHECK(source.getPositionSamples() == 512);

    source.seekToSample(0);
    source.getNextAudioBlock(block, 0, 256);
    CHECK(block.getSample(0, 0) == rampSample(0, 0));
    CHECK(block.getSample(0, 255) == rampSample(0, 255));

    source.seekToSample(15000);
    source.getNextAudioBlock(block, 0, 256);
    CHECK(block.getSample(0, 0) == rampSample(0, 15000));
}

TEST_CASE("StreamingTrackSource maps channels and applies gain", "[streaming_track_source]") {
    TempDir tempDir;
    ReadAheadThread readAhead;

    SECTION("mono take fills both stereo outputs") {
        const auto wavPath = tempDir.path / "mono.wav";
        REQUIRE(writeTestWav(wavPath, 44100.0, 1, 4000));
        StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
        REQUIRE(source.prepare(wavPath));
        source.setGain(0.5f);

        juce::AudioBuffer<float> block(2, 100);
        source.getNextAudioBlock(block, 0, 100);
        CHECK_THAT(block.getSample(0, 42), WithinAbs(rampSample(0, 42) * 0.5f, 1e-6));
        CHECK_THAT(block.getSample(1, 42), WithinAbs(rampSample(0, 42) * 0.5f, 1e-6));
    }

    SECTION("stereo take averages into a mono output") {
        const auto wavPath = tempDir.path / "stereo.wav";
        REQUIRE(writeTestWav(wavPath, 44100.0, 2, 4000));
        StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
        REQUIRE(source.prepare(wavPath));

        juce::AudioBuffer<float> block(1, 100);
        source.getNextAudioBlock(block, 0, 100);
        const float expected = (rampSample(0, 42) + rampSample(1, 42)) * 0.5f;
        CHECK_THAT(block.getSample(0, 42), WithinAbs(expected, 1e-6));
    }

    SECTION("writes only the requested region") {
        const auto wavPath = tempDir.path / "region.wav";
        REQUIRE(writeTestWav(wavPath, 44100.0, 1, 4000));
        StreamingTrackSource source(readAhead.thread, kTestTimeoutMs);
        REQUIRE(source.prepare(wavPath));

        juce::AudioBuffer<float> block(1, 100);
        block.clear();
        block.setSample(0, 0, 9.0f);
        source.getNextAudioBlock(block, 50, 50);
        CHECK(block.getSample(0, 0) == 9.0f);
        CHECK(block.getSample(0, 50) == rampSample(0, 0));
    }
}
