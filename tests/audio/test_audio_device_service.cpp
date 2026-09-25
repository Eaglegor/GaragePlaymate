#include <catch2/catch_test_macros.hpp>

#include "audio/AudioDeviceService.h"

TEST_CASE("AudioDeviceService maps WASAPI to an available device type", "[audio_device_service]") {
    garageplaymate::AudioDeviceService service;
    // Windows provides "Windows Audio"; other platforms map to their first native type.
    CHECK(service.isDriverTypeAvailable(garageplaymate::AudioDriverType::Wasapi));
}

TEST_CASE("AudioDeviceService does not open a device until asked", "[audio_device_service]") {
    garageplaymate::AudioDeviceService service;
    CHECK(service.getDeviceManager().getCurrentAudioDevice() == nullptr);
    CHECK(service.getCurrentSampleRate() == 0.0);
}

#if !JUCE_ASIO
TEST_CASE("AudioDeviceService falls back to WASAPI when ASIO is unavailable", "[audio_device_service]") {
    garageplaymate::AudioDeviceService service;
    REQUIRE_FALSE(service.isDriverTypeAvailable(garageplaymate::AudioDriverType::Asio));

    juce::String notification;
    service.onFallbackNotification = [&notification](const juce::String& message) { notification = message; };

    garageplaymate::AppSettings settings;
    settings.driverType = garageplaymate::AudioDriverType::Asio;
    settings.outputDeviceId = "Some ASIO Interface";

    juce::String error;
    const bool opened = service.applySettings(settings, error);

    CHECK(notification.contains("ASIO"));
    // Whether the WASAPI fallback opens depends on the machine having an output device.
    CHECK(opened == (service.getDeviceManager().getCurrentAudioDevice() != nullptr));
    if (opened) {
        CHECK(service.getCurrentDriverType() == garageplaymate::AudioDriverType::Wasapi);
    }
}
#endif
