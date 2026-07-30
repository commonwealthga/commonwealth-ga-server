#pragma once
#include <string>

// Friendly display-name lookup for ATgMissionObjective::nObjectiveId, e.g.
// turning KOTH/Ticket/Breach/SuperAgent capture-point ids into the same names
// players see on the retail HUD ("Security Gate 1", "Tower Gates", ...).
//
// The join (confirmed against a live DB copy, 2026-07-30 -- NOT what's in
// this repo's own tiny dev server.db, which predates these tables entirely):
//   asm_data_set_objectives.objective_id  = ATgMissionObjective::nObjectiveId
//     (confirmed directly in code: SuperAgent.cpp's SpawnPoint does
//      `Obj->nObjectiveId = cfg.objectiveDefId;`, and CapturePoint::objectiveDefId
//      is doc'd as "asm_data_set_objectives id")
//   asm_data_set_objectives.text_msg_id   = asm_data_set_msg_translations.msg_id
//   asm_data_set_msg_translations.message = the friendly text
// NOT name_msg_id/name_msg_translated on asm_data_set_objectives -- those hold
// an internal dev placeholder (e.g. "Him_Fac_Climate_Control01"), confirmed
// against real data, not the player-facing name.
//
// Custom-spawned point pools (KOTH's CtrPointRotation.cpp, and apparently at
// least some Payload/Escort maps) reuse the SAME nObjectiveId for every point
// in the pool, so this correctly returns the SAME name for all of them --
// callers that need to tell points apart (the overlay) must detect that
// case themselves (e.g. fall back to numbering when multiple concurrently-
// shown points resolve to an identical name) rather than treating a returned
// name as automatically unique.
namespace ObjectiveNames {

// "" if there's no row, no translation, or this DB predates these tables.
// Cached in memory after the first lookup per id -- this is static config
// data (never changes mid-process), and MissionProgressFeed calls this once
// per active objective every push (~1/sec).
std::string LookupFriendlyName(int objectiveId);

}  // namespace ObjectiveNames
