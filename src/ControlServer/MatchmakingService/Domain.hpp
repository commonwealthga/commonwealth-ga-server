#pragma once
//
// Matchmaking domain types — PURE. No asio, no DB, no Logger, no singletons.
// Everything here is plain data + free functions so the rules and placement
// algorithms can be unit-tested in isolation (see tests/matchmaking/).
//
// MatchmakingService.hpp includes this and re-exports it, so existing callers
// that `#include MatchmakingService.hpp` keep compiling unchanged.
//
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <algorithm>

// ---------------------------------------------------------------------------
// Players & parties
// ---------------------------------------------------------------------------

// One queued player. `profile_id` is the engine PROFILE_* class id.
struct QueuedPlayer {
    std::string session_guid;
    uint32_t    profile_id = 0;   // ASSAULT/MEDIC/RECON/ROBOTICS engine ids
    int64_t     user_id    = 0;   // ga_users.id — drives requires_pvp_verification
    std::chrono::steady_clock::time_point joined_at{};
};

// A party is the UNIT OF QUEUEING. A solo player is a party of one
// (is_team=false); a team queued by its leader is a party of N (is_team=true).
// party_id is unique across all live parties — for teams it's the TeamService
// team id; for solos a synthetic id derived from the session.
struct QueuedParty {
    uint64_t    party_id = 0;
    bool        is_team  = false;
    std::string leader_guid;             // == members[0].session_guid for solos
    std::vector<QueuedPlayer> members;   // never empty
    std::chrono::steady_clock::time_point joined_at{};  // earliest member join

    size_t size() const { return members.size(); }
};

// ---------------------------------------------------------------------------
// Policy enums (per-queue config; see ga_queues)
// ---------------------------------------------------------------------------

// How a popped match splits players across task forces (TF1/TF2). The
// algorithm; team_side_policy modulates how parties are kept together within it.
enum class TaskforcePolicy : uint8_t {
    Pinned1,      // everyone -> TF 1 (SpecOps / Solo coop / DDR)
    Pinned2,      // everyone -> TF 2 (Defense)
    Balanced,     // place on the smaller of {team1, team2} by headcount
    BalancedPvp,  // class-aware role-weighted variant; see SidePlacement
};

// What happens when a TEAM party pops from this queue.
enum class TeamPolicy : uint8_t {
    Block,        // teams may not queue here (rejected before AddParty)
    OwnMatch,     // a team pop ALWAYS spawns a fresh PARTY_LOCKED match;
                  //   solo parties never see it (standard PvE)
    Mixed,        // teams enter the shared pool like everyone else (DA/merc/DDR)
    VersusSides,  // parties matched against each other as whole sides (1v1)
};

// Where teammates land relative to each other.
enum class TeamSidePolicy : uint8_t {
    Ignore,       // members placed individually (legacy behaviour)
    Preferred,    // try to keep a party on one TF; balance/shape may split it
    Required,     // a party always lands entirely on a single TF
};

// Who may be routed into an already-spawned match.
enum class AccessMode : uint8_t {
    Open,         // any party from this queue may be routed in (drop-in)
    PartyLocked,  // only owner_party_ids + explicit invite-accepts may enter
    Sealed,       // nobody enters after match start — not even invites (DA)
};

// Pop-delay re-arm shape (governs MaybeResetDelayedPop while a timer runs).
enum class PopDelayPolicy : uint8_t {
    HalveOnJoin,   // next_duration halves on each join, floor 0.5s
    Fixed,         // timer set once; ignores joins (cancels only on leave below min)
    ResetOnJoin,   // every join re-arms timer to cfg.pop_delay_seconds
};

// ---------------------------------------------------------------------------
// Map pool
// ---------------------------------------------------------------------------

// One weighted (map, game_mode) pair. weight is 1-based; uniform when all 1.
struct MapModeEntry {
    std::string map_name;
    std::string game_mode;
    int weight = 1;
    std::optional<int> min_players;  // NULL in DB → unbounded below
    std::optional<int> max_players;  // NULL in DB → unbounded above
};

// Legacy (map, game_mode) pair without weight. Returned by pool pickers.
struct MapModeSpec {
    std::string map_name;
    std::string game_mode;
};

// ---------------------------------------------------------------------------
// Team seed — per-team aggregate used by placement
// ---------------------------------------------------------------------------

// A team's current makeup (live roster + reserved-but-not-yet-arrived players)
// used to seed placement when joining an in-progress instance, or to score
// candidate placements. Empty = fresh side.
struct TeamSeed {
    int   size       = 0;
    float heal_score = 0.0f;
    std::unordered_map<uint32_t, int> class_counts;  // profile_id -> headcount
};

// ---------------------------------------------------------------------------
// RunningInstance — a candidate instance a rule may route a party into.
// The InstanceProvider (impure, in main.cpp) gathers ALL live state here so
// rules stay pure functions of (parties, instances).
// ---------------------------------------------------------------------------

struct RunningInstance {
    int64_t     instance_id = 0;
    std::string map_name;
    std::string game_mode;
    int         player_count = 0;   // live + reserved bodies
    int         max_players  = 0;   // 0 = unlimited
    uint32_t    queue_id     = 0;

    TeamSeed    team1;              // combined live + reserved on TF1
    TeamSeed    team2;              //                          TF2

    AccessMode  access_mode = AccessMode::Open;
    std::vector<uint64_t> owner_party_ids;  // populated when PartyLocked

    int  seats_free() const {
        return max_players > 0 ? std::max(0, max_players - player_count) : -1;  // -1 = unlimited
    }
    // A party of `party_id` may enter iff Open, or PartyLocked-and-owner.
    bool admits_party(uint64_t party_id) const {
        if (access_mode == AccessMode::Open) return true;
        if (access_mode == AccessMode::Sealed) return false;
        return std::find(owner_party_ids.begin(), owner_party_ids.end(), party_id)
               != owner_party_ids.end();
    }
};

// ---------------------------------------------------------------------------
// MatchResult — a rule's decision for one pop.
// ---------------------------------------------------------------------------

struct MatchResult {
    std::string map_name;     // empty + no existing_instance_id => fill from pool
    std::string game_mode;
    std::vector<std::string> session_guids;
    std::optional<int64_t> existing_instance_id;   // set => join, don't spawn
    std::unordered_map<std::string, int> task_force_assignments;  // guid -> TF (1/2)
    std::unordered_map<std::string, uint32_t> profile_ids;        // guid -> profile_id

    // Override QueueConfig cap for the resulting PendingMatch. Used to seal a
    // match at its popped size (DA) regardless of the queue's nominal cap.
    std::optional<uint32_t> cap_override;

    // Access semantics for the spawned/targeted instance.
    AccessMode access_mode = AccessMode::Open;
    std::vector<uint64_t> owner_party_ids;  // PARTY_LOCKED owners

    // Parties fully consumed by this result — the orchestrator removes these
    // from the queue. Derived from session_guids' owning parties, but tracked
    // explicitly so partial-party pops never happen (parties are atomic).
    std::vector<uint64_t> consumed_party_ids;
};

// ---------------------------------------------------------------------------
// QueueConfig — one ga_queues row + its joined map_pool.
// ---------------------------------------------------------------------------

struct QueueConfig {
    uint32_t queue_id = 0;
    uint32_t map_pool_id = 0;                         // 0 = no pool assigned
    std::string name;                                 // log/debug only
    std::string rule_class;                           // empty => Coop (DataDriven)
    TaskforcePolicy taskforce_policy = TaskforcePolicy::Pinned1;
    bool continue_in_queue = false;
    bool enabled = true;

    // Team-aware config (added 2026-06-11).
    TeamPolicy     team_policy      = TeamPolicy::Mixed;
    TeamSidePolicy team_side_policy = TeamSidePolicy::Ignore;
    uint32_t       max_team_size    = 0;  // 0 => queue's per-side cap

    // GET_TICKET_INFO wire fields — fed to TicketInfoEncoder verbatim.
    uint32_t queue_type_value_id = 0;
    uint32_t status_msg_id = 0;
    uint32_t name_msg_id = 0;
    uint32_t desc_msg_id = 0;
    uint32_t icon_id = 0;
    uint32_t max_players_per_side = 1;
    uint32_t min_players_per_team = 1;
    uint32_t max_players_per_team = 1;
    uint32_t level_min = 1;
    uint32_t level_max = 200;
    uint32_t tab = 0;
    float map_x = 0.0f;
    float map_y = 0.0f;
    bool map_active_flag = true;
    uint32_t map_icon_texture_res_id = 0;
    uint32_t video_res_id = 0;
    uint32_t location_value_id = 0;
    bool double_agent_flag = false;
    uint32_t sys_site_id = 0;
    uint32_t sort_order = 0;
    bool bonus_queue_flag = false;
    uint32_t difficulty_value_id = 0;
    std::optional<uint32_t> marshal_difficulty_value_id;
    uint64_t access_flags = 0;
    bool active_flag = true;
    bool locked_flag = false;
    std::optional<uint32_t> remaining_seconds;

    // Queue-pop controls.
    uint32_t min_players_to_pop       = 1;
    uint32_t max_players_per_instance = 0;   // 0 = unlimited
    float    pop_delay_seconds        = 0.0f;
    PopDelayPolicy pop_delay_policy   = PopDelayPolicy::HalveOnJoin;
    bool instant_pop_when_full        = true;
    bool requires_pvp_verification    = false;

    std::vector<MapModeEntry> map_pool;
};

// ---------------------------------------------------------------------------
// Pure free helpers (no I/O). Shared by DB loader, rules, and tests.
// ---------------------------------------------------------------------------

namespace mm {

inline TaskforcePolicy ParseTaskforcePolicy(const std::string& s, bool* ok = nullptr) {
    if (ok) *ok = true;
    if (s == "pinned_1")     return TaskforcePolicy::Pinned1;
    if (s == "pinned_2")     return TaskforcePolicy::Pinned2;
    if (s == "balanced")     return TaskforcePolicy::Balanced;
    if (s == "balanced_pvp") return TaskforcePolicy::BalancedPvp;
    if (ok) *ok = false;
    return TaskforcePolicy::Pinned1;
}

inline TeamPolicy ParseTeamPolicy(const std::string& s, bool* ok = nullptr) {
    if (ok) *ok = true;
    if (s == "block")        return TeamPolicy::Block;
    if (s == "own_match")    return TeamPolicy::OwnMatch;
    if (s == "mixed")        return TeamPolicy::Mixed;
    if (s == "versus_sides") return TeamPolicy::VersusSides;
    if (ok) *ok = false;
    return TeamPolicy::Mixed;
}

inline TeamSidePolicy ParseTeamSidePolicy(const std::string& s, bool* ok = nullptr) {
    if (ok) *ok = true;
    if (s == "ignore")    return TeamSidePolicy::Ignore;
    if (s == "preferred") return TeamSidePolicy::Preferred;
    if (s == "required")  return TeamSidePolicy::Required;
    if (ok) *ok = false;
    return TeamSidePolicy::Ignore;
}

inline PopDelayPolicy ParsePopDelayPolicy(const std::string& s, bool* ok = nullptr) {
    if (ok) *ok = true;
    if (s == "halve_on_join") return PopDelayPolicy::HalveOnJoin;
    if (s == "fixed")         return PopDelayPolicy::Fixed;
    if (s == "reset_on_join") return PopDelayPolicy::ResetOnJoin;
    if (ok) *ok = false;
    return PopDelayPolicy::HalveOnJoin;
}

inline const char* AccessModeToString(AccessMode m) {
    switch (m) {
        case AccessMode::Open:        return "OPEN";
        case AccessMode::PartyLocked: return "PARTY_LOCKED";
        case AccessMode::Sealed:      return "SEALED";
    }
    return "OPEN";
}

inline AccessMode ParseAccessMode(const std::string& s) {
    if (s == "PARTY_LOCKED") return AccessMode::PartyLocked;
    if (s == "SEALED")       return AccessMode::Sealed;
    return AccessMode::Open;
}

// The hard ceiling on a single instance's roster. Pure function of config
// (+ an optional instance-reported max). Mirrors the legacy
// MatchmakingService::GetQueueInstanceCap exactly.
inline uint32_t QueueInstanceCap(const QueueConfig& cfg, uint32_t instance_max_players = 0) {
    if (cfg.max_players_per_instance > 0) return cfg.max_players_per_instance;
    if (cfg.taskforce_policy == TaskforcePolicy::Balanced
            || cfg.taskforce_policy == TaskforcePolicy::BalancedPvp) {
        if (cfg.max_players_per_side > 0) return cfg.max_players_per_side * 2;
        if (cfg.max_players_per_team > 0) return cfg.max_players_per_team * 2;
    }
    if (cfg.max_players_per_side > 0) return cfg.max_players_per_side;
    if (cfg.max_players_per_team > 0) return cfg.max_players_per_team;
    return instance_max_players;
}

// The max party size a queue accepts. 0 (default) => the per-instance cap.
inline uint32_t MaxTeamSize(const QueueConfig& cfg) {
    if (cfg.max_team_size > 0) return cfg.max_team_size;
    return QueueInstanceCap(cfg);
}

// Flatten a party list into its members in order (parties kept contiguous).
inline std::vector<QueuedPlayer> FlattenParties(const std::vector<QueuedParty>& parties) {
    std::vector<QueuedPlayer> out;
    for (const auto& party : parties)
        for (const auto& m : party.members) out.push_back(m);
    return out;
}

}  // namespace mm
