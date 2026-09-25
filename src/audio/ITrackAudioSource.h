#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>
#include <filesystem>

namespace garageplaymate {

// One take of one track (ARCHITECTURE §9.2).
//
// Threading: prepare()/release() on the message thread while no audio callback
// uses the source. getNextAudioBlock() and seekToSample() on the audio thread
// (or the message thread while no callback is active). setGain() from any thread.
// Implementations must be real-time safe after prepare() completes.
class ITrackAudioSource {
public:
    virtual ~ITrackAudioSource() = default;
    virtual bool prepare(const std::filesystem::path& wavPath) = 0;
    virtual void release() = 0;
    // Overwrites [startSample, startSample + numSamples) on every buffer channel with
    // the take's audio (channel-mapped, gain applied); silence past the end.
    virtual void getNextAudioBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) = 0;
    virtual int64_t getLengthSamples() const = 0;
    virtual int getNumChannels() const = 0;
    virtual double getSampleRate() const = 0;
    virtual bool isFinished() const = 0;
    virtual void setGain(float linearGain) = 0;
    virtual void seekToSample(int64_t samplePos) = 0;
};

}  // namespace garageplaymate
