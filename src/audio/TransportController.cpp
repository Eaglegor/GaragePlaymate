#include "audio/TransportController.h"

#include <algorithm>

namespace garageplaymate {

TransportController::TransportController() = default;

TransportController::~TransportController() {
    detach();
}

void TransportController::attach(MultitrackMixerEngine& engine, IAudioOutput& output) {
    detach();
    engine_ = &engine;
    output_ = &output;
}

void TransportController::detach() {
    stop();
    engine_ = nullptr;
    output_ = nullptr;
}

TransportState TransportController::getState() const {
    return state_;
}

int64_t TransportController::getPositionSamples() const {
    if (engine_ == nullptr) {
        return 0;
    }
    return std::min(engine_->getPositionSamples(), engine_->getSessionLengthSamples());
}

int64_t TransportController::getDurationSamples() const {
    return engine_ != nullptr ? engine_->getSessionLengthSamples() : 0;
}

double TransportController::getSampleRate() const {
    return engine_ != nullptr ? engine_->getSampleRate() : 0.0;
}

int64_t TransportController::getPositionMs() const {
    return samplesToMs(getPositionSamples());
}

int64_t TransportController::getDurationMs() const {
    return samplesToMs(getDurationSamples());
}

bool TransportController::play(juce::String* errorMessage) {
    juce::String error;
    if (engine_ == nullptr || output_ == nullptr) {
        error = "Transport is not attached to an engine";
    } else if (state_ == TransportState::Playing) {
        return true;
    } else if (state_ == TransportState::Paused) {
        engine_->setPlaying(true);
        setState(TransportState::Playing);
        return true;
    } else if (engine_->getNumTracks() == 0) {
        error = "No tracks to play";
    } else if (output_->ensureDeviceOpen(error)) {
        engine_->prepareToPlay(output_->getCurrentSampleRate(), output_->getCurrentBlockSize(),
                               output_->getNumOutputChannels());
        engine_->requestSeek(0);
        engine_->setPlaying(true);
        output_->addCallback(engine_);
        setState(TransportState::Playing);
        startTimerHz(kPositionUpdateHz);
        notifyPosition();
        return true;
    }

    juce::Logger::writeToLog("[TransportController] Cannot play: " + error);
    if (errorMessage != nullptr) {
        *errorMessage = error;
    }
    return false;
}

void TransportController::pause() {
    if (state_ != TransportState::Playing) {
        return;
    }
    engine_->setPlaying(false);
    setState(TransportState::Paused);
    notifyPosition();
}

void TransportController::stop() {
    if (state_ == TransportState::Stopped || engine_ == nullptr) {
        return;
    }
    stopTimer();
    output_->removeCallback(engine_);
    engine_->setPlaying(false);
    engine_->requestSeek(0);
    setState(TransportState::Stopped);
    notifyPosition();
    if (onStop) {
        onStop();
    }
}

void TransportController::seekToSample(int64_t samplePos) {
    if (state_ == TransportState::Stopped || engine_ == nullptr) {
        return;
    }
    engine_->requestSeek(std::clamp<int64_t>(samplePos, 0, getDurationSamples()));
    notifyPosition();
}

void TransportController::update() {
    if (engine_ == nullptr || state_ == TransportState::Stopped) {
        return;
    }
    if (state_ == TransportState::Playing && engine_->hasReachedEnd()) {
        stop();
        if (onReachedEnd) {
            onReachedEnd();
        }
        return;
    }
    notifyPosition();
}

void TransportController::timerCallback() {
    update();
}

void TransportController::setState(TransportState newState) {
    if (state_ == newState) {
        return;
    }
    state_ = newState;
    if (onStateChanged) {
        onStateChanged(newState);
    }
}

void TransportController::notifyPosition() {
    if (onPositionChanged) {
        onPositionChanged(getPositionSamples(), getDurationSamples());
    }
}

int64_t TransportController::samplesToMs(int64_t samples) const {
    const double sampleRate = getSampleRate();
    return sampleRate > 0.0 ? static_cast<int64_t>(static_cast<double>(samples) * 1000.0 / sampleRate) : 0;
}

}  // namespace garageplaymate
