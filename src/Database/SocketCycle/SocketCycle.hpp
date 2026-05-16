#pragma once

#include "src/pch.hpp"

// Server-side fix for AI-bot multi-socket weapon firing.
//
// Background: TgDevice's CalcFireSocketIndexMax + GetFireSocketName natives
// route through c_DeviceForm->c_Mesh to discover how many "ShotOrigin"
// sockets a weapon has and what their FNames are. On the dedicated server
// c_DeviceForm->c_Mesh is null for bot weapons (UMeshComponent is
// visual-only and doesn't initialize on servers), so both natives short-
// circuit and return defaults — m_nSocketMax stays at 1, GetFireSocketName
// returns 'None'. As a result, multi-cannon bots (e.g. Boss IceShrike with
// dual shoulder cannons) muzzle-flash always on socket #1 and their
// projectiles spawn from the pawn body instead of the shoulder.
//
// The data we need IS in the DB (asm_data_set_asm_mesh_fxs joined to
// asm_data_set_resources): display_order gives the cycle index, and
// socket_res_id → resources.name gives the socket FName. We load this once
// at startup, key it by (body_asm_id, equip_point), and serve lookups from
// our CalcFireSocketIndexMax / GetFireSocketName hooks.
//
// NOTE on FName lifecycle: socket names are stored as std::string in the
// cache and converted to FName lazily at LookupOriginSockets() call time.
// FName(char*) walks/mutates the engine's global GNames table, which is
// unsafe to touch until the engine has finished its own name registration
// — at DLL-injection time we can't assume it has. Constructing FNames
// only at runtime (when a device is actually firing) sidesteps the issue;
// the SDK's FName NameCache makes the repeated lookups effectively free.
namespace SocketCycle {

    // Loads the (body_asm_id, equip_point) → ordered-socket-names table
    // from the asm_data_set_asm_mesh_fxs / asm_data_set_resources tables.
    // Call once after Database::GetConnection() is live. Idempotent.
    void Init();

    // Look up the ordered list of "*Origin*" socket names for a given
    // pawn + equip point. Returns the number of names available (0 if no
    // cycle is registered). When count > 0, the caller can resolve any
    // index in [0, count) via GetOriginSocketName.
    //
    // Resolves the pawn's body_asm_id internally via
    // TgGame__SpawnBotById::m_spawnedBotIds → asm_data_set_bots.body_asm_id.
    int LookupOriginSocketCount(ATgPawn* Pawn, int equipPoint);

    // Returns the FName for socket cycle slot `index` (0-based) on the
    // pawn's (body_asm_id, equip_point) cycle. Constructs the FName on
    // first access (engine GNames is hot by the time devices are firing).
    // Returns FName(0) if the pawn / slot / index isn't registered.
    FName GetOriginSocketName(ATgPawn* Pawn, int equipPoint, int index);

    // Resolves a pawn pointer to its asm_data_set_bots.body_asm_id via
    // TgGame__SpawnBotById::m_spawnedBotIds → asm_data_set_bots lookup.
    // Returns 0 if pawn isn't a tracked bot. Cached internally per bot_id.
    int GetBodyAsmId(ATgPawn* Pawn);

}  // namespace SocketCycle
