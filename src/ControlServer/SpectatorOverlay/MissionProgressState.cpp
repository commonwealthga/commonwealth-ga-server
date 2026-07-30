#include "src/ControlServer/SpectatorOverlay/MissionProgressState.hpp"

#include <ctime>

std::mutex MissionProgressState::mutex_;
std::map<int64_t, MissionProgressState::MissionSnapshot> MissionProgressState::state_;

void MissionProgressState::Update(int64_t instance_id, const MissionSnapshot& snap) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_[instance_id] = snap;
}

std::optional<MissionProgressState::MissionSnapshot> MissionProgressState::GetForInstance(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = state_.find(instance_id);
    if (it == state_.end()) return std::nullopt;

    const int64_t now = (int64_t)std::time(nullptr);
    if (now - it->second.updated_at > kStaleSeconds) {
        state_.erase(it);
        return std::nullopt;
    }
    return it->second;
}

void MissionProgressState::ClearInstance(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.erase(instance_id);
}

void MissionProgressState::Sweep() {
    std::lock_guard<std::mutex> lock(mutex_);
    const int64_t now = (int64_t)std::time(nullptr);

    for (auto it = state_.begin(); it != state_.end(); ) {
        if (now - it->second.updated_at > kStaleSeconds) {
            it = state_.erase(it);
        } else {
            ++it;
        }
    }
}
