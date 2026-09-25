#pragma once

#include "audio/IAudioOutput.h"
#include "core/SettingsTypes.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <functional>
#include <string>
#include <vector>

namespace garageplaymate {

struct AudioDeviceInfo {
    std::string deviceId;  // JUCE device name; stable identifier stored in AppSettings
    std::string displayName;
    AudioDriverType driverType = AudioDriverType::Wasapi;
};

// Wraps juce::AudioDeviceManager. WASAPI (shared mode, "Windows Audio") is the
// default; ASIO is used only when requested and available, otherwise the service
// falls back to the last-used or default WASAPI device and notifies the caller.
// No device is opened until the first call that needs one (lazy init).
//
// On non-Windows development builds "WASAPI" maps to the platform's first device
// type so the app and tests still run. All methods: message thread only.
class AudioDeviceService : public IAudioOutput {
public:
    static constexpr int kDefaultBufferSizeSamples = 512;

    AudioDeviceService();
    ~AudioDeviceService() override;

    juce::AudioDeviceManager& getDeviceManager();

    bool isDriverTypeAvailable(AudioDriverType type);
    std::vector<AudioDeviceInfo> listOutputDevices(AudioDriverType type);

    // Opens the device described by the settings. ASIO failure → WASAPI fallback
    // plus onFallbackNotification; returns true when some device ended up open.
    bool applySettings(const AppSettings& settings, juce::String& errorMessage);

    // System default WASAPI output, stereo, 512-sample buffer (FR-PLAY-07).
    bool initializeDefaultWasapi(juce::String& errorMessage);

    // Current device as settings fields, e.g. to persist what first launch picked.
    AudioDriverType getCurrentDriverType() const;
    std::string getCurrentOutputDeviceId() const;

    std::function<void(const juce::String& message)> onFallbackNotification;

    // IAudioOutput
    bool ensureDeviceOpen(juce::String& errorMessage) override;
    double getCurrentSampleRate() const override;
    int getCurrentBlockSize() const override;
    int getNumOutputChannels() const override;
    void addCallback(juce::AudioIODeviceCallback* callback) override;
    void removeCallback(juce::AudioIODeviceCallback* callback) override;

private:
    juce::AudioIODeviceType* findDeviceType(AudioDriverType type);
    bool openOutputDevice(AudioDriverType type, const juce::String& deviceName, int bufferSizeSamples,
                          juce::String& errorMessage);
    bool fallBackToWasapi(const juce::String& reason, int bufferSizeSamples, juce::String& errorMessage);

    juce::AudioDeviceManager deviceManager_;
    juce::String lastWasapiDeviceName_;
    AudioDriverType currentDriverType_ = AudioDriverType::Wasapi;
};

}  // namespace garageplaymate
