#include "audio/TrackSourceBase.h"

#include <algorithm>

namespace garageplaymate {

std::unique_ptr<juce::AudioFormatReader> createWavReader(const std::filesystem::path& wavPath) {
    juce::AudioFormatManager formatManager;
    formatManager.registerFormat(new juce::WavAudioFormat(), true);
    return std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(toJuceFile(wavPath)));
}

juce::File toJuceFile(const std::filesystem::path& path) {
    const std::u8string utf8 = path.u8string();
    return juce::File(juce::String::fromUTF8(reinterpret_cast<const char*>(utf8.c_str()),
                                             static_cast<int>(utf8.size())));
}

void TrackSourceBase::getNextAudioBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    const int outputChannels = buffer.getNumChannels();
    const float gain = gain_.load(std::memory_order_relaxed);
    int64_t position = position_.load(std::memory_order_relaxed);

    if (numChannels_ == 0 || position >= lengthSamples_) {
        buffer.clear(startSample, numSamples);
        position_.store(position + numSamples, std::memory_order_relaxed);
        return;
    }

    int done = 0;
    while (done < numSamples) {
        const int chunk = std::min(numSamples - done, kScratchBlockSamples);
        readSamples(scratch_, position, chunk);
        const int destStart = startSample + done;

        if (outputChannels == 1 && numChannels_ > 1) {
            buffer.copyFrom(0, destStart, scratch_, 0, 0, chunk);
            for (int channel = 1; channel < numChannels_; ++channel) {
                buffer.addFrom(0, destStart, scratch_, channel, 0, chunk);
            }
            buffer.applyGain(0, destStart, chunk, gain / static_cast<float>(numChannels_));
        } else {
            for (int channel = 0; channel < outputChannels; ++channel) {
                const int sourceChannel = numChannels_ == 1 ? 0 : channel;
                if (sourceChannel < numChannels_) {
                    buffer.copyFrom(channel, destStart, scratch_.getReadPointer(sourceChannel), chunk, gain);
                } else {
                    buffer.clear(channel, destStart, chunk);
                }
            }
        }

        position += chunk;
        done += chunk;
    }
    position_.store(position, std::memory_order_relaxed);
}

int64_t TrackSourceBase::getLengthSamples() const {
    return lengthSamples_;
}

int TrackSourceBase::getNumChannels() const {
    return numChannels_;
}

double TrackSourceBase::getSampleRate() const {
    return sampleRate_;
}

bool TrackSourceBase::isFinished() const {
    return position_.load(std::memory_order_relaxed) >= lengthSamples_;
}

void TrackSourceBase::setGain(float linearGain) {
    gain_.store(std::clamp(linearGain, 0.0f, 1.0f), std::memory_order_relaxed);
}

void TrackSourceBase::seekToSample(int64_t samplePos) {
    position_.store(std::max<int64_t>(0, samplePos), std::memory_order_relaxed);
}

int64_t TrackSourceBase::getPositionSamples() const {
    return position_.load(std::memory_order_relaxed);
}

void TrackSourceBase::setFormat(int64_t lengthSamples, int numChannels, double sampleRate) {
    lengthSamples_ = lengthSamples;
    numChannels_ = numChannels;
    sampleRate_ = sampleRate;
    scratch_.setSize(numChannels, kScratchBlockSamples);
    position_.store(0, std::memory_order_relaxed);
}

void TrackSourceBase::clearFormat() {
    lengthSamples_ = 0;
    numChannels_ = 0;
    sampleRate_ = 0.0;
    scratch_.setSize(0, 0);
    position_.store(0, std::memory_order_relaxed);
}

}  // namespace garageplaymate
