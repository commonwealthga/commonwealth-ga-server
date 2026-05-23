#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/MatchmakingService/RuntimeStats.hpp"
#include <cstdint>
#include <vector>

// TicketInfoEncoder — writes one inner queue record into a GET_TICKET_INFO
// response payload (the outer TLV envelope + DATA_SET header is the caller's
// responsibility). See FUN_10927190 in the client binary for the matching
// reader; field tag set is fixed by that decompile.
//
// Fields with default/sentinel values (status_msg_id=0, access_flags=0,
// remaining_seconds=nullopt) are OMITTED from the wire so the field count
// matches what's actually written.
class TicketInfoEncoder {
public:
    static void EncodeRecord(
        std::vector<uint8_t>& out,
        const QueueConfig& cfg,
        const QueueRuntimeStats& stats);
};
