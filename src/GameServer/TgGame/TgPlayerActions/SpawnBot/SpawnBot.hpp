#pragma once

#include <string>

namespace TgPlayerActions::SpawnBotCmd {

enum class Team {
    Friend,  // requesting player's task force
    Enemy,   // opposing task force
};

// Spawn a bot in front of the requesting player and assign it to a task force.
// Resolves the player's pawn via GClientConnectionsData using session_guid,
// reads the player's current task force from PlayerReplicationInfo to map
// Friend/Enemy onto the concrete Attackers/Defenders TaskForce, places the bot
// a fixed distance ahead along the player's yaw, then runs the SpawnBotById
// path. Silent (no IPC reply) on success and on every failure path; all
// outcomes logged on the "chat-command" channel.
//
// difficulty_scalar_override: 0 = use map default (Config::GetDifficultyScalar);
// non-zero = apply this exact scalar to HP and outgoing damage on this bot,
// regardless of map difficulty. Difficulty scaling is always applied (Friend
// AND Enemy bots get BBM × scalar), since the chat command is intentionally
// dev/test-facing and the user explicitly opted in.
//
// henchman (-spawnhenchman): Friend-team spawn additionally flagged
// r_bIsHenchman with the requesting player set as its leader (the AI
// controller's m_pOwner) — the behavior system's follow-owner / defend-owner
// actions key off that pointer.
//
// MUST be called on the game thread (IpcClient::DrainInbound is invoked from
// Actor::Tick, which satisfies this precondition).
void Execute(const std::string& session_guid, int bot_id, Team team,
             float difficulty_scalar_override, bool henchman = false);

} // namespace TgPlayerActions::SpawnBotCmd
