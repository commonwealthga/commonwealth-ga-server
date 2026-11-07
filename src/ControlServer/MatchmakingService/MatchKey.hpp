#pragma once
// Optional per-player match key. Queues that must keep
// players apart by target use it: parties with different keys never share a
// match, and an instance carries the key of the players it was spawned for.
// A "holding" player only joins a match / instance that has its key (it never
// starts one alone) until its hold time runs out; after that it has no key and
// matches like any other party.

#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace MatchKey {

// key 0 clears. hold_secs: 0 = no hold, < 0 = hold until the player leaves the queue.
void Set(const std::string& guid, int64_t key, int hold_secs);
int64_t KeyOf(const std::string& guid);          // 0 = none (or hold ran out)
bool Holding(const std::string& guid);
int SecondsToNextRelease(const std::vector<QueuedParty>& parties);  // -1 = nobody holding

// Instance <- key of the first keyed player it was spawned for.
void NoteInstance(int64_t instance_id, const std::vector<std::string>& guids);
int64_t InstanceKey(int64_t instance_id);

struct Group {
    int64_t key = 0;
    bool hold_only = false;                 // every party in it is holding
    std::vector<QueuedParty> parties;
    std::vector<RunningInstance> instances; // instances this group may join
};
// Groups in queue order (earliest party first). A hold-only group is left out
// when neither a running instance nor a pending match (pending_keys) has its key.
std::vector<Group> Groups(const std::vector<QueuedParty>& eligible,
                          const std::vector<RunningInstance>& instances,
                          const std::set<int64_t>& pending_keys);

int SettingInt(const char* key, int def);  // cs_settings integer, def when absent

}  // namespace MatchKey
