#include "storage/SongYamlParser.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <unordered_set>

namespace garageplaymate {
namespace {

[[noreturn]] void throwParseError(std::string message) {
    throw ParseError(std::move(message));
}

void logWarning(const std::string& message) {
    std::cerr << "[SongYamlParser] " << message << '\n';
}

std::string requireScalar(const YAML::Node& node, const char* field) {
    const YAML::Node value = node[field];
    if (!value) {
        throwParseError(std::string("Missing required field '") + field + "'");
    }
    if (!value.IsScalar()) {
        throwParseError(std::string("Field '") + field + "' must be a scalar");
    }
    return value.as<std::string>();
}

int64_t requirePositiveInt(const YAML::Node& node, const char* field) {
    const YAML::Node value = node[field];
    if (!value) {
        throwParseError(std::string("Missing required field '") + field + "'");
    }
    if (!value.IsScalar()) {
        throwParseError(std::string("Field '") + field + "' must be a number");
    }
    try {
        const int64_t parsed = value.as<int64_t>();
        if (parsed < 0) {
            throwParseError(std::string("Field '") + field + "' must be non-negative");
        }
        return parsed;
    } catch (const YAML::Exception&) {
        throwParseError(std::string("Field '") + field + "' must be a number");
    }
}

template <typename T>
std::optional<T> tryAsScalar(const YAML::Node& node, const char* fieldName) {
    if (!node) {
        return std::nullopt;
    }
    if (!node.IsScalar()) {
        logWarning(std::string("Field '") + fieldName + "' must be a scalar; ignoring");
        return std::nullopt;
    }
    try {
        return node.as<T>();
    } catch (const YAML::Exception&) {
        logWarning(std::string("Field '") + fieldName + "' has an invalid value; ignoring");
        return std::nullopt;
    }
}

YAML::Node loadYamlFile(const std::filesystem::path& songYamlPath) {
    std::ifstream input(songYamlPath);
    if (!input.is_open()) {
        throwParseError("Failed to open song.yaml: " + songYamlPath.string());
    }

    try {
        return YAML::Load(input);
    } catch (const YAML::Exception& exception) {
        throwParseError(std::string("YAML parse error: ") + exception.what());
    }
}

void parseTimeSignature(const YAML::Node& root, Song& song) {
    const YAML::Node value = root["timeSignature"];
    if (!value) {
        return;
    }
    if (!value.IsSequence() || value.size() != 2) {
        throwParseError("Field 'timeSignature' must be an array of two integers");
    }

    try {
        const int numerator = value[0].as<int>();
        const int denominator = value[1].as<int>();
        if (numerator <= 0 || denominator <= 0) {
            throwParseError("Field 'timeSignature' values must be positive");
        }
        song.timeSignature = std::make_pair(numerator, denominator);
    } catch (const YAML::Exception&) {
        throwParseError("Field 'timeSignature' must be an array of two integers");
    }
}

void parseTracks(const YAML::Node& root, const std::filesystem::path& songFolderPath, Song& song) {
    const YAML::Node tracksNode = root["tracks"];
    if (!tracksNode) {
        throwParseError("Missing required field 'tracks'");
    }
    if (!tracksNode.IsSequence() || tracksNode.size() == 0) {
        throwParseError("Field 'tracks' must be a non-empty sequence");
    }

    std::unordered_set<std::string> seenIds;

    for (std::size_t index = 0; index < tracksNode.size(); ++index) {
        const YAML::Node trackNode = tracksNode[index];
        if (!trackNode.IsMap()) {
            throwParseError("Each track entry must be a mapping");
        }

        TrackDef track;
        track.id = requireScalar(trackNode, "id");
        track.name = requireScalar(trackNode, "name");
        const std::string folderRelative = requireScalar(trackNode, "folder");

        if (!seenIds.insert(track.id).second) {
            throwParseError("Duplicate track id: " + track.id);
        }

        if (auto volume = tryAsScalar<float>(trackNode["defaultVolume"], "defaultVolume")) {
            track.defaultVolume = *volume;
        }

        track.folderPath = songFolderPath / folderRelative;
        song.tracks.push_back(std::move(track));
    }
}

void parseSections(const YAML::Node& root, Song& song) {
    const YAML::Node sectionsNode = root["sections"];
    if (!sectionsNode) {
        return;
    }
    if (!sectionsNode.IsSequence()) {
        throwParseError("Field 'sections' must be a sequence");
    }

    std::unordered_set<std::string> seenIds;

    for (std::size_t index = 0; index < sectionsNode.size(); ++index) {
        const YAML::Node sectionNode = sectionsNode[index];
        if (!sectionNode.IsMap()) {
            throwParseError("Each section entry must be a mapping");
        }

        Section section;
        section.id = requireScalar(sectionNode, "id");
        section.name = requireScalar(sectionNode, "name");
        section.startMs = requirePositiveInt(sectionNode, "startMs");

        if (!seenIds.insert(section.id).second) {
            throwParseError("Duplicate section id: " + section.id);
        }

        song.sections.push_back(std::move(section));
    }

    std::sort(song.sections.begin(), song.sections.end(),
              [](const Section& left, const Section& right) { return left.startMs < right.startMs; });
}

}  // namespace

Song parseSongYaml(const std::filesystem::path& songYamlPath,
                   const std::filesystem::path& songFolderPath) {
    const YAML::Node root = loadYamlFile(songYamlPath);
    if (!root.IsMap()) {
        throwParseError("song.yaml root must be a mapping");
    }

    Song song;
    song.folderPath = songYamlPath.parent_path();
    song.id = requireScalar(root, "id");
    song.title = requireScalar(root, "title");

    if (auto bpm = tryAsScalar<int>(root["bpm"], "bpm")) {
        if (*bpm > 0) {
            song.bpm = *bpm;
        } else {
            logWarning("Field 'bpm' must be positive; ignoring");
        }
    }

    parseTimeSignature(root, song);
    parseTracks(root, songFolderPath, song);
    parseSections(root, song);

    return song;
}

}  // namespace garageplaymate
