#include "audio/StreamingTrackSource.h"

namespace garageplaymate {

StreamingTrackSource::StreamingTrackSource(juce::TimeSliceThread& readAheadThread, int audioReadTimeoutMs)
    : readAheadThread_(readAheadThread), audioReadTimeoutMs_(audioReadTimeoutMs) {}

StreamingTrackSource::~StreamingTrackSource() {
    release();
}

bool StreamingTrackSource::prepare(const std::filesystem::path& wavPath) {
    release();

    std::unique_ptr<juce::AudioFormatReader> fileReader = createWavReader(wavPath);
    if (fileReader == nullptr) {
        juce::Logger::writeToLog("[StreamingTrackSource] Cannot open WAV: " + toJuceFile(wavPath).getFullPathName());
        return false;
    }

    const auto lengthSamples = static_cast<int64_t>(fileReader->lengthInSamples);
    const auto numChannels = static_cast<int>(fileReader->numChannels);
    const double sampleRate = fileReader->sampleRate;
    const int samplesToBuffer = static_cast<int>(sampleRate * kReadAheadSeconds);

    reader_ = std::make_unique<juce::BufferingAudioReader>(fileReader.release(), readAheadThread_, samplesToBuffer);
    setFormat(lengthSamples, numChannels, sampleRate);

    // Wait (on the message thread) for the first block so playback starts with audio,
    // then switch to non-blocking reads for the audio thread.
    juce::AudioBuffer<float> firstBlock(numChannels, kScratchBlockSamples);
    reader_->setReadTimeout(kInitialBufferTimeoutMs);
    reader_->read(firstBlock.getArrayOfWritePointers(), numChannels, 0, kScratchBlockSamples);
    reader_->setReadTimeout(audioReadTimeoutMs_);
    return true;
}

void StreamingTrackSource::release() {
    reader_.reset();
    clearFormat();
}

void StreamingTrackSource::readSamples(juce::AudioBuffer<float>& scratch, int64_t sourcePosition, int numSamples) {
    if (reader_ == nullptr) {
        scratch.clear(0, numSamples);
        return;
    }
    reader_->read(scratch.getArrayOfWritePointers(), scratch.getNumChannels(), sourcePosition, numSamples);
}

}  // namespace garageplaymate
