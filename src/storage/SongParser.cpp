#include "storage/SongParser.h"

#include "storage/WavMetadataReader.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
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
    std::cerr << "[SongParser] " << message << '\n';
}

void addWarning(std::vector<ParseWarning>& warnings, const std::filesystem::path& path, std::string message) {
    warnings.push_back(ParseWarning{path.string(), std::move(message)});
}

bool hasWavExtension(const std::filesystem::path& filePath) {
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension == ".wav";
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

YAML::Node loadYamlFile(const std::filesystem::path& yamlPath) {
    std::ifstream input(yamlPath);
    if (!input.is_open()) {
        throwParseError("Failed to open YAML file: " + yamlPath.string());
    }

    try {
        return YAML::Load(input);
    } catch (const YAML::Exception& exception) {
        throwParseError(std::string("YAML parse error in ") + yamlPath.string() + ": " + exception.what());
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

TrackDef parseTrackYaml(const std::filesystem::path& trackYamlPath,
                        const std::filesystem::path& trackFolderPath) {
    const YAML::Node root = loadYamlFile(trackYamlPath);
    if (!root.IsMap()) {
        throwParseError(trackYamlPath.string() + ": root must be a mapping");
    }

    TrackDef track;
    track.id = trackFolderPath.filename().string();
    track.name = requireScalar(root, "name");
    if (auto volume = tryAsScalar<float>(root["defaultVolume"], "defaultVolume")) {
        track.defaultVolume = volume.value();
    }
    track.folderPath = trackFolderPath;
    return track;
}

void discoverTracks(const std::filesystem::path& songFolderPath, Song& song) {
    const std::filesystem::path tracksRoot = songFolderPath / "tracks";
    if (!std::filesystem::is_directory(tracksRoot)) {
        throwParseError("Missing tracks directory: " + tracksRoot.string());
    }

    std::vector<TrackDef> tracks;

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(tracksRoot)) {
        if (!entry.is_directory()) {
            continue;
        }

        const std::filesystem::path trackYamlPath = entry.path() / "track.yaml";
        if (!std::filesystem::is_regular_file(trackYamlPath)) {
            continue;
        }

        tracks.push_back(parseTrackYaml(trackYamlPath, entry.path()));
    }

    if (tracks.empty()) {
        throwParseError("No tracks found under " + tracksRoot.string());
    }

    std::sort(tracks.begin(), tracks.end(),
              [](const TrackDef& left, const TrackDef& right) { return left.id < right.id; });
    song.tracks = std::move(tracks);
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

void enumerateTrackTakes(TrackDef& track, std::vector<ParseWarning>& warnings) {
    std::vector<TakeFile> takes;

    std::error_code errorCode;
    if (!std::filesystem::is_directory(track.folderPath, errorCode)) {
        addWarning(warnings, track.folderPath, "Track folder is missing or unreadable");
        track.takes = std::move(takes);
        return;
    }

    std::error_code iterationError;
    std::filesystem::directory_iterator iterator(track.folderPath, iterationError);
    if (iterationError) {
        addWarning(warnings, track.folderPath, "Failed to read track folder: " + iterationError.message());
        track.takes = std::move(takes);
        return;
    }

    for (; iterator != std::filesystem::directory_iterator(); iterator.increment(iterationError)) {
        if (iterationError) {
            addWarning(warnings, track.folderPath, "Failed to read track folder: " + iterationError.message());
            break;
        }

        const std::filesystem::directory_entry& entry = *iterator;
        if (!entry.is_regular_file(errorCode)) {
            if (errorCode) {
                addWarning(warnings, entry.path(), "Failed to inspect file: " + errorCode.message());
                errorCode.clear();
            }
            continue;
        }

        if (!hasWavExtension(entry.path())) {
            addWarning(warnings, entry.path(), "Ignoring non-WAV file");
            continue;
        }

        TakeFile take;
        take.filename = entry.path().filename().string();

        std::string metadataError;
        if (const std::optional<WavMetadata> metadata = readWavMetadata(entry.path(), &metadataError)) {
            take.durationMs = metadata.value().durationMs;
        } else {
            addWarning(warnings, entry.path(), metadataError.empty() ? "Failed to read WAV metadata" : metadataError);
            take.durationMs = 0;
        }

        takes.push_back(std::move(take));
    }

    std::sort(takes.begin(), takes.end(),
              [](const TakeFile& left, const TakeFile& right) { return left.filename < right.filename; });
    track.takes = std::move(takes);
}

void populateSongTakes(Song& song, std::vector<ParseWarning>& warnings) {
    for (TrackDef& track : song.tracks) {
        enumerateTrackTakes(track, warnings);
    }
}

Song parseSongManifest(const std::filesystem::path& songYamlPath,
                       const std::filesystem::path& songFolderPath) {
    const YAML::Node root = loadYamlFile(songYamlPath);
    if (!root.IsMap()) {
        throwParseError("song.yaml root must be a mapping");
    }

    Song song;
    song.folderPath = songYamlPath.parent_path();
    song.id = songFolderPath.filename().string();
    song.title = requireScalar(root, "title");

    if (auto bpm = tryAsScalar<int>(root["bpm"], "bpm")) {
        if (bpm.value() > 0) {
            song.bpm = bpm.value();
        } else {
            logWarning("Field 'bpm' must be positive; ignoring");
        }
    }

    parseTimeSignature(root, song);
    discoverTracks(songFolderPath, song);
    parseSections(root, song);

    return song;
}

}  // namespace

SongParseResult parseSong(const std::filesystem::path& songFolderPath) {
    const std::filesystem::path songYamlPath = songFolderPath / "song.yaml";

    SongParseResult result;
    result.song = parseSongManifest(songYamlPath, songFolderPath);
    populateSongTakes(result.song, result.warnings);
    return result;
}

}  // namespace garageplaymate
