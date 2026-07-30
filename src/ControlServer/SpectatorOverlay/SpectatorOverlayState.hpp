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
        float power = 0.0f;
        float power_max = 0.0f;
        // Whitelisted (OverlayIdCatalog::IsEffectId), deduped -- the only ids
        // the HTML renders as a tile icon.
        std::vector<int> effect_ids;
        // Whitelisted (OverlayIdCatalog::IsSkillId), deduped -- pushed through
        // the same feed but intentionally not rendered on the player card.
        std::vector<int> skill_ids;
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

    // Prune stale entries across EVERY instance, regardless of whether
    // anyone's currently reading or writing. GetForInstance() only prunes
    // the one instance being polled -- an instance a spectator visited and
    // then left, with no one ever polling it again, would otherwise hold
    // onto those entries for the rest of that instance's (possibly long)
    // remaining lifetime, since ClearInstance() only fires on instance stop.
    // Call this periodically (see main.cpp's existing 60s maintenance timer).
    // Also erases any instance whose map is empty after pruning, so this
    // doesn't leave empty husk entries behind for long-dead instance ids.
    static void Sweep();

    // Instance ids that currently have at least one non-stale snapshot --
    // i.e. instances an active spectator is actually watching right now, as
    // opposed to every running match (InstanceRegistry has that list, but it
    // includes matches nobody is broadcasting). Backs the overlay HTML's
    // instance picker (see OverlayHttpServer's GET /overlay/instances) so a
    // hosted page the operator can't edit still only ever offers instances
    // worth selecting.
    static std::vector<int64_t> ListActiveInstances();

private:
    static constexpr int64_t kStaleSeconds = 15;

    static std::mutex mutex_;
    // instance_id -> session_guid -> snapshot
    static std::map<int64_t, std::map<std::string, PawnSnapshot>> state_;
};
