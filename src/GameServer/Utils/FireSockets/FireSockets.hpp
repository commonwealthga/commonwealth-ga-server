#pragma once

#include "src/pch.hpp"

// Universal fire-socket support for the dedicated server.
//
// Retail pipeline (verified intact in the binary, see
// .planning/2026-06-11-fire-sockets-investigation.md):
//   - TgDevice::GetFireSocketName / CalcFireSocketIndexMax resolve socket
//     name + cycle size from the in-memory assembly model's FX entry list
//     (group "ShotOrigin", display_mode == CurrentFireMode, slot == equip
//     point, indexed by display_order). Their ONLY mesh dependency is using
//     c_DeviceForm->c_Mesh to discover the asm id — null server-side. We
//     substitute Instigator->r_nBodyMeshAsmId and call the binary's own
//     model query functions (this file).
//   - TgPawn::GetWeaponStartTraceLocationFromSocketOffsetInfo (intact)
//     turns the socket name into a world-space trace origin via the pawn's
//     m_TgSocketOffsetInfo data asset. The ONLY missing piece game-wide is
//     the populator of that field (it lived in the lost original-server
//     binary): EnsurePopulated() reimplements it — reads the SOI resource
//     FName the loader already parked at model+0x40 (tag 0x048A
//     SOCKET_OFFSET_INFO_RES_ID) and StaticLoadObject's the asset.
class FireSockets {
public:
	// CAmAssemblyMeshModel* for an assembly id, or null. Thin wrapper over
	// the binary's CAssemblyManager::GetAssemblyMeshModel (0x109429f0) on
	// the global manager object @ 0x1199f868. Guarded against asmId<=0
	// (the binary logs "Invalid mesh assembly id" on misses).
	static void* GetMeshModel(int asmId);

	// Socket FName for the given cycle slot, or FName(0) ('None').
	// Mirrors the stock GetFireSocketName data query (FUN_109a3530):
	// group "ShotOrigin", display_mode == fireMode, display_order ==
	// socketIndex (1-based), slot == equipPoint (1..24).
	static FName GetShotOriginSocketName(void* model, int fireMode, int socketIndex, int equipPoint);

	// Cycle size = max display_order over matching entries, default 1.
	// Mirrors the stock CalcFireSocketIndexMax data query (FUN_109a3490).
	static int GetShotOriginSocketMax(void* model, int fireMode, int equipPoint);

	// Reimplementation of the original-server m_TgSocketOffsetInfo populator.
	// No-op when already populated, no body asm, no model, or the model has
	// no SOI resource bound (model+0x40 FName == None — true for all but 12
	// assembly meshes). GC-safe: the field is a UObjectProperty on TgPawn.
	// Logs on channel "soi"; also one-shot dumps the contents of all five
	// SOI assets that exist game-wide when the channel is enabled (needed to
	// bind the two assets no asm row references — see planning doc).
	static void EnsurePopulated(ATgPawn* Pawn);
};
