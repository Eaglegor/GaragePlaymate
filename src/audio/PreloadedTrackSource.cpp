#include "audio/PreloadedTrackSource.h"

#include <algorithm>
#include <limits>

namespace garageplaymate {

bool PreloadedTrackSource::prepare(const std::filesystem::path& wavPath) {
    return prepare(wavPath, {});
}

bool PreloadedTrackSource::prepare(const std::filesystem::path& wavPath, const ProgressCallback& progressCallback) {
    release();

    std::unique_ptr<juce::AudioFormatReader> reader = createWavReader(wavPath);
    if (reader == nullptr) {
        juce::Logger::writeToLog("[PreloadedTrackSource] Cannot open WAV: " + toJuceFile(wavPath).getFullPathName());
        return false;
    }

    const auto lengthSamples = static_cast<int64_t>(reader->lengthInSamples);
    const auto numChannels = static_cast<int>(reader->numChannels);
    if (lengthSamples > std::numeric_limits<int>::max()) {
        juce::Logger::writeToLog("[PreloadedTrackSource] Take too long to preload: " +
                                 toJuceFile(wavPath).getFullPathName());
        return false;
    }

    audio_.setSize(numChannels, static_cast<int>(lengthSamples));
    juce::HeapBlock<float*> channelPointers(static_cast<size_t>(numChannels));
    int64_t loaded = 0;
    while (loaded < lengthSamples) {
        const int chunk = static_cast<int>(std::min<int64_t>(kLoadChunkSamples, lengthSamples - loaded));
        for (int channel = 0; channel < numChannels; ++channel) {
            channelPointers[channel] = audio_.getWritePointer(channel, static_cast<int>(loaded));
        }
        if (!reader->read(channelPointers.get(), numChannels, loaded, chunk)) {
            juce::Logger::writeToLog("[PreloadedTrackSource] Read failed: " + toJuceFile(wavPath).getFullPathName());
            audio_.setSize(0, 0);
            return false;
        }
        loaded += chunk;
        if (progressCallback) {
            progressCallback(static_cast<float>(loaded) / static_cast<float>(lengthSamples));
        }
    }

    if (lengthSamples == 0 && progressCallback) {
        progressCallback(1.0f);
    }

    setFormat(lengthSamples, numChannels, reader->sampleRate);
    return true;
}

void PreloadedTrackSource::release() {
    audio_.setSize(0, 0);
    clearFormat();
}

std::size_t PreloadedTrackSource::estimateMemoryBytes(int numChannels, int64_t numSamples) {
    return static_cast<std::size_t>(std::max(numChannels, 0)) * static_cast<std::size_t>(std::max<int64_t>(numSamples, 0)) *
           sizeof(float);
}

void PreloadedTrackSource::readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) {
    const int64_t length = audio_.getNumSamples();
    const int available = static_cast<int>(std::clamp<int64_t>(length - sourcePosition, 0, numSamples));
    for (int channel = 0; channel < scratch.getNumChannels(); ++channel) {
        if (available > 0) {
            scratch.copyFrom(channel, 0, audio_, channel, static_cast<int>(sourcePosition), available);
        }
        if (available < numSamples) {
            scratch.clear(channel, available, numSamples - available);
        }
    }
}

}  // namespace garageplaymate
