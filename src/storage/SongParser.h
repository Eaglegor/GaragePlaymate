#pragma once

#include "core/SongTypes.h"

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace garageplaymate {

struct ParseError : std::exception {
    std::string message;

    explicit ParseError(std::string message) : message(std::move(message)) {}

    const char* what() const noexcept override { return message.c_str(); }
};

struct ParseWarning {
    std::string path;
    std::string message;
};

struct SongParseResult {
    Song song;
    std::vector<ParseWarning> warnings;
};

SongParseResult parseSong(const std::filesystem::path& songFolderPath);

}  // namespace garageplaymate
