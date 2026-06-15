#include "storage/WavMetadataReader.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <string>

namespace garageplaymate {

std::optional<WavMetadata> readWavMetadata(const std::filesystem::path& wavPath, std::string* errorMessage) {
    drwav wav{};
    if (!drwav_init_file(&wav, wavPath.string().c_str(), nullptr)) {
        if (errorMessage != nullptr) {
            *errorMessage = "Failed to open or parse WAV file: " + wavPath.string();
        }
        return std::nullopt;
    }

    WavMetadata metadata;
    metadata.sampleRate = static_cast<int>(wav.sampleRate);
    metadata.channels = static_cast<int>(wav.channels);
    metadata.bitsPerSample = static_cast<int>(wav.bitsPerSample);

    if (metadata.sampleRate <= 0 || metadata.channels <= 0 || metadata.bitsPerSample <= 0) {
        drwav_uninit(&wav);
        if (errorMessage != nullptr) {
            *errorMessage = "Invalid WAV metadata in: " + wavPath.string();
        }
        return std::nullopt;
    }

    metadata.durationMs =
        (static_cast<std::int64_t>(wav.totalPCMFrameCount) * 1000) / metadata.sampleRate;
    drwav_uninit(&wav);
    return metadata;
}

}  // namespace garageplaymate
