#pragma once
// Patch 1.5 "Assignment-specific missions should have a simplified
// end-of-mission screen rather than the standard screen populated with no data."
// The client picks that screen itself: RoundEnded.BeginState -> TgHUD.CheckShouldSkipEndMission(), which
// is true when the GO_PLAY MAP_TYPE (valid value group 59) is 613 Solo, 748 Training or 321 Mission
// (exe 0x113a09e0). Assignment instances are the hidden zone-door queues (ga_queues.name "zone_*"),
// so their GO_PLAY carries MAP_TYPE 321 "Mission". No other client branch tests 321.
#include <cstdint>
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

namespace AssignmentMapType {
constexpr uint32_t kMapTypeMission = 321;
inline uint32_t ForQueue(uint32_t queue_id) {
    if (queue_id == 0) return 0;
    auto q = MatchmakingService::GetQueueConfig(queue_id);
    return (q && q->name.rfind("zone_", 0) == 0) ? kMapTypeMission : 0;
}
}
