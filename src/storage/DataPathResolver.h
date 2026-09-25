#pragma once

#include <filesystem>
#include <optional>

namespace garageplaymate {

enum class DistributionMode { Portable, Installed };

// Platform locations supplied by the app layer (e.g. from juce::File special
// locations) so the storage layer stays free of JUCE and Win32 calls.
struct PlatformPaths {
    std::filesystem::path executableFile;  // full path to GaragePlaymate.exe
    std::filesystem::path documentsDir;    // user's Documents folder
};

// Pure path computation; safe to call from any thread. Never touches the disk.
class DataPathResolver {
public:
    // Distribution mode selected at compile time via GARAGEPLAYMATE_PORTABLE_BUILD.
    static DistributionMode getBuildDistributionMode();

    // Portable → {exe_dir}/data; Installed → {Documents}/GaragePlaymate
    static std::filesystem::path getDefaultDataRoot(DistributionMode mode, const PlatformPaths& platformPaths);
    static std::filesystem::path getDefaultDataRoot(const PlatformPaths& platformPaths);

    // A non-empty user override wins over the build default.
    static std::filesystem::path getEffectiveDataRoot(const std::optional<std::filesystem::path>& userOverride,
                                                      const PlatformPaths& platformPaths);

    static std::filesystem::path getSongsDirectory(const std::filesystem::path& effectiveDataRoot);
    static std::filesystem::path getDatabasePath(const std::filesystem::path& effectiveDataRoot);
};

}  // namespace garageplaymate
