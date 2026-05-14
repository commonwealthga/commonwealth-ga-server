#pragma once

#include <string>

namespace TgPlayerActions::ChangeTeamCmd {

enum class Target {
    Toggle,
    Attackers,
    Defenders,
};

// Execute the team-switch transaction on the game thread for a player
// identified by session_guid. Looks up the pawn via GClientConnectionsData,
// resolves the new team, calls SetTaskForceNumber, then eventSuicide. Silent
// (no IPC reply) on success and on every failure path; all outcomes logged on
// the "chat-command" channel.
//
// MUST be called on the game thread. IpcClient::DrainInbound (the sole caller
// path) is invoked from Actor::Tick, so this precondition is satisfied there.
void Execute(const std::string& session_guid, Target target);

} // namespace TgPlayerActions::ChangeTeamCmd
