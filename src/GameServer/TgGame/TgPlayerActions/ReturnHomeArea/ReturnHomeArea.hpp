#pragma once

#include <string>

namespace TgPlayerActions::ReturnHomeAreaCmd {

// Move the player back to the current map's player-start area.
// Currently only valid for Dome3_VR_Arena_P; other home maps need their own
// return-area policy once they exist.
void Execute(const std::string& session_guid);

// Player -stuck: same move on any map, live pawn only; refused (with an
// on-screen line) when pvp is true. Cooldown is enforced by the control server.
void ExecuteStuck(const std::string& session_guid, bool pvp);

} // namespace TgPlayerActions::ReturnHomeAreaCmd
