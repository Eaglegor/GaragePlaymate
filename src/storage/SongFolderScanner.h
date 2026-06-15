#pragma once

#include "core/SongTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace garageplaymate {

struct ScanWarning {
    std::string path;
    std::string message;
};

struct ScanResult {
    std::vector<Song> songs;
    std::vector<ScanWarning> warnings;
};

ScanResult scanLibrary(const std::filesystem::path& dataRoot);

}  // namespace garageplaymate
