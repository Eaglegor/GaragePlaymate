#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace garageplaymate {

struct FailureEvent {
    std::string trackId;
    int64_t onsetMs = 0;
    int64_t restoreMs = 0;
    float failureLevel = 0.15f;
};

struct PlaybackSession {
    int64_t id = 0;
    std::string songId;
    int64_t timestampUnixMs = 0;
    std::map<std::string, std::string> takeMap;
    std::vector<FailureEvent> failureEvents;
    bool isReplay = false;
};

}  // namespace garageplaymate
