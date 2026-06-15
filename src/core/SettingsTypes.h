#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace garageplaymate {

enum class AudioDriverType { Wasapi, Asio };

enum class AudioLoadingMode { Stream, Preload };

struct AppSettings {
    std::optional<std::filesystem::path> dataRootOverride;
    AudioDriverType driverType = AudioDriverType::Wasapi;
    std::string outputDeviceId;
    int bufferSizeSamples = 512;
    AudioLoadingMode loadingMode = AudioLoadingMode::Stream;
    bool failureSimulationEnabled = false;
    float failureProbability = 0.10f;
    float failureLevel = 0.15f;
    int failureFadeMs = 300;
};

}  // namespace garageplaymate
