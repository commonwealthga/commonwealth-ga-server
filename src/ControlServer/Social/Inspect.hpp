#pragma once
#include <cstdint>
#include <string>

// GO_PLAY MAP_TYPE (GA_T 0x330). The client keeps it per map and only
// lets X (InspectAgentProfile) inspect another agent when the map type is one
// of the city/open-world types (valid-value group 59: Public, Alliance,
// Agency, Open World PvE). 0 = do not send the field.
namespace Inspect {
    uint32_t MapTypeFor(const std::string& game_mode);

    // The viewer's client asked (via the game server, IPC "inspect_request")
    // for an agent's profile. Remembers the target for this session (forgets it when the
    // viewer opens their own profile: target == viewer) and returns the character id to
    // answer with (0 = ignore).
    int64_t NoteRequest(const std::string& session_guid, int64_t viewer_char, int64_t target_char);
    // Target whose Information panel the 2-s keepalive push should restate instead of
    // the viewer's own (the client keeps a 0x017F reply only while the Stats tab is
    // open and has no character filter). 0 = none / expired.
    int64_t ActiveTarget(const std::string& session_guid);
}
