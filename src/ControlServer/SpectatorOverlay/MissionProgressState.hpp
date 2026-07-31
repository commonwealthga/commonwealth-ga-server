#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// In-memory-only live state for mission-progress info on the spectator
// broadcast overlay (ticket counts, raid wave/boss health, koth/payload/
// breach objective capture). Sibling to SpectatorOverlayState but keyed
// one-snapshot-per-instance rather than per-player -- there's exactly one
// ATgGame per match. Fed by MISSION_PROGRESS_SNAPSHOT, read by the overlay
// HTTP endpoint. Deliberately not persisted, same as SpectatorOverlayState.
class MissionProgressState {
public:
    struct Objective {
        int id = 0;
        std::string name;  // "" if no DB row/translation -- see ObjectiveNames.hpp
        int priority = 0;
        bool locked = false;
        bool active = false;
        int owner_taskforce = 0;
        float capture_pct = 0.0f;
    };

    struct MissionSnapshot {
        std::string mode;  // "ticket" | "raid" | "koth" | "payload" | "breach" | "pve" | "superagent"

        // Universal, every mode -- the engine's own live countdown. See
        // IpcProtocol.hpp's MSG_MISSION_PROGRESS_SNAPSHOT doc for the
        // mission_timer_state (TGMTS_*) value meanings.
        float mission_remaining_seconds = 0.0f;
        int mission_timer_state = 0;

        // mode == "ticket" | "koth"
        int attacker_points = 0;
        int defender_points = 0;
        int points_to_win = 0;

        // mode == "raid"
        int round_current = 0;
        int round_max = 0;

        // mode == "raid" | "pve" | "superagent"
        int boss_health = 0;
        int boss_health_max = 0;
        std::string boss_name;

        // mode == "raid" only -- a defender-owned NPC/reactor being protected,
        // separate from the boss (e.g. "Bancroft" on Raid_DomeCityDefense_P).
        int friendly_health = 0;
        int friendly_health_max = 0;
        std::string friendly_name;

        // mode == "ticket" | "koth" | "payload" | "breach" | "superagent"
        std::vector<Objective> objectives;

        int64_t updated_at = 0;  // unix seconds -- used to prune a stale/ended match
    };

    static void Update(int64_t instance_id, const MissionSnapshot& snap);

    // Returns the current snapshot for instance_id, or nullopt if there is
    // none or it's older than kStaleSeconds (mirrors SpectatorOverlayState's
    // self-healing read path -- a match can end without any explicit
    // removal message reaching this store).
    static std::optional<MissionSnapshot> GetForInstance(int64_t instance_id);

    // Drop the snapshot for an instance (called on instance stop/empty).
    static void ClearInstance(int64_t instance_id);

    // Prune stale entries across every instance. Call periodically (see
    // main.cpp's existing 60s maintenance timer, alongside
    // SpectatorOverlayState::Sweep()).
    static void Sweep();

private:
    static constexpr int64_t kStaleSeconds = 15;

    static std::mutex mutex_;
    static std::map<int64_t, MissionSnapshot> state_;
};
