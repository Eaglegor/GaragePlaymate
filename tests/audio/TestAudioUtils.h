#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <filesystem>
#include <functional>
#include <random>
#include <string>

namespace garageplaymate::test {

// Unique temporary directory removed at scope exit.
struct TempDir {
    std::filesystem::path path;

    TempDir() {
        std::random_device device;
        path = std::filesystem::temp_directory_path() /
               ("garageplaymate-audio-test-" + std::to_string(device()) + std::to_string(device()));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code errorCode;
        std::filesystem::remove_all(path, errorCode);
    }
};

using SampleGenerator = std::function<float(int channel, int sampleIndex)>;

// Deterministic, non-repeating test signal in [-0.5, 0.5).
inline float rampSample(int channel, int sampleIndex) {
    return static_cast<float>((sampleIndex + channel * 7) % 1000) / 1000.0f - 0.5f;
}

// Writes a 32-bit float WAV so samples round-trip exactly.
inline bool writeTestWav(const std::filesystem::path& path, double sampleRate, int numChannels, int numSamples,
                         const SampleGenerator& generator = rampSample) {
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    for (int channel = 0; channel < numChannels; ++channel) {
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(channel, i, generator(channel, i));
        }
    }

    const juce::File file(path.string());
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    if (stream == nullptr) {
        return false;
    }

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        format.createWriterFor(stream.get(), sampleRate, static_cast<unsigned int>(numChannels), 32, {}, 0));
    if (writer == nullptr) {
        return false;
    }
    stream.release();  // owned by the writer now
    return writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
}

}  // namespace garageplaymate::test
