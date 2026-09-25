#include "storage/DataPathResolver.h"

#ifndef GARAGEPLAYMATE_PORTABLE_BUILD
#define GARAGEPLAYMATE_PORTABLE_BUILD 0
#endif

namespace garageplaymate {

DistributionMode DataPathResolver::getBuildDistributionMode() {
#if GARAGEPLAYMATE_PORTABLE_BUILD
    return DistributionMode::Portable;
#else
    return DistributionMode::Installed;
#endif
}

std::filesystem::path DataPathResolver::getDefaultDataRoot(DistributionMode mode, const PlatformPaths& platformPaths) {
    if (mode == DistributionMode::Portable) {
        return platformPaths.executableFile.parent_path() / "data";
    }
    return platformPaths.documentsDir / "GaragePlaymate";
}

std::filesystem::path DataPathResolver::getDefaultDataRoot(const PlatformPaths& platformPaths) {
    return getDefaultDataRoot(getBuildDistributionMode(), platformPaths);
}

std::filesystem::path DataPathResolver::getEffectiveDataRoot(const std::optional<std::filesystem::path>& userOverride,
                                                             const PlatformPaths& platformPaths) {
    if (userOverride.has_value() && !userOverride.value().empty()) {
        return userOverride.value();
    }
    return getDefaultDataRoot(platformPaths);
}

std::filesystem::path DataPathResolver::getSongsDirectory(const std::filesystem::path& effectiveDataRoot) {
    return effectiveDataRoot / "songs";
}

std::filesystem::path DataPathResolver::getDatabasePath(const std::filesystem::path& effectiveDataRoot) {
    return effectiveDataRoot / "garageplaymate.db";
}

}  // namespace garageplaymate
