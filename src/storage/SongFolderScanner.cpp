#include "storage/SongFolderScanner.h"

#include "storage/SongParser.h"

#include <algorithm>
#include <filesystem>

namespace garageplaymate {
namespace {

void addWarning(ScanResult& result, const std::filesystem::path& path, std::string message) {
    result.warnings.push_back(ScanWarning{path.string(), std::move(message)});
}

void appendParseWarnings(ScanResult& result, const std::vector<ParseWarning>& parseWarnings) {
    result.warnings.reserve(result.warnings.size() + parseWarnings.size());
    for (const ParseWarning& warning : parseWarnings) {
        result.warnings.push_back(ScanWarning{warning.path, warning.message});
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
            const SongParseResult parseResult = parseSong(songFolderPath);
            appendParseWarnings(result, parseResult.warnings);
            result.songs.push_back(parseResult.song);
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
