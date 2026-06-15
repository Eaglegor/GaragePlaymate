#include "storage/SongFolderScanner.h"

#include "storage/SongYamlParser.h"
#include "storage/WavMetadataReader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace garageplaymate {
namespace {

bool hasWavExtension(const std::filesystem::path& filePath) {
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension == ".wav";
}

void addWarning(ScanResult& result, const std::filesystem::path& path, std::string message) {
    result.warnings.push_back(ScanWarning{path.string(), std::move(message)});
}

void enumerateTrackTakes(TrackDef& track, ScanResult& result) {
    std::vector<TakeFile> takes;

    std::error_code errorCode;
    if (!std::filesystem::is_directory(track.folderPath, errorCode)) {
        addWarning(result, track.folderPath, "Track folder is missing or unreadable");
        track.takes = std::move(takes);
        return;
    }

    std::error_code iterationError;
    std::filesystem::directory_iterator iterator(track.folderPath, iterationError);
    if (iterationError) {
        addWarning(result, track.folderPath, "Failed to read track folder: " + iterationError.message());
        track.takes = std::move(takes);
        return;
    }

    for (; iterator != std::filesystem::directory_iterator(); iterator.increment(iterationError)) {
        if (iterationError) {
            addWarning(result, track.folderPath, "Failed to read track folder: " + iterationError.message());
            break;
        }

        const std::filesystem::directory_entry& entry = *iterator;
        if (!entry.is_regular_file(errorCode)) {
            if (errorCode) {
                addWarning(result, entry.path(), "Failed to inspect file: " + errorCode.message());
                errorCode.clear();
            }
            continue;
        }

        if (!hasWavExtension(entry.path())) {
            addWarning(result, entry.path(), "Ignoring non-WAV file");
            continue;
        }

        TakeFile take;
        take.filename = entry.path().filename().string();

        std::string metadataError;
        if (const std::optional<WavMetadata> metadata = readWavMetadata(entry.path(), &metadataError)) {
            take.durationMs = metadata->durationMs;
        } else {
            addWarning(result, entry.path(), metadataError.empty() ? "Failed to read WAV metadata" : metadataError);
            take.durationMs = 0;
        }

        takes.push_back(std::move(take));
    }

    std::sort(takes.begin(), takes.end(),
              [](const TakeFile& left, const TakeFile& right) { return left.filename < right.filename; });
    track.takes = std::move(takes);
}

void populateSongTakes(Song& song, ScanResult& result) {
    for (TrackDef& track : song.tracks) {
        enumerateTrackTakes(track, result);
    }
}

}  // namespace

ScanResult scanLibrary(const std::filesystem::path& dataRoot) {
    ScanResult result;
    const std::filesystem::path songsRoot = dataRoot / "songs";

    std::error_code errorCode;
    if (!std::filesystem::exists(songsRoot, errorCode)) {
        addWarning(result, songsRoot, "Songs directory does not exist");
        return result;
    }

    if (!std::filesystem::is_directory(songsRoot, errorCode)) {
        addWarning(result, songsRoot, "Songs path is not a directory");
        return result;
    }

    std::error_code iterationError;
    std::filesystem::directory_iterator iterator(songsRoot, iterationError);
    if (iterationError) {
        addWarning(result, songsRoot, "Failed to read songs directory: " + iterationError.message());
        return result;
    }

    for (; iterator != std::filesystem::directory_iterator(); iterator.increment(iterationError)) {
        if (iterationError) {
            addWarning(result, songsRoot, "Failed to read songs directory: " + iterationError.message());
            break;
        }

        const std::filesystem::directory_entry& entry = *iterator;
        if (!entry.is_directory(errorCode)) {
            if (errorCode) {
                addWarning(result, entry.path(), "Failed to inspect song folder: " + errorCode.message());
                errorCode.clear();
            }
            continue;
        }

        const std::filesystem::path songFolderPath = entry.path();
        const std::filesystem::path songYamlPath = songFolderPath / "song.yaml";
        if (!std::filesystem::is_regular_file(songYamlPath, errorCode)) {
            if (errorCode) {
                addWarning(result, songYamlPath, "Failed to inspect song manifest: " + errorCode.message());
                errorCode.clear();
            }
            continue;
        }

        try {
            Song song = parseSongYaml(songYamlPath, songFolderPath);
            populateSongTakes(song, result);
            result.songs.push_back(std::move(song));
        } catch (const ParseError& parseError) {
            addWarning(result, songYamlPath, parseError.what());
        } catch (const std::exception& exception) {
            addWarning(result, songYamlPath, exception.what());
        }
    }

    std::sort(result.songs.begin(), result.songs.end(),
              [](const Song& left, const Song& right) { return left.id < right.id; });
    return result;
}

}  // namespace garageplaymate
