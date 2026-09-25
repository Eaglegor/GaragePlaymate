#pragma once

#include "audio/TrackSourceBase.h"

#include <functional>

namespace garageplaymate::test {

// In-memory track source: `length` frames produced by `generator` at `sampleRate`.
class FakeTrackSource : public TrackSourceBase {
public:
    FakeTrackSource(int64_t length, int numChannels, double sampleRate,
                    std::function<float(int channel, int64_t index)> generator)
        : length_(length), numChannels_(numChannels), sampleRate_(sampleRate), generator_(std::move(generator)) {
        prepare({});
    }

    bool prepare(const std::filesystem::path&) override {
        setFormat(length_, numChannels_, sampleRate_);
        return true;
    }

    void release() override { released = true; }

    bool released = false;

protected:
    void readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) override {
        for (int channel = 0; channel < scratch.getNumChannels(); ++channel) {
            for (int i = 0; i < numSamples; ++i) {
                const int64_t index = sourcePosition + i;
                scratch.setSample(channel, i, index < length_ ? generator_(channel, index) : 0.0f);
            }
        }
    }

private:
    int64_t length_;
    int numChannels_;
    double sampleRate_;
    std::function<float(int, int64_t)> generator_;
};

inline std::unique_ptr<FakeTrackSource> constantSource(int64_t length, float value, double sampleRate = 48000.0) {
    return std::make_unique<FakeTrackSource>(length, 1, sampleRate, [value](int, int64_t) { return value; });
}

}  // namespace garageplaymate::test
