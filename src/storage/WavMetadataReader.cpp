#include "storage/WavMetadataReader.h"

#include <fstream>
#include <string>

namespace garageplaymate {
namespace {

bool readExact(std::ifstream& input, void* buffer, std::size_t size) {
    input.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(input.gcount()) == size;
}

bool readUint32(std::ifstream& input, std::uint32_t& value) {
    return readExact(input, &value, sizeof(value));
}

bool readUint16(std::ifstream& input, std::uint16_t& value) {
    return readExact(input, &value, sizeof(value));
}

void skipBytes(std::ifstream& input, std::uint32_t count) {
    input.seekg(static_cast<std::streamoff>(count), std::ios::cur);
}

bool chunkIdsMatch(const char* actual, const char* expected) {
    return actual[0] == expected[0] && actual[1] == expected[1] && actual[2] == expected[2] &&
           actual[3] == expected[3];
}

}  // namespace

std::optional<WavMetadata> readWavMetadata(const std::filesystem::path& wavPath, std::string* errorMessage) {
    std::ifstream input(wavPath, std::ios::binary);
    if (!input.is_open()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Failed to open WAV file: " + wavPath.string();
        }
        return std::nullopt;
    }

    char riffHeader[12]{};
    if (!readExact(input, riffHeader, sizeof(riffHeader))) {
        if (errorMessage != nullptr) {
            *errorMessage = "WAV file too short: " + wavPath.string();
        }
        return std::nullopt;
    }

    if (!chunkIdsMatch(riffHeader, "RIFF") || !chunkIdsMatch(riffHeader + 8, "WAVE")) {
        if (errorMessage != nullptr) {
            *errorMessage = "Not a RIFF/WAVE file: " + wavPath.string();
        }
        return std::nullopt;
    }

    WavMetadata metadata;
    std::uint32_t dataSize = 0;
    bool hasFmt = false;
    bool hasData = false;

    while (input) {
        char chunkId[4]{};
        std::uint32_t chunkSize = 0;
        if (!readExact(input, chunkId, sizeof(chunkId)) || !readUint32(input, chunkSize)) {
            break;
        }

        if (chunkIdsMatch(chunkId, "fmt ")) {
            if (chunkSize < 16) {
                if (errorMessage != nullptr) {
                    *errorMessage = "Invalid fmt chunk in: " + wavPath.string();
                }
                return std::nullopt;
            }

            std::uint16_t audioFormat = 0;
            std::uint16_t channels = 0;
            std::uint32_t sampleRate = 0;
            std::uint16_t bitsPerSample = 0;

            if (!readUint16(input, audioFormat) || !readUint16(input, channels) || !readUint32(input, sampleRate)) {
                if (errorMessage != nullptr) {
                    *errorMessage = "Failed to read fmt chunk in: " + wavPath.string();
                }
                return std::nullopt;
            }

            skipBytes(input, 4);

            std::uint16_t blockAlign = 0;
            if (!readUint16(input, blockAlign) || !readUint16(input, bitsPerSample)) {
                if (errorMessage != nullptr) {
                    *errorMessage = "Failed to read bits per sample in: " + wavPath.string();
                }
                return std::nullopt;
            }

            if (chunkSize > 16) {
                skipBytes(input, chunkSize - 16);
            }

            if (audioFormat != 1) {
                if (errorMessage != nullptr) {
                    *errorMessage = "Unsupported WAV audio format in: " + wavPath.string();
                }
                return std::nullopt;
            }

            metadata.channels = static_cast<int>(channels);
            metadata.sampleRate = static_cast<int>(sampleRate);
            metadata.bitsPerSample = static_cast<int>(bitsPerSample);
            hasFmt = true;
        } else if (chunkIdsMatch(chunkId, "data")) {
            dataSize = chunkSize;
            skipBytes(input, chunkSize);
            hasData = true;
        } else {
            skipBytes(input, chunkSize);
        }

        if (chunkSize % 2 != 0) {
            skipBytes(input, 1);
        }
    }

    if (!hasFmt || !hasData) {
        if (errorMessage != nullptr) {
            *errorMessage = "Missing fmt or data chunk in: " + wavPath.string();
        }
        return std::nullopt;
    }

    if (metadata.sampleRate <= 0 || metadata.channels <= 0 || metadata.bitsPerSample <= 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "Invalid WAV metadata in: " + wavPath.string();
        }
        return std::nullopt;
    }

    const std::int64_t bytesPerSecond =
        static_cast<std::int64_t>(metadata.sampleRate) * metadata.channels * (metadata.bitsPerSample / 8);
    if (bytesPerSecond <= 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "Invalid WAV byte rate in: " + wavPath.string();
        }
        return std::nullopt;
    }

    metadata.durationMs = (static_cast<std::int64_t>(dataSize) * 1000) / bytesPerSecond;
    return metadata;
}

}  // namespace garageplaymate
