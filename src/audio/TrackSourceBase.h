#pragma once

#include "audio/ITrackAudioSource.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <memory>

namespace garageplaymate {

// Creates a reader for a WAV file (the only format supported in v1), or nullptr.
std::unique_ptr<juce::AudioFormatReader> createWavReader(const std::filesystem::path& wavPath);

juce::File toJuceFile(const std::filesystem::path& path);

// Shared playback state for track sources: read position, gain, channel mapping.
// Subclasses provide readSamples(), which fills a pre-allocated scratch buffer.
//
// Channel mapping into the output buffer: mono takes are copied to every output
// channel; multi-channel takes feed matching output channels (extra output
// channels are silent); a mono output receives the average of all take channels.
class TrackSourceBase : public ITrackAudioSource {
public:
    static constexpr int kScratchBlockSamples = 4096;

    void getNextAudioBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;
    int64_t getLengthSamples() const override;
    int getNumChannels() const override;
    double getSampleRate() const override;
    bool isFinished() const override;
    void setGain(float linearGain) override;
    void seekToSample(int64_t samplePos) override;

    int64_t getPositionSamples() const;

protected:
    // Message thread, in prepare(): records format info and allocates scratch space.
    void setFormat(int64_t lengthSamples, int numChannels, double sampleRate);
    void clearFormat();

    // Audio thread: fill `numSamples` (<= kScratchBlockSamples) frames of every
    // take channel starting at `sourcePosition` into scratch; zeros past the end.
    virtual void readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) = 0;

private:
    juce::AudioBuffer<float> scratch_;
    int64_t lengthSamples_ = 0;
    int numChannels_ = 0;
    double sampleRate_ = 0.0;
    std::atomic<int64_t> position_{0};
    std::atomic<float> gain_{1.0f};
};

}  // namespace garageplaymate
