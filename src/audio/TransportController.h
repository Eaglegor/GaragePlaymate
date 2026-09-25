#pragma once

#include "audio/IAudioOutput.h"
#include "audio/MultitrackMixerEngine.h"
#include "core/SectionNavigator.h"

#include <juce_events/juce_events.h>

#include <cstdint>
#include <functional>
#include <string>

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
    // Stops playback without invoking callbacks.
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
    // While Playing, all tracks jump together at the next audio block; while
    // Paused only the position changes.
    void seekToSample(int64_t samplePos);

    // Section navigation (FR-PLAY-03), same rules as seekToSample().
    void seekToSectionMs(int64_t startMs);
    // Returns false if the section id is unknown.
    bool seekToSectionId(const std::string& sectionId, const SectionNavigator& navigator);
    // No-op in the last section.
    void seekToNextSection(const SectionNavigator& navigator);
    // Restarts the current section when more than kRestartSectionThresholdMs into
    // it (like a media player's "previous"); otherwise goes to the previous section.
    void seekToPreviousSection(const SectionNavigator& navigator);

    static constexpr int64_t kRestartSectionThresholdMs = 2000;

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
    int64_t msToSamples(int64_t ms) const;

    MultitrackMixerEngine* engine_ = nullptr;
    IAudioOutput* output_ = nullptr;
    TransportState state_ = TransportState::Stopped;
};

}  // namespace garageplaymate
