#pragma once

#include "audio/IAudioOutput.h"

#include <juce_audio_devices/juce_audio_devices.h>

namespace garageplaymate::test {

// Device stand-in that reports a fixed format; never produces callbacks itself.
class FakeAudioIODevice : public juce::AudioIODevice {
public:
    FakeAudioIODevice(double sampleRate, int blockSize) : AudioIODevice("Fake", "Fake"), sampleRate_(sampleRate), blockSize_(blockSize) {}

    juce::StringArray getOutputChannelNames() override { return {"L", "R"}; }
    juce::StringArray getInputChannelNames() override { return {}; }
    juce::Array<double> getAvailableSampleRates() override { return {sampleRate_}; }
    juce::Array<int> getAvailableBufferSizes() override { return {blockSize_}; }
    int getDefaultBufferSize() override { return blockSize_; }
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return blockSize_; }
    double getCurrentSampleRate() override { return sampleRate_; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return juce::BigInteger(3); }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

private:
    double sampleRate_;
    int blockSize_;
};

// IAudioOutput whose "audio thread" is the test calling pump().
class FakeAudioOutput : public IAudioOutput {
public:
    explicit FakeAudioOutput(double sampleRate = 48000.0, int blockSize = 256)
        : device_(sampleRate, blockSize), output_(2, blockSize) {}

    bool ensureDeviceOpen(juce::String& errorMessage) override {
        if (failToOpen) {
            errorMessage = "No output device";
            return false;
        }
        return true;
    }
    double getCurrentSampleRate() const override { return const_cast<FakeAudioIODevice&>(device_).getCurrentSampleRate(); }
    int getCurrentBlockSize() const override { return const_cast<FakeAudioIODevice&>(device_).getCurrentBufferSizeSamples(); }
    int getNumOutputChannels() const override { return 2; }

    void addCallback(juce::AudioIODeviceCallback* callback) override {
        callback_ = callback;
        callback_->audioDeviceAboutToStart(&device_);
    }

    void removeCallback(juce::AudioIODeviceCallback* callback) override {
        if (callback_ == callback) {
            callback_->audioDeviceStopped();
            callback_ = nullptr;
        }
    }

    // Simulates `blocks` audio callbacks; returns the last block rendered.
    const juce::AudioBuffer<float>& pump(int blocks = 1) {
        for (int i = 0; i < blocks; ++i) {
            output_.clear();
            if (callback_ != nullptr) {
                callback_->audioDeviceIOCallbackWithContext(nullptr, 0, output_.getArrayOfWritePointers(), 2,
                                                            output_.getNumSamples(), {});
            }
        }
        return output_;
    }

    bool hasCallback() const { return callback_ != nullptr; }

    bool failToOpen = false;

private:
    FakeAudioIODevice device_;
    juce::AudioBuffer<float> output_;
    juce::AudioIODeviceCallback* callback_ = nullptr;
};

}  // namespace garageplaymate::test
