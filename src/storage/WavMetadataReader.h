#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace garageplaymate {

struct WavMetadata {
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 0;
    int64_t durationMs = 0;
};

std::optional<WavMetadata> readWavMetadata(const std::filesystem::path& wavPath, std::string* errorMessage = nullptr);

}  // namespace garageplaymate
