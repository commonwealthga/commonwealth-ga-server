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
// resolves the new team, hands off any carried/deployed beacon, calls
// SetTaskForceNumber, then teleports the (living) pawn to the new team's
// player start — no death, no respawn timer. Silent (no IPC reply) on success
// and on every failure path; all outcomes logged on the "chat-command" channel.
//
// is_autobalance=true marks an auto-rebalance move (not a manual -changeteam):
// the player gets an on-screen "you have been autobalanced" alert.
//
// MUST be called on the game thread. IpcClient::DrainInbound (the sole caller
// path) is invoked from Actor::Tick, so this precondition is satisfied there.
void Execute(const std::string& session_guid, Target target, bool is_autobalance);

} // namespace TgPlayerActions::ChangeTeamCmd
