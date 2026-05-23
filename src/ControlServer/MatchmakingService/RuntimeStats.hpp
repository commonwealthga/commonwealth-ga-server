#pragma once

#include <cstdint>

// RuntimeStats — the three live counters TicketInfoEncoder needs per queue:
//   • PLAYER_COUNT          = (queued in this queue) + (in any active mission of this queue)
//   • INSTANCE_COUNT        = active (non-STOPPED) mission instances of this queue
//   • DATA_SET_PROFILE_COUNTS = per-class breakdown, queued+in-mission summed
//
// "Active" instance state = anything other than STOPPED (STARTING, DRAFTING,
// READY, DRAINING all count). Profiles outside the four dashboard classes
// (ASSAULT / MEDIC / RECON / ROBOTICS) are silently dropped.
struct QueueRuntimeStats {
    uint32_t player_count = 0;
    uint32_t instance_count = 0;
    uint32_t assault = 0;
    uint32_t medic = 0;
    uint32_t recon = 0;
    uint32_t robotics = 0;
};

class RuntimeStats {
public:
    static QueueRuntimeStats ForQueue(uint32_t queue_id);
};
