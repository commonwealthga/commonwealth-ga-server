#pragma once
// Builders + assertions shared by the matchmaking rule tests. Pure — only the
// matchmaking Domain + a MatchResult.
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace tu {

// Engine PROFILE_* class ids.
constexpr uint32_t A = 680;  // Assault
constexpr uint32_t M = 567;  // Medic
constexpr uint32_t R = 681;  // Recon
constexpr uint32_t B = 679;  // Robotics

// Deterministic join time from a sequence number (lower seq = longer waiting).
inline std::chrono::steady_clock::time_point At(int seq) {
    return std::chrono::steady_clock::time_point{} + std::chrono::seconds(seq);
}

// A solo party. guid defaults to "s<seq>".
inline QueuedParty Solo(uint32_t profile, int seq, const std::string& guid = "") {
    QueuedParty p;
    p.party_id = 0x8000000000000000ull | (uint64_t)(seq + 1);  // solo id space
    p.is_team = false;
    p.joined_at = At(seq);
    const std::string g = guid.empty() ? ("s" + std::to_string(seq)) : guid;
    p.leader_guid = g;
    QueuedPlayer m; m.session_guid = g; m.profile_id = profile; m.user_id = seq + 1; m.joined_at = At(seq);
    p.members.push_back(m);
    return p;
}

// A team party. party_id is the team id; members guid = "t<id>_<i>".
inline QueuedParty Team(uint64_t id, const std::vector<uint32_t>& profiles, int seq) {
    QueuedParty p;
    p.party_id = id;
    p.is_team = true;
    p.joined_at = At(seq);
    p.leader_guid = "t" + std::to_string(id) + "_0";
    for (size_t i = 0; i < profiles.size(); ++i) {
        QueuedPlayer m;
        m.session_guid = "t" + std::to_string(id) + "_" + std::to_string(i);
        m.profile_id = profiles[i];
        m.user_id = (int64_t)(id * 100 + i);
        m.joined_at = At(seq);
        p.members.push_back(m);
    }
    return p;
}

// Count members assigned to each task force in a result.
struct TfCounts { int tf1 = 0; int tf2 = 0; };
inline TfCounts CountTf(const MatchResult& r) {
    TfCounts c;
    for (const auto& [guid, tf] : r.task_force_assignments) (tf == 2 ? c.tf2 : c.tf1)++;
    return c;
}

// Class headcount on a side of a result.
inline int ClassOnTf(const MatchResult& r, uint32_t profile, int tf) {
    int n = 0;
    for (const auto& [guid, t] : r.task_force_assignments) {
        if (t != tf) continue;
        auto it = r.profile_ids.find(guid);
        if (it != r.profile_ids.end() && it->second == profile) n++;
    }
    return n;
}

// All members of a party landed on the same task force?
inline bool PartyTogether(const MatchResult& r, const QueuedParty& party) {
    int tf = -1;
    for (const auto& m : party.members) {
        auto it = r.task_force_assignments.find(m.session_guid);
        if (it == r.task_force_assignments.end()) return false;  // not placed
        if (tf == -1) tf = it->second;
        else if (tf != it->second) return false;
    }
    return true;
}

// Was a guid placed at all?
inline bool Placed(const MatchResult& r, const std::string& guid) {
    return r.task_force_assignments.count(guid) > 0;
}

// Build a QueueConfig with the policy knobs the tests care about.
inline QueueConfig Cfg(const std::string& rule_class,
                       TaskforcePolicy tf, TeamPolicy team, TeamSidePolicy side,
                       uint32_t cap = 0) {
    QueueConfig c;
    c.queue_id = 1;
    c.rule_class = rule_class;
    c.taskforce_policy = tf;
    c.team_policy = team;
    c.team_side_policy = side;
    c.max_players_per_instance = cap;
    c.max_players_per_side = cap > 0 ? cap : 8;
    return c;
}

}  // namespace tu
