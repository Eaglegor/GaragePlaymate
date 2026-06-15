#pragma once

#include "SongTypes.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace garageplaymate {

struct VolumeDropFailure {
    float failureLevel = 0.15f;
};

struct FailureEvent {
    std::string trackId;
    int64_t onsetMs = 0;
    int64_t restoreMs = 0;
    std::variant<VolumeDropFailure> eventData;
};

struct PlaybackTransport {
    int64_t currentTimestamp = 0;
};

struct PlaybackSession {
    int64_t id = 0;
    std::string songId;
    std::map<std::string, std::shared_ptr<TakeFile>> takeMap;
    std::vector<FailureEvent> failureEvents;

    int64_t maxTakeDurationMs() const {
        int64_t maxDuration = 0;
        for (const auto& [_, take] : takeMap) {
            if (take != nullptr) {
                maxDuration = std::max(maxDuration, take->durationMs);
            }
        }
        return maxDuration;
    }
};

}  // namespace garageplaymate
