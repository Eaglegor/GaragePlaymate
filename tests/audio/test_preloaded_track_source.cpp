#include <catch2/catch_test_macros.hpp>

#include "audio/PreloadedTrackSource.h"
#include "audio/StreamingTrackSource.h"
#include "audio/TestAudioUtils.h"

#include <algorithm>
#include <vector>

using garageplaymate::PreloadedTrackSource;
using garageplaymate::test::rampSample;
using garageplaymate::test::TempDir;
using garageplaymate::test::writeTestWav;

TEST_CASE("PreloadedTrackSource loads the whole take with progress", "[preloaded_track_source]") {
    TempDir tempDir;
    const auto wavPath = tempDir.path / "take.wav";
    constexpr int kLength = 48000 * 10;  // 10 seconds
    REQUIRE(writeTestWav(wavPath, 48000.0, 2, kLength));

    PreloadedTrackSource source;
    std::vector<float> progress;
    REQUIRE(source.prepare(wavPath, [&progress](float fraction) { progress.push_back(fraction); }));

    CHECK(source.getLengthSamples() == kLength);
    CHECK(source.getNumChannels() == 2);
    CHECK(source.getSampleRate() == 48000.0);
    REQUIRE(progress.size() > 1);
    CHECK(progress.back() == 1.0f);
    CHECK(std::is_sorted(progress.begin(), progress.end()));
}

TEST_CASE("PreloadedTrackSource output matches StreamingTrackSource", "[preloaded_track_source]") {
    TempDir tempDir;
    const auto wavPath = tempDir.path / "take.wav";
    constexpr int kLength = 30000;
    REQUIRE(writeTestWav(wavPath, 44100.0, 2, kLength));

    juce::TimeSliceThread readAheadThread("test read-ahead");
    readAheadThread.startThread();

    PreloadedTrackSource preloaded;
    garageplaymate::StreamingTrackSource streaming(readAheadThread, 2000);
    REQUIRE(preloaded.prepare(wavPath));
    REQUIRE(streaming.prepare(wavPath));

    juce::AudioBuffer<float> preloadedBlock(2, 480);
    juce::AudioBuffer<float> streamingBlock(2, 480);
    bool identical = true;
    for (int position = 0; position < kLength + 960; position += 480) {
        preloaded.getNextAudioBlock(preloadedBlock, 0, 480);
        streaming.getNextAudioBlock(streamingBlock, 0, 480);
        for (int channel = 0; channel < 2; ++channel) {
            for (int i = 0; i < 480; ++i) {
                identical = identical && preloadedBlock.getSample(channel, i) == streamingBlock.getSample(channel, i);
            }
        }
    }
    CHECK(identical);
    CHECK(preloaded.isFinished());

    streaming.release();
    readAheadThread.stopThread(2000);
}

TEST_CASE("PreloadedTrackSource seeks and can be re-prepared after release", "[preloaded_track_source]") {
    TempDir tempDir;
    const auto firstPath = tempDir.path / "first.wav";
    const auto secondPath = tempDir.path / "second.wav";
    REQUIRE(writeTestWav(firstPath, 44100.0, 1, 5000));
    REQUIRE(writeTestWav(secondPath, 44100.0, 1, 8000));

    PreloadedTrackSource source;
    REQUIRE(source.prepare(firstPath));

    juce::AudioBuffer<float> block(1, 64);
    source.seekToSample(4990);
    source.getNextAudioBlock(block, 0, 64);
    CHECK(block.getSample(0, 0) == rampSample(0, 4990));
    CHECK(block.getSample(0, 20) == 0.0f);
    CHECK(source.isFinished());

    source.release();
    CHECK(source.getLengthSamples() == 0);
    source.getNextAudioBlock(block, 0, 64);
    CHECK(block.getSample(0, 0) == 0.0f);

    REQUIRE(source.prepare(secondPath));
    CHECK(source.getLengthSamples() == 8000);
    source.getNextAudioBlock(block, 0, 64);
    CHECK(block.getSample(0, 10) == rampSample(0, 10));
}

TEST_CASE("PreloadedTrackSource estimates memory", "[preloaded_track_source]") {
    CHECK(PreloadedTrackSource::estimateMemoryBytes(2, 48000) == 2 * 48000 * sizeof(float));
    CHECK(PreloadedTrackSource::estimateMemoryBytes(0, 48000) == 0);
}

TEST_CASE("PreloadProgress combines per-track progress", "[preloaded_track_source]") {
    garageplaymate::PreloadProgress progress{1, 4, 0.5f};
    CHECK(progress.overallFraction() == 0.375f);
    CHECK(garageplaymate::PreloadProgress{}.overallFraction() == 1.0f);
}
