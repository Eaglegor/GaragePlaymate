#include "audio/MultitrackMixerEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace garageplaymate {

MultitrackMixerEngine::MultitrackMixerEngine() = default;

MultitrackMixerEngine::~MultitrackMixerEngine() {
    clearTracks();
}

void MultitrackMixerEngine::setTracks(std::vector<TrackSlot> tracks) {
    if (attached_.load()) {
        jassertfalse;  // tracks may only change while the engine is detached from the device
        juce::Logger::writeToLog("[MultitrackMixerEngine] setTracks ignored while attached to a device");
        return;
    }

    clearTracks();
    tracks_.reserve(tracks.size());
    for (TrackSlot& slot : tracks) {
        if (slot.source == nullptr) {
            continue;
        }
        slot.source->setGain(slot.gain);
        TrackRuntime runtime;
        runtime.source = std::move(slot.source);
        runtime.trackId = std::move(slot.trackId);
        tracks_.push_back(std::move(runtime));
    }

    configureTracks();
    applySeek(0);
}

void MultitrackMixerEngine::clearTracks() {
    jassert(!attached_.load());
    for (TrackRuntime& track : tracks_) {
        track.source->release();
    }
    tracks_.clear();
    sessionLengthSamples_ = 0;
    positionSamples_.store(0);
    reachedEnd_.store(false);
}

int MultitrackMixerEngine::getNumTracks() const {
    return static_cast<int>(tracks_.size());
}

std::vector<std::string> MultitrackMixerEngine::getTrackIds() const {
    std::vector<std::string> ids;
    ids.reserve(tracks_.size());
    for (const TrackRuntime& track : tracks_) {
        ids.push_back(track.trackId);
    }
    return ids;
}

void MultitrackMixerEngine::setTrackGain(const std::string& trackId, float linearGain) {
    for (TrackRuntime& track : tracks_) {
        if (track.trackId == trackId) {
            track.source->setGain(linearGain);
        }
    }
}

void MultitrackMixerEngine::prepareToPlay(double sampleRate, int maxBlockSize, int numOutputChannels) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = std::max(maxBlockSize, 1);
    numOutputChannels_ = std::max(numOutputChannels, 1);
    trackBuffer_.setSize(numOutputChannels_, maxBlockSize_);
    configureTracks();
    applySeek(positionSamples_.load());
}

double MultitrackMixerEngine::getSampleRate() const {
    return sampleRate_;
}

int64_t MultitrackMixerEngine::getSessionLengthSamples() const {
    return sessionLengthSamples_;
}

void MultitrackMixerEngine::setPlaying(bool shouldPlay) {
    if (shouldPlay) {
        reachedEnd_.store(false);
    }
    playing_.store(shouldPlay);
}

bool MultitrackMixerEngine::isPlaying() const {
    return playing_.load();
}

int64_t MultitrackMixerEngine::getPositionSamples() const {
    const int64_t pending = pendingSeek_.load();
    return pending != kNoPendingSeek ? pending : positionSamples_.load();
}

void MultitrackMixerEngine::requestSeek(int64_t positionSamples) {
    const int64_t clamped = std::clamp<int64_t>(positionSamples, 0, sessionLengthSamples_);
    if (attached_.load()) {
        pendingSeek_.store(clamped);
    } else {
        applySeek(clamped);
    }
}

bool MultitrackMixerEngine::hasReachedEnd() const {
    return reachedEnd_.load();
}

bool MultitrackMixerEngine::isAttachedToDevice() const {
    return attached_.load();
}

void MultitrackMixerEngine::audioDeviceIOCallbackWithContext(const float* const*, int, float* const* outputChannelData,
                                                             int numOutputChannels, int numSamples,
                                                             const juce::AudioIODeviceCallbackContext&) {
    for (int channel = 0; channel < numOutputChannels; ++channel) {
        if (outputChannelData[channel] != nullptr) {
            std::memset(outputChannelData[channel], 0, sizeof(float) * static_cast<size_t>(numSamples));
        }
    }

    const int64_t pending = pendingSeek_.exchange(kNoPendingSeek);
    if (pending != kNoPendingSeek) {
        applySeek(pending);
    }

    if (!playing_.load() || maxBlockSize_ == 0) {
        return;
    }

    for (int offset = 0; offset < numSamples; offset += maxBlockSize_) {
        renderChunk(outputChannelData, numOutputChannels, offset, std::min(maxBlockSize_, numSamples - offset));
    }

    const int64_t position = positionSamples_.fetch_add(numSamples) + numSamples;
    if (position >= sessionLengthSamples_) {
        playing_.store(false);
        reachedEnd_.store(true);
    }
}

void MultitrackMixerEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    const int channels = device->getActiveOutputChannels().countNumberOfSetBits();
    if (device->getCurrentSampleRate() != sampleRate_ || device->getCurrentBufferSizeSamples() > maxBlockSize_ ||
        channels != numOutputChannels_) {
        prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples(), channels);
    }
    attached_.store(true);
}

void MultitrackMixerEngine::audioDeviceStopped() {
    attached_.store(false);
    const int64_t pending = pendingSeek_.exchange(kNoPendingSeek);
    if (pending != kNoPendingSeek) {
        applySeek(pending);
    }
}

void MultitrackMixerEngine::configureTracks() {
    sessionLengthSamples_ = 0;
    if (sampleRate_ <= 0.0) {
        return;
    }

    for (TrackRuntime& track : tracks_) {
        const double sourceRate = track.source->getSampleRate();
        track.sourceSamplesPerOutputSample = sourceRate > 0.0 ? sourceRate / sampleRate_ : 1.0;
        track.lengthOutputSamples = static_cast<int64_t>(
            std::ceil(static_cast<double>(track.source->getLengthSamples()) / track.sourceSamplesPerOutputSample));
        sessionLengthSamples_ = std::max(sessionLengthSamples_, track.lengthOutputSamples);

        if (track.needsResampling()) {
            const int capacity =
                static_cast<int>(std::ceil(maxBlockSize_ * track.sourceSamplesPerOutputSample)) + kInterpolatorPadding * 2;
            track.fifo.setSize(numOutputChannels_, capacity);
            track.interpolators = std::vector<juce::LagrangeInterpolator>(static_cast<size_t>(numOutputChannels_));
            juce::Logger::writeToLog("[MultitrackMixerEngine] Track '" + juce::String(track.trackId) + "' at " +
                                     juce::String(sourceRate) + " Hz resampled to " + juce::String(sampleRate_) + " Hz");
        } else {
            track.fifo.setSize(0, 0);
            track.interpolators.clear();
        }
        track.fifoCount = 0;
    }
}

void MultitrackMixerEngine::applySeek(int64_t positionSamples) {
    for (TrackRuntime& track : tracks_) {
        const auto sourcePosition =
            static_cast<int64_t>(std::llround(static_cast<double>(positionSamples) * track.sourceSamplesPerOutputSample));
        track.source->seekToSample(sourcePosition);
        track.fifoCount = 0;
        for (juce::LagrangeInterpolator& interpolator : track.interpolators) {
            interpolator.reset();
        }
    }
    positionSamples_.store(positionSamples);
    reachedEnd_.store(false);
}

void MultitrackMixerEngine::renderChunk(float* const* outputChannelData, int numOutputChannels, int offset,
                                        int numSamples) {
    const int channels = std::min(numOutputChannels, trackBuffer_.getNumChannels());
    for (TrackRuntime& track : tracks_) {
        renderTrack(track, numSamples);
        for (int channel = 0; channel < channels; ++channel) {
            if (outputChannelData[channel] != nullptr) {
                juce::FloatVectorOperations::add(outputChannelData[channel] + offset, trackBuffer_.getReadPointer(channel),
                                                 numSamples);
            }
        }
    }
}

void MultitrackMixerEngine::renderTrack(TrackRuntime& track, int numSamples) {
    if (!track.needsResampling()) {
        track.source->getNextAudioBlock(trackBuffer_, 0, numSamples);
        return;
    }

    const double ratio = track.sourceSamplesPerOutputSample;
    const int needed = std::min(static_cast<int>(std::ceil(numSamples * ratio)) + kInterpolatorPadding,
                                track.fifo.getNumSamples());
    if (track.fifoCount < needed) {
        track.source->getNextAudioBlock(track.fifo, track.fifoCount, needed - track.fifoCount);
        track.fifoCount = needed;
    }

    int used = 0;
    for (int channel = 0; channel < trackBuffer_.getNumChannels(); ++channel) {
        used = track.interpolators[static_cast<size_t>(channel)].process(
            ratio, track.fifo.getReadPointer(channel), trackBuffer_.getWritePointer(channel), numSamples);
    }

    used = std::min(used, track.fifoCount);
    const int remaining = track.fifoCount - used;
    for (int channel = 0; channel < track.fifo.getNumChannels(); ++channel) {
        float* data = track.fifo.getWritePointer(channel);
        std::memmove(data, data + used, sizeof(float) * static_cast<size_t>(remaining));
    }
    track.fifoCount = remaining;
}

}  // namespace garageplaymate
