#pragma once

#include "audio/TrackSourceBase.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>

namespace garageplaymate {

// Stream mode (FR-PLAY-09): the take is read from disk during playback.
//
// Disk I/O happens on the shared background `readAheadThread` through a
// juce::BufferingAudioReader, so the audio thread only copies already-buffered
// samples. If the read-ahead falls behind (e.g. right after a seek on a slow
// disk), the missing samples play as silence rather than blocking the callback.
//
// Sample-rate conversion is done by MultitrackMixerEngine; this source always
// delivers samples at the file's own rate.
class StreamingTrackSource : public TrackSourceBase {
public:
    static constexpr double kReadAheadSeconds = 4.0;
    static constexpr int kInitialBufferTimeoutMs = 2000;

    // `readAheadThread` must outlive this source and be started by the owner.
    // `audioReadTimeoutMs` is how long a playback read may wait for read-ahead:
    // keep 0 for real-time use; tests pass a timeout for deterministic output.
    explicit StreamingTrackSource(juce::TimeSliceThread& readAheadThread, int audioReadTimeoutMs = 0);
    ~StreamingTrackSource() override;

    bool prepare(const std::filesystem::path& wavPath) override;
    void release() override;

protected:
    void readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) override;

private:
    juce::TimeSliceThread& readAheadThread_;
    int audioReadTimeoutMs_ = 0;
    std::unique_ptr<juce::BufferingAudioReader> reader_;
};

}  // namespace garageplaymate
