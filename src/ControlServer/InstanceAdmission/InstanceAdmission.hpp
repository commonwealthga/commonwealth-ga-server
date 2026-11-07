#pragma once

#include <cstdint>
#include <string>

struct InstanceInfo;

// InstanceAdmission -- FIFO wait list for a shared instance that is at its
// player cap (open-world zones today). A player whose travel finds the target
// full waits where they stand and is let in, in arrival order, as soon as a
// slot frees up. Free slots = max_players - active players - recent admits
// (an admitted player only gets an instance row after PLAYER_REGISTER acks).
//
// Optional ServiceHooks event "instance_admit" {map, instance_id, players,
// max_players, waiting} -> {"admit": false} holds the line even when a slot is
// free. No service / no reply / anything else -> the built-in count rule.
class InstanceAdmission {
public:
    enum class Result { Admit, Wait };

    // Called on every travel poll. Adds the player to the end of the map's
    // line on the first call; returns Admit when it is their turn and a slot
    // is free (the player is then removed from the line). fallback_cap is used
    // when the instance has not reported max_players.
    static Result Poll(const std::string& map_name, const InstanceInfo& target,
                       const std::string& session_guid, int fallback_cap,
                       int* position_out, int* line_length_out);

    // Removes the player from every line (left the line, disconnected, moved).
    // Returns true if they were waiting somewhere.
    static bool Leave(const std::string& session_guid);

    static bool IsWaiting(const std::string& session_guid);

    // Seconds a player may wait before the line gives up on them.
    static constexpr int kMaxWaitSeconds = 15 * 60;
};
