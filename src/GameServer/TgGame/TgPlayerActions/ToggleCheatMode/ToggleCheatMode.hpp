#pragma once

#include <string>

namespace TgPlayerActions::ToggleCheatCmd {

// Toggle one of the cheat mode for the player owning session_guid.
//
void Execute(const std::string& session_guid, int cheat_mode);

} // namespace TgPlayerActions::ToggleCheatCmd
