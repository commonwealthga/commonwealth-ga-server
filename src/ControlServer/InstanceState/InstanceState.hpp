#pragma once
#include "lib/nlohmann/json.hpp"

#include <cstdint>

// InstanceState -- last known live state of each match instance, kept from
// the instance's own reports: the latest MISSION_PROGRESS_SNAPSHOT and the
// latest MATCH_STATS per character. Entries outlive the instance process
// (so the state is still there after the process is gone) and are dropped
// after an hour without updates.
namespace InstanceState {

void SetProgress(int64_t instance_id, const nlohmann::json& snapshot);
void SetPlayerStats(int64_t instance_id, int64_t character_id, const nlohmann::json& stats);

// {"instance_id", "updated_at", "progress": {...} | null, "players": [...]};
// null if nothing is known about the instance.
nlohmann::json Get(int64_t instance_id);

// Task force of a character in its last MATCH_STATS, 0 if unknown.
int TaskForceOf(int64_t instance_id, int64_t character_id);
int TaskForceOfUser(int64_t instance_id, int64_t user_id);

void Forget(int64_t instance_id);

// IPC message held for an instance that is not READY yet; handed out (once)
// by TakeQueued when its INSTANCE_READY arrives. A newer message replaces it.
void Queue(int64_t instance_id, const nlohmann::json& msg);
nlohmann::json TakeQueued(int64_t instance_id); // null if none

} // namespace InstanceState
