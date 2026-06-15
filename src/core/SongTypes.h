#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace garageplaymate {

struct TakeFile {
    std::string filename;
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

}  // namespace garageplaymate
