#pragma once

#include "audio/TrackSourceBase.h"

#include <cstddef>
#include <functional>

namespace garageplaymate {

// Aggregate progress for preloading all selected takes of a session (NFR-PERF-04).
struct PreloadProgress {
    int tracksCompleted = 0;
    int tracksTotal = 0;
    float currentTrackFraction = 0.f;

    float overallFraction() const {
        return tracksTotal > 0 ? (static_cast<float>(tracksCompleted) + currentTrackFraction) /
                                     static_cast<float>(tracksTotal)
                               : 1.f;
    }
};

// Preload mode (FR-PLAY-09/10): the whole take is decoded into RAM in prepare(),
// before playback starts; playback then only copies from memory. Only the takes
// selected for the current session are preloaded, and release() frees the
// memory on Stop or song change (NFR-PERF-03).
//
// prepare() may run on the message thread or a loader thread; the progress
// callback is invoked on the calling thread.
class PreloadedTrackSource : public TrackSourceBase {
public:
    using ProgressCallback = std::function<void(float fraction0to1)>;

    static constexpr int kLoadChunkSamples = 65536;

    bool prepare(const std::filesystem::path& wavPath) override;
    bool prepare(const std::filesystem::path& wavPath, const ProgressCallback& progressCallback);
    void release() override;

    static std::size_t estimateMemoryBytes(int numChannels, int64_t numSamples);

protected:
    void readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) override;

private:
    juce::AudioBuffer<float> audio_;
};

}  // namespace garageplaymate
