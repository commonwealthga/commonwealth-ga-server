#pragma once

#include <string>

namespace TgPlayerActions::SpawnBotCmd {

enum class Team {
    Attackers,
    Defenders,
};

// Spawn a bot in front of the requesting player and assign it to a task force.
// Resolves the player's pawn via GClientConnectionsData using session_guid,
// places the bot a fixed distance ahead along the player's yaw, then runs the
// SpawnBotById path with the chosen team. Silent (no IPC reply) on success and
// on every failure path; all outcomes logged on the "chat-command" channel.
//
// MUST be called on the game thread (IpcClient::DrainInbound is invoked from
// Actor::Tick, which satisfies this precondition).
void Execute(const std::string& session_guid, int bot_id, Team team);

} // namespace TgPlayerActions::SpawnBotCmd
