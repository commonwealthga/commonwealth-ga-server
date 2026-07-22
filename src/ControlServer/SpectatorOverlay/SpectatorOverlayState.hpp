#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// In-memory-only live state for the spectator broadcast overlay. Fed by
// MSG_PAWN_HEALTH_SNAPSHOT (see IpcProtocol.hpp), read by the overlay HTTP
// endpoint. Deliberately not persisted — this is a transient "what does the
// match look like right now" view, not match history.
class SpectatorOverlayState {
public:
    struct PawnSnapshot {
        std::string session_guid;
        int task_force = 0;   // 0 = unknown/none, 1 = attackers, 2 = defenders
        int health = 0;
        int health_max = 0;
        std::vector<int> effect_ids;
        int64_t updated_at = 0;  // unix seconds — used to prune disconnected players
    };

    static void Update(int64_t instance_id, const PawnSnapshot& snap);

    // Returns every snapshot currently held for instance_id, first pruning
    // entries older than kStaleSeconds (a player can disconnect without any
    // explicit removal message reaching this store — mid-air/crash/timeout —
    // so the read path self-heals rather than relying on a delete call site).
    static std::vector<PawnSnapshot> GetForInstance(int64_t instance_id);

    // Drop all snapshots for an instance (called on instance stop/empty so a
    // finished match doesn't linger in memory forever).
    static void ClearInstance(int64_t instance_id);

private:
    static constexpr int64_t kStaleSeconds = 15;

    static std::mutex mutex_;
    // instance_id -> session_guid -> snapshot
    static std::map<int64_t, std::map<std::string, PawnSnapshot>> state_;
};
