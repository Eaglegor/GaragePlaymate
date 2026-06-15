#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace garageplaymate {

struct TakeFile {
    std::string filename;
    std::filesystem::path absolutePath;
    int64_t durationMs = 0;
    bool disabled = false;
};

struct TrackDef {
    std::string id;
    std::string name;
    std::filesystem::path folderPath;
    float defaultVolume = 1.0f;
    std::vector<TakeFile> takes;
};

struct Section {
    std::string id;
    std::string name;
    int64_t startMs = 0;
};

struct Song {
    std::string id;
    std::string title;
    std::optional<int> bpm;
    std::optional<std::pair<int, int>> timeSignature;
    std::filesystem::path folderPath;
    std::vector<TrackDef> tracks;
    std::vector<Section> sections;
};

struct SongSummary {
    std::string id;
    std::string title;
    std::optional<int> bpm;
    int trackCount = 0;
    int totalTakeCount = 0;
};

inline int64_t songMaxTakeDurationMs(const Song& song) {
    int64_t maxDuration = 0;
    for (const auto& track : song.tracks) {
        for (const auto& take : track.takes) {
            maxDuration = std::max(maxDuration, take.durationMs);
        }
    }
    return maxDuration;
}

inline const Section* findSectionAtMs(const Song& song, int64_t positionMs) {
    const Section* current = nullptr;
    for (const auto& section : song.sections) {
        if (section.startMs > positionMs) {
            break;
        }
        current = &section;
    }
    return current;
}

}  // namespace garageplaymate
