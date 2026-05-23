#include "src/ControlServer/MatchmakingService/RuntimeStats.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

QueueRuntimeStats RuntimeStats::ForQueue(uint32_t queue_id) {
    QueueRuntimeStats out;

    // In-queue half (waiting for match-pop).
    const auto queued = MatchmakingService::GetQueuedProfileCounts(queue_id);
    out.assault  = queued.assault;
    out.medic    = queued.medic;
    out.recon    = queued.recon;
    out.robotics = queued.robotics;
    const uint32_t queued_total = queued.assault + queued.medic + queued.recon + queued.robotics;

    // In-mission half (active instances of this queue).
    const auto in_mission_total   = InstanceRegistry::GetActivePlayerCountForQueue(queue_id);
    const auto in_mission_classes = InstanceRegistry::GetActiveProfileCountsForQueue(queue_id);
    out.assault  += in_mission_classes.assault;
    out.medic    += in_mission_classes.medic;
    out.recon    += in_mission_classes.recon;
    out.robotics += in_mission_classes.robotics;

    out.player_count   = queued_total + (uint32_t)in_mission_total;
    out.instance_count = (uint32_t)InstanceRegistry::GetActiveInstanceCountForQueue(queue_id);
    return out;
}
