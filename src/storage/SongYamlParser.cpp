#include "storage/SongYamlParser.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace garageplaymate {
namespace {

ParseError makeError(std::string message) {
    return ParseError{std::move(message)};
}

bool requireScalar(const YAML::Node& node, const char* field, std::string& out, std::string& error) {
    const YAML::Node value = node[field];
    if (!value) {
        error = std::string("Missing required field '") + field + "'";
        return false;
    }
    if (!value.IsScalar()) {
        error = std::string("Field '") + field + "' must be a scalar";
        return false;
    }
    out = value.as<std::string>();
    return true;
}

bool requirePositiveInt(const YAML::Node& node, const char* field, int64_t& out, std::string& error) {
    const YAML::Node value = node[field];
    if (!value) {
        error = std::string("Missing required field '") + field + "'";
        return false;
    }
    if (!value.IsScalar()) {
        error = std::string("Field '") + field + "' must be a number";
        return false;
    }
    try {
        out = value.as<int64_t>();
    } catch (const YAML::Exception&) {
        error = std::string("Field '") + field + "' must be a number";
        return false;
    }
    if (out < 0) {
        error = std::string("Field '") + field + "' must be non-negative";
        return false;
    }
    return true;
}

std::expected<YAML::Node, ParseError> loadYamlFile(const std::filesystem::path& songYamlPath) {
    std::ifstream input(songYamlPath);
    if (!input.is_open()) {
        return std::unexpected(makeError("Failed to open song.yaml: " + songYamlPath.string()));
    }

    try {
        return YAML::Load(input);
    } catch (const YAML::Exception& exception) {
        return std::unexpected(makeError(std::string("YAML parse error: ") + exception.what()));
    }
}

std::expected<void, ParseError> parseTimeSignature(const YAML::Node& root, Song& song) {
    const YAML::Node value = root["timeSignature"];
    if (!value) {
        return {};
    }
    if (!value.IsSequence() || value.size() != 2) {
        return std::unexpected(makeError("Field 'timeSignature' must be an array of two integers"));
    }

    try {
        const int numerator = value[0].as<int>();
        const int denominator = value[1].as<int>();
        if (numerator <= 0 || denominator <= 0) {
            return std::unexpected(makeError("Field 'timeSignature' values must be positive"));
        }
        song.timeSignature = std::make_pair(numerator, denominator);
    } catch (const YAML::Exception&) {
        return std::unexpected(makeError("Field 'timeSignature' must be an array of two integers"));
    }

    return {};
}

std::expected<void, ParseError> parseTracks(const YAML::Node& root,
                                            const std::filesystem::path& songFolderPath,
                                            Song& song) {
    const YAML::Node tracksNode = root["tracks"];
    if (!tracksNode) {
        return std::unexpected(makeError("Missing required field 'tracks'"));
    }
    if (!tracksNode.IsSequence() || tracksNode.size() == 0) {
        return std::unexpected(makeError("Field 'tracks' must be a non-empty sequence"));
    }

    std::unordered_set<std::string> seenIds;

    for (std::size_t index = 0; index < tracksNode.size(); ++index) {
        const YAML::Node trackNode = tracksNode[index];
        if (!trackNode.IsMap()) {
            return std::unexpected(makeError("Each track entry must be a mapping"));
        }

        TrackDef track;
        std::string error;
        if (!requireScalar(trackNode, "id", track.id, error)) {
            return std::unexpected(makeError("tracks[" + std::to_string(index) + "]: " + error));
        }
        if (!requireScalar(trackNode, "name", track.name, error)) {
            return std::unexpected(makeError("tracks[" + std::to_string(index) + "]: " + error));
        }

        std::string folderRelative;
        if (!requireScalar(trackNode, "folder", folderRelative, error)) {
            return std::unexpected(makeError("tracks[" + std::to_string(index) + "]: " + error));
        }

        if (!seenIds.insert(track.id).second) {
            return std::unexpected(makeError("Duplicate track id: " + track.id));
        }

        const YAML::Node volumeNode = trackNode["defaultVolume"];
        if (volumeNode) {
            if (!volumeNode.IsScalar()) {
                return std::unexpected(
                    makeError("tracks[" + std::to_string(index) + "]: Field 'defaultVolume' must be a number"));
            }
            try {
                track.defaultVolume = volumeNode.as<float>();
            } catch (const YAML::Exception&) {
                return std::unexpected(
                    makeError("tracks[" + std::to_string(index) + "]: Field 'defaultVolume' must be a number"));
            }
        }

        track.folderPath = songFolderPath / folderRelative;
        song.tracks.push_back(std::move(track));
    }

    return {};
}

std::expected<void, ParseError> parseSections(const YAML::Node& root, Song& song) {
    const YAML::Node sectionsNode = root["sections"];
    if (!sectionsNode) {
        return {};
    }
    if (!sectionsNode.IsSequence()) {
        return std::unexpected(makeError("Field 'sections' must be a sequence"));
    }

    std::unordered_set<std::string> seenIds;

    for (std::size_t index = 0; index < sectionsNode.size(); ++index) {
        const YAML::Node sectionNode = sectionsNode[index];
        if (!sectionNode.IsMap()) {
            return std::unexpected(makeError("Each section entry must be a mapping"));
        }

        Section section;
        std::string error;
        if (!requireScalar(sectionNode, "id", section.id, error)) {
            return std::unexpected(makeError("sections[" + std::to_string(index) + "]: " + error));
        }
        if (!requireScalar(sectionNode, "name", section.name, error)) {
            return std::unexpected(makeError("sections[" + std::to_string(index) + "]: " + error));
        }
        if (!requirePositiveInt(sectionNode, "startMs", section.startMs, error)) {
            return std::unexpected(makeError("sections[" + std::to_string(index) + "]: " + error));
        }

        if (!seenIds.insert(section.id).second) {
            return std::unexpected(makeError("Duplicate section id: " + section.id));
        }

        song.sections.push_back(std::move(section));
    }

    std::sort(song.sections.begin(), song.sections.end(),
              [](const Section& left, const Section& right) { return left.startMs < right.startMs; });

    return {};
}

}  // namespace

std::expected<Song, ParseError> parseSongYaml(const std::filesystem::path& songYamlPath,
                                              const std::filesystem::path& songFolderPath) {
    auto yamlResult = loadYamlFile(songYamlPath);
    if (!yamlResult) {
        return std::unexpected(yamlResult.error());
    }

    const YAML::Node& root = *yamlResult;
    if (!root.IsMap()) {
        return std::unexpected(makeError("song.yaml root must be a mapping"));
    }

    Song song;
    song.folderPath = songYamlPath.parent_path();

    std::string error;
    if (!requireScalar(root, "id", song.id, error)) {
        return std::unexpected(makeError(error));
    }
    if (!requireScalar(root, "title", song.title, error)) {
        return std::unexpected(makeError(error));
    }

    const YAML::Node bpmNode = root["bpm"];
    if (bpmNode) {
        if (!bpmNode.IsScalar()) {
            return std::unexpected(makeError("Field 'bpm' must be a number"));
        }
        try {
            const int bpm = bpmNode.as<int>();
            if (bpm <= 0) {
                return std::unexpected(makeError("Field 'bpm' must be positive"));
            }
            song.bpm = bpm;
        } catch (const YAML::Exception&) {
            return std::unexpected(makeError("Field 'bpm' must be a number"));
        }
    }

    if (auto timeSignatureResult = parseTimeSignature(root, song); !timeSignatureResult) {
        return std::unexpected(timeSignatureResult.error());
    }

    if (auto tracksResult = parseTracks(root, songFolderPath, song); !tracksResult) {
        return std::unexpected(tracksResult.error());
    }

    if (auto sectionsResult = parseSections(root, song); !sectionsResult) {
        return std::unexpected(sectionsResult.error());
    }

    return song;
}

}  // namespace garageplaymate
