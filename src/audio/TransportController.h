#pragma once

#include "audio/IAudioOutput.h"
#include "audio/MultitrackMixerEngine.h"

#include <juce_events/juce_events.h>

#include <cstdint>
#include <functional>

namespace garageplaymate {

enum class TransportState { Stopped, Playing, Paused };

// Play/Pause/Stop/Seek state machine over a MultitrackMixerEngine (FR-PLAY-03).
// Session duration is the engine's longest take (FR-PLAY-04).
//
// All methods and callbacks run on the message thread. Position is polled from
// the engine by a 30 Hz timer, so the audio thread never calls into UI code.
class TransportController : private juce::Timer {
public:
    static constexpr int kPositionUpdateHz = 30;

    TransportController();
    ~TransportController() override;

    // `engine` and `output` must outlive the controller (or until detach()).
    void attach(MultitrackMixerEngine& engine, IAudioOutput& output);
    void detach();

    TransportState getState() const;
    int64_t getPositionSamples() const;
    int64_t getDurationSamples() const;
    double getSampleRate() const;
    int64_t getPositionMs() const;
    int64_t getDurationMs() const;

    // From Stopped: opens the output device if needed and starts at 0.
    // From Paused: resumes. Returns false (with a message) if playback can't start.
    bool play(juce::String* errorMessage = nullptr);
    void pause();
    // Detaches the engine from the device, rewinds to 0 and invokes onStop.
    void stop();
    // Ignored while Stopped (play always starts from 0). Clamped to the session.
    void seekToSample(int64_t samplePos);

    // Polls the engine: fires onPositionChanged, and onReachedEnd after an
    // automatic stop at the session end. Called by the timer; public for tests.
    void update();

    std::function<void(TransportState)> onStateChanged;
    std::function<void(int64_t positionSamples, int64_t durationSamples)> onPositionChanged;
    std::function<void()> onReachedEnd;
    // After stop(): the owner may release track sources (e.g. preloaded RAM).
    std::function<void()> onStop;

private:
    void timerCallback() override;
    void setState(TransportState newState);
    void notifyPosition();
    int64_t samplesToMs(int64_t samples) const;

    MultitrackMixerEngine* engine_ = nullptr;
    IAudioOutput* output_ = nullptr;
    TransportState state_ = TransportState::Stopped;
};

}  // namespace garageplaymate
