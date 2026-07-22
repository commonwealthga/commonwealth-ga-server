#include "src/ControlServer/SpectatorOverlay/SpectatorOverlayState.hpp"

#include <ctime>

std::mutex SpectatorOverlayState::mutex_;
std::map<int64_t, std::map<std::string, SpectatorOverlayState::PawnSnapshot>> SpectatorOverlayState::state_;

void SpectatorOverlayState::Update(int64_t instance_id, const PawnSnapshot& snap) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_[instance_id][snap.session_guid] = snap;
}

std::vector<SpectatorOverlayState::PawnSnapshot> SpectatorOverlayState::GetForInstance(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PawnSnapshot> out;

    auto it = state_.find(instance_id);
    if (it == state_.end()) return out;

    const int64_t now = (int64_t)std::time(nullptr);
    auto& byGuid = it->second;
    for (auto guidIt = byGuid.begin(); guidIt != byGuid.end(); ) {
        if (now - guidIt->second.updated_at > kStaleSeconds) {
            guidIt = byGuid.erase(guidIt);
        } else {
            out.push_back(guidIt->second);
            ++guidIt;
        }
    }
    return out;
}

void SpectatorOverlayState::ClearInstance(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.erase(instance_id);
}
