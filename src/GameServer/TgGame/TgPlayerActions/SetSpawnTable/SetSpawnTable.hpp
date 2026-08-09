#pragma once

#include <string>

namespace TgPlayerActions::SetSpawnTableCmd {

// ‼️ DEV TOOL MASTER SWITCH — flip to false to disable `-setspawntable`
// everywhere. When false, Execute() logs + tells the caller and does nothing.
// This is the one boolean to change once open-world spawn-table authoring is
// finished; the command is otherwise unconditional (no role/permission gate).
constexpr bool kEnabled = false;

// -setspawntable <mapObjectId> <spawnTableId>
//
// Retarget ONE map-baked bot factory's spawn table at runtime so a table can be
// eyeballed in-game before it is written into a migration. Finds the
// ATgBotFactory whose m_nMapObjectId matches, suicides its live bots
// (KillBots(false), so BotDied still runs and the counters unwind), stamps
// nDefaultSpawnTableId + nSpawnTableId, then rebuilds the queue through
// TgBotFactory__ResetQueue so the new roster spawns immediately.
//
// Runtime-only: nothing is persisted. A map reload restores the DB/map values.
//
// MUST be called on the game thread (IpcClient::DrainInbound runs from
// Actor::Tick, which satisfies this).
void Execute(const std::string& session_guid, int map_object_id, int spawn_table_id);

} // namespace TgPlayerActions::SetSpawnTableCmd
