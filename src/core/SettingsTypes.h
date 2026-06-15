#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace garageplaymate {

enum class AudioDriverType { Wasapi, Asio };

enum class AudioLoadingMode { Stream, Preload };

struct VolumeDropFailureConfig {
    float probability = 0.10f;
    float failureLevel = 0.15f;
    int failureFadeMs = 300;
};

using FailureConfig = std::variant<VolumeDropFailureConfig>;

struct AppSettings {
    std::optional<std::filesystem::path> dataRootOverride;
    AudioDriverType driverType = AudioDriverType::Wasapi;
    std::string outputDeviceId;
    int bufferSizeSamples = 512;
    AudioLoadingMode loadingMode = AudioLoadingMode::Stream;
    std::vector<FailureConfig> failureConfigs;
};

}  // namespace garageplaymate
