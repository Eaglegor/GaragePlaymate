#pragma once

#include "core/SongTypes.h"

#include <exception>
#include <filesystem>
#include <string>

namespace garageplaymate {

struct ParseError : std::exception {
    std::string message;

    explicit ParseError(std::string message) : message(std::move(message)) {}

    const char* what() const noexcept override { return message.c_str(); }
};

Song parseSongYaml(const std::filesystem::path& songYamlPath,
                   const std::filesystem::path& songFolderPath);

}  // namespace garageplaymate
