#pragma once

#include "core/SongTypes.h"

#include <expected>
#include <filesystem>
#include <string>

namespace garageplaymate {

struct ParseError {
    std::string message;
};

std::expected<Song, ParseError> parseSongYaml(const std::filesystem::path& songYamlPath,
                                              const std::filesystem::path& songFolderPath);

}  // namespace garageplaymate
