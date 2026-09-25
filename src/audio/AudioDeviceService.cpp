#include "audio/AudioDeviceService.h"

namespace garageplaymate {
namespace {

constexpr const char* kWasapiSharedTypeName = "Windows Audio";
constexpr const char* kAsioTypeName = "ASIO";

void logDevice(const juce::String& message) {
    juce::Logger::writeToLog("[AudioDeviceService] " + message);
}

}  // namespace

AudioDeviceService::AudioDeviceService() = default;

AudioDeviceService::~AudioDeviceService() {
    deviceManager_.closeAudioDevice();
}

juce::AudioDeviceManager& AudioDeviceService::getDeviceManager() {
    return deviceManager_;
}

bool AudioDeviceService::isDriverTypeAvailable(AudioDriverType type) {
    return findDeviceType(type) != nullptr;
}

std::vector<AudioDeviceInfo> AudioDeviceService::listOutputDevices(AudioDriverType type) {
    std::vector<AudioDeviceInfo> devices;
    juce::AudioIODeviceType* deviceType = findDeviceType(type);
    if (deviceType == nullptr) {
        return devices;
    }

    deviceType->scanForDevices();
    for (const juce::String& name : deviceType->getDeviceNames(false)) {
        devices.push_back(AudioDeviceInfo{name.toStdString(), name.toStdString(), type});
    }
    return devices;
}

bool AudioDeviceService::applySettings(const AppSettings& settings, juce::String& errorMessage) {
    const juce::String deviceName(settings.outputDeviceId);
    const int bufferSize = settings.bufferSizeSamples > 0 ? settings.bufferSizeSamples : kDefaultBufferSizeSamples;

    if (settings.driverType == AudioDriverType::Asio) {
        juce::String asioError;
        if (findDeviceType(AudioDriverType::Asio) == nullptr) {
            asioError = "ASIO is not available in this build or on this system";
        } else if (openOutputDevice(AudioDriverType::Asio, deviceName, bufferSize, asioError)) {
            return true;
        }
        return fallBackToWasapi(asioError, bufferSize, errorMessage);
    }

    return openOutputDevice(AudioDriverType::Wasapi, deviceName, bufferSize, errorMessage);
}

bool AudioDeviceService::initializeDefaultWasapi(juce::String& errorMessage) {
    return openOutputDevice(AudioDriverType::Wasapi, {}, kDefaultBufferSizeSamples, errorMessage);
}

AudioDriverType AudioDeviceService::getCurrentDriverType() const {
    return currentDriverType_;
}

std::string AudioDeviceService::getCurrentOutputDeviceId() const {
    if (auto* device = deviceManager_.getCurrentAudioDevice()) {
        return device->getName().toStdString();
    }
    return {};
}

bool AudioDeviceService::ensureDeviceOpen(juce::String& errorMessage) {
    if (deviceManager_.getCurrentAudioDevice() != nullptr) {
        return true;
    }
    return initializeDefaultWasapi(errorMessage);
}

double AudioDeviceService::getCurrentSampleRate() const {
    auto* device = deviceManager_.getCurrentAudioDevice();
    return device != nullptr ? device->getCurrentSampleRate() : 0.0;
}

int AudioDeviceService::getCurrentBlockSize() const {
    auto* device = deviceManager_.getCurrentAudioDevice();
    return device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
}

int AudioDeviceService::getNumOutputChannels() const {
    auto* device = deviceManager_.getCurrentAudioDevice();
    return device != nullptr ? device->getActiveOutputChannels().countNumberOfSetBits() : 0;
}

void AudioDeviceService::addCallback(juce::AudioIODeviceCallback* callback) {
    deviceManager_.addAudioCallback(callback);
}

void AudioDeviceService::removeCallback(juce::AudioIODeviceCallback* callback) {
    deviceManager_.removeAudioCallback(callback);
}

juce::AudioIODeviceType* AudioDeviceService::findDeviceType(AudioDriverType type) {
    const auto& types = deviceManager_.getAvailableDeviceTypes();
    const juce::String wanted = type == AudioDriverType::Asio ? kAsioTypeName : kWasapiSharedTypeName;
    for (auto* deviceType : types) {
        if (deviceType->getTypeName() == wanted) {
            return deviceType;
        }
    }

#if !JUCE_WINDOWS
    // Development builds on other platforms: treat the first native type as "WASAPI".
    if (type == AudioDriverType::Wasapi && !types.isEmpty()) {
        return types.getFirst();
    }
#endif
    return nullptr;
}

bool AudioDeviceService::openOutputDevice(AudioDriverType type, const juce::String& deviceName,
                                          int bufferSizeSamples, juce::String& errorMessage) {
    juce::AudioIODeviceType* deviceType = findDeviceType(type);
    if (deviceType == nullptr) {
        errorMessage = "Audio driver type is not available";
        return false;
    }

    deviceType->scanForDevices();
    const juce::StringArray names = deviceType->getDeviceNames(false);
    if (names.isEmpty()) {
        errorMessage = "No " + deviceType->getTypeName() + " output devices found";
        return false;
    }

    juce::String chosenName = deviceName;
    if (chosenName.isEmpty() || !names.contains(chosenName)) {
        if (chosenName.isNotEmpty()) {
            logDevice("Output device '" + chosenName + "' not found; using system default");
        }
        const int defaultIndex = deviceType->getDefaultDeviceIndex(false);
        chosenName = names[juce::jlimit(0, names.size() - 1, defaultIndex)];
    }

    deviceManager_.setCurrentAudioDeviceType(deviceType->getTypeName(), true);

    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager_.getAudioDeviceSetup();
    setup.outputDeviceName = chosenName;
    setup.inputDeviceName = {};
    setup.useDefaultInputChannels = false;
    setup.inputChannels.clear();
    setup.useDefaultOutputChannels = true;
    setup.bufferSize = bufferSizeSamples;

    errorMessage = deviceManager_.setAudioDeviceSetup(setup, true);
    if (errorMessage.isEmpty() && deviceManager_.getCurrentAudioDevice() == nullptr) {
        errorMessage = "Failed to open output device '" + chosenName + "'";
    }
    if (errorMessage.isNotEmpty()) {
        logDevice("Failed to open " + deviceType->getTypeName() + " device '" + chosenName + "': " + errorMessage);
        return false;
    }

    currentDriverType_ = type;
    if (type == AudioDriverType::Wasapi) {
        lastWasapiDeviceName_ = chosenName;
    }

    auto* device = deviceManager_.getCurrentAudioDevice();
    logDevice("Opened " + deviceType->getTypeName() + " device '" + chosenName + "' at " +
              juce::String(device->getCurrentSampleRate()) + " Hz, buffer " +
              juce::String(device->getCurrentBufferSizeSamples()));
    return true;
}

bool AudioDeviceService::fallBackToWasapi(const juce::String& reason, int bufferSizeSamples,
                                          juce::String& errorMessage) {
    logDevice("ASIO unavailable (" + reason + "); falling back to WASAPI");

    const bool opened = openOutputDevice(AudioDriverType::Wasapi, lastWasapiDeviceName_, bufferSizeSamples,
                                         errorMessage);
    if (onFallbackNotification) {
        juce::String message = "The ASIO device could not be opened (" + reason + ").";
        message += opened ? " Switched to Windows Audio output." : " No fallback output device is available.";
        onFallbackNotification(message);
    }
    return opened;
}

}  // namespace garageplaymate
