#pragma once

#include "audio/ITrackAudioSource.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace garageplaymate {

// Mixes N track sources that all start together at sample 0 (FR-PLAY-01).
// Session length is the longest take (FR-PLAY-04); finished takes contribute
// silence (FR-PLAY-05). Positions are in output (device) samples.
//
// Takes whose sample rate differs from the device are resampled per track
// (Lagrange interpolation), so e.g. 44.1 kHz takes play at the correct pitch on a
// 48 kHz device.
//
// Threading:
// - setTracks(), clearTracks(), prepareToPlay(): message thread, only while the
//   engine is not registered as a device callback (all allocation happens here).
// - setPlaying(), requestSeek(), setTrackGain() and the getters: message thread,
//   safe while the audio callback runs. Seeks requested during playback are
//   applied at the start of the next audio block, sample-accurately for all tracks.
// - The audio callback never allocates, locks or does file I/O.
class MultitrackMixerEngine : public juce::AudioIODeviceCallback {
public:
    struct TrackSlot {
        std::unique_ptr<ITrackAudioSource> source;
        float gain = 1.f;
        std::string trackId;
    };

    MultitrackMixerEngine();
    ~MultitrackMixerEngine() override;

    void setTracks(std::vector<TrackSlot> tracks);
    // Releases and destroys all track sources.
    void clearTracks();
    int getNumTracks() const;
    std::vector<std::string> getTrackIds() const;

    void setTrackGain(const std::string& trackId, float linearGain);

    void prepareToPlay(double sampleRate, int maxBlockSize, int numOutputChannels);
    double getSampleRate() const;
    int64_t getSessionLengthSamples() const;

    void setPlaying(bool shouldPlay);
    bool isPlaying() const;
    int64_t getPositionSamples() const;
    void requestSeek(int64_t positionSamples);
    // Set by the audio thread when the session end is reached (playback stops itself).
    bool hasReachedEnd() const;
    bool isAttachedToDevice() const;

    // juce::AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels, int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    struct TrackRuntime {
        std::unique_ptr<ITrackAudioSource> source;
        std::string trackId;
        double sourceSamplesPerOutputSample = 1.0;
        int64_t lengthOutputSamples = 0;
        juce::AudioBuffer<float> fifo;  // source-rate samples awaiting interpolation
        int fifoCount = 0;
        std::vector<juce::LagrangeInterpolator> interpolators;

        bool needsResampling() const { return sourceSamplesPerOutputSample != 1.0; }
    };

    static constexpr int kInterpolatorPadding = 8;
    static constexpr int64_t kNoPendingSeek = -1;

    void configureTracks();
    void applySeek(int64_t positionSamples);
    void renderChunk(float* const* outputChannelData, int numOutputChannels, int offset, int numSamples);
    void renderTrack(TrackRuntime& track, int numSamples);

    std::vector<TrackRuntime> tracks_;
    juce::AudioBuffer<float> trackBuffer_;
    double sampleRate_ = 0.0;
    int maxBlockSize_ = 0;
    int numOutputChannels_ = 0;
    int64_t sessionLengthSamples_ = 0;

    std::atomic<bool> playing_{false};
    std::atomic<bool> reachedEnd_{false};
    std::atomic<bool> attached_{false};
    std::atomic<int64_t> positionSamples_{0};
    std::atomic<int64_t> pendingSeek_{kNoPendingSeek};
};

}  // namespace garageplaymate
