#pragma once

#include <string>

namespace TgPlayerActions::DeployCmd {

enum class Team {
    Friend,  // requesting player's task force
    Enemy,   // opposing task force
};

// Spawn a deployable in front of the requesting player and bind it to a task
// force. Resolves the player's pawn via GClientConnectionsData using
// session_guid, places the deployable ~500uu ahead at ground level, then runs
// SpawnDeployableActor on the player's pawn. For Team::Enemy, post-processes
// the DRI team fields to flip ownership to the opposing task force; the
// Instigator is also nulled out so damage attribution doesn't credit the
// requesting player. Silent on success and on every failure path; all
// outcomes logged on the "chat-command" channel.
//
// MUST be called on the game thread (IpcClient::DrainInbound is invoked from
// Actor::Tick, which satisfies this precondition).
void Execute(const std::string& session_guid, int deployable_id, Team team);

} // namespace TgPlayerActions::DeployCmd
