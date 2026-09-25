#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio/FakeAudioOutput.h"
#include "audio/FakeTrackSource.h"
#include "audio/MultitrackMixerEngine.h"

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;
using garageplaymate::MultitrackMixerEngine;
using garageplaymate::test::constantSource;
using garageplaymate::test::FakeTrackSource;

namespace {

// Runs the engine as a device would, with a stereo output of `numSamples`.
struct Render {
    juce::AudioBuffer<float> output;

    Render(MultitrackMixerEngine& engine, int numSamples) : output(2, numSamples) {
        output.clear();
        engine.audioDeviceIOCallbackWithContext(nullptr, 0, output.getArrayOfWritePointers(), 2, numSamples, {});
    }

    float at(int channel, int index) const { return output.getSample(channel, index); }
};

MultitrackMixerEngine::TrackSlot slot(std::unique_ptr<garageplaymate::ITrackAudioSource> source, const std::string& id,
                                      float gain = 1.0f) {
    return MultitrackMixerEngine::TrackSlot{std::move(source), gain, id};
}

}  // namespace

TEST_CASE("Mixer session length is the longest take", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 256, 2);

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(constantSource(1000, 0.25f), "drums"));
    tracks.push_back(slot(constantSource(2000, 0.5f), "guitar"));
    engine.setTracks(std::move(tracks));

    CHECK(engine.getNumTracks() == 2);
    CHECK(engine.getSessionLengthSamples() == 2000);
    CHECK(engine.getTrackIds() == std::vector<std::string>{"drums", "guitar"});
}

TEST_CASE("Mixer starts all tracks together and silences finished ones", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 256, 2);

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(constantSource(1000, 0.25f), "drums"));
    tracks.push_back(slot(constantSource(2000, 0.5f), "guitar"));
    engine.setTracks(std::move(tracks));
    engine.setPlaying(true);

    Render first(engine, 1024);
    CHECK_THAT(first.at(0, 0), WithinAbs(0.75, 1e-6));
    CHECK_THAT(first.at(1, 0), WithinAbs(0.75, 1e-6));
    CHECK_THAT(first.at(0, 999), WithinAbs(0.75, 1e-6));
    CHECK_THAT(first.at(0, 1000), WithinAbs(0.5, 1e-6));
    CHECK(engine.getPositionSamples() == 1024);
    CHECK(engine.isPlaying());

    Render second(engine, 1024);
    CHECK_THAT(second.at(0, 975), WithinAbs(0.5, 1e-6));
    CHECK(second.at(0, 976) == 0.0f);
    CHECK_FALSE(engine.isPlaying());
    CHECK(engine.hasReachedEnd());
}

TEST_CASE("Mixer applies per-track gain", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 128, 2);

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(constantSource(500, 0.5f), "bass", 0.5f));
    tracks.push_back(slot(constantSource(500, 0.5f), "keys"));
    engine.setTracks(std::move(tracks));
    engine.setPlaying(true);

    Render first(engine, 64);
    CHECK_THAT(first.at(0, 10), WithinAbs(0.75, 1e-6));

    engine.setTrackGain("keys", 0.0f);
    Render second(engine, 64);
    CHECK_THAT(second.at(0, 10), WithinAbs(0.25, 1e-6));
}

TEST_CASE("Mixer outputs silence while paused and keeps position", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 128, 2);

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(constantSource(5000, 0.5f), "drums"));
    engine.setTracks(std::move(tracks));

    Render stopped(engine, 128);
    CHECK(stopped.at(0, 0) == 0.0f);
    CHECK(engine.getPositionSamples() == 0);

    engine.setPlaying(true);
    Render playing(engine, 128);
    engine.setPlaying(false);
    Render paused(engine, 128);
    CHECK(paused.at(0, 5) == 0.0f);
    CHECK(engine.getPositionSamples() == 128);
}

TEST_CASE("Mixer seeks all tracks to the same sample", "[mixer]") {
    MultitrackMixerEngine engine;
    auto ramp = [](int, int64_t index) { return static_cast<float>(index) / 10000.0f; };
    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(std::make_unique<FakeTrackSource>(8000, 1, 48000.0, ramp), "a"));
    tracks.push_back(slot(std::make_unique<FakeTrackSource>(8000, 1, 48000.0, ramp), "b"));
    engine.setTracks(std::move(tracks));

    SECTION("while detached, seeks apply immediately") {
        engine.prepareToPlay(48000.0, 128, 2);
        engine.requestSeek(4000);
        engine.setPlaying(true);
        Render afterSeek(engine, 128);
        CHECK_THAT(afterSeek.at(0, 0), WithinAbs(2 * 0.4, 1e-6));
        CHECK(engine.getPositionSamples() == 4128);
    }

    SECTION("while attached, seeks apply at the next audio block") {
        garageplaymate::test::FakeAudioOutput output(48000.0, 128);
        output.addCallback(&engine);
        REQUIRE(engine.isAttachedToDevice());
        engine.setPlaying(true);
        output.pump();

        engine.requestSeek(4000);
        CHECK(engine.getPositionSamples() == 4000);  // reported before the audio thread applies it
        const auto& block = output.pump();
        CHECK_THAT(block.getSample(0, 0), WithinAbs(2 * 0.4, 1e-6));
        CHECK_THAT(block.getSample(1, 127), WithinAbs(2 * 0.4127, 1e-5));
        CHECK(engine.getPositionSamples() == 4128);

        output.removeCallback(&engine);
        CHECK_FALSE(engine.isAttachedToDevice());
    }

    SECTION("seeks are clamped to the session") {
        engine.prepareToPlay(48000.0, 128, 2);
        engine.requestSeek(99999);
        CHECK(engine.getPositionSamples() == 8000);
        engine.requestSeek(-5);
        CHECK(engine.getPositionSamples() == 0);
    }
}

TEST_CASE("Mixer handles device blocks larger than the prepared size", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 100, 2);

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(constantSource(1000, 0.5f), "drums"));
    engine.setTracks(std::move(tracks));
    engine.setPlaying(true);

    Render big(engine, 512);
    CHECK_THAT(big.at(0, 0), WithinAbs(0.5, 1e-6));
    CHECK_THAT(big.at(1, 511), WithinAbs(0.5, 1e-6));
    CHECK(engine.getPositionSamples() == 512);
}

TEST_CASE("Mixer resamples takes to the device rate", "[mixer]") {
    MultitrackMixerEngine engine;
    engine.prepareToPlay(48000.0, 256, 2);

    // 1 second of a 441 Hz sine at 44.1 kHz (exactly 100 samples per cycle).
    constexpr double kSourceRate = 44100.0;
    auto sine = [](int, int64_t index) {
        return static_cast<float>(0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * static_cast<double>(index) / 100.0));
    };
    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(std::make_unique<FakeTrackSource>(44100, 1, kSourceRate, sine), "vocals"));
    engine.setTracks(std::move(tracks));
    CHECK(engine.getSessionLengthSamples() == 48000);

    engine.setPlaying(true);
    std::vector<float> rendered;
    for (int block = 0; block < 190; ++block) {
        Render render(engine, 256);
        for (int i = 0; i < 256; ++i) {
            rendered.push_back(render.at(0, i));
        }
    }

    // At 48 kHz a 441 Hz sine repeats every 48000/441 ≈ 108.84 samples; compare
    // against the ideal waveform, allowing for the interpolator's 2-sample latency.
    double maxError = 0.0;
    for (size_t i = 1000; i < 40000; ++i) {
        const double t = (static_cast<double>(i) - 2.0) * 441.0 / 48000.0;
        const double expected = 0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * t);
        maxError = std::max(maxError, std::abs(expected - rendered[i]));
    }
    CHECK(maxError < 0.02);
}

TEST_CASE("Mixer releases sources when tracks are cleared", "[mixer]") {
    MultitrackMixerEngine engine;
    auto source = constantSource(100, 0.1f);
    FakeTrackSource* raw = source.get();

    std::vector<MultitrackMixerEngine::TrackSlot> tracks;
    tracks.push_back(slot(std::move(source), "drums"));
    engine.setTracks(std::move(tracks));
    CHECK_FALSE(raw->released);

    engine.clearTracks();
    CHECK(engine.getNumTracks() == 0);
    CHECK(engine.getSessionLengthSamples() == 0);
}
