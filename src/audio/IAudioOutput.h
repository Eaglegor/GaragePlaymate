#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

namespace garageplaymate {

// Minimal output-device surface used by TransportController, so transport logic
// can be driven by a fake device in tests. Message thread only.
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    // Opens an output device if none is open yet. Returns false with a message on failure.
    virtual bool ensureDeviceOpen(juce::String& errorMessage) = 0;
    virtual double getCurrentSampleRate() const = 0;
    virtual int getCurrentBlockSize() const = 0;
    virtual int getNumOutputChannels() const = 0;

    // add/remove follow juce::AudioDeviceManager semantics: once removeCallback()
    // returns, the callback is no longer invoked from the audio thread.
    virtual void addCallback(juce::AudioIODeviceCallback* callback) = 0;
    virtual void removeCallback(juce::AudioIODeviceCallback* callback) = 0;
};

}  // namespace garageplaymate
