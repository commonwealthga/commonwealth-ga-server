#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__KillDeployables : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, unsigned long),
	0x109c0870,
	TgPawn__KillDeployables> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, unsigned long bAll);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, unsigned long bAll) {
		m_original(Pawn, edx, bAll);
	};

	// Destroy all personal deployables owned by Pawn's PRI.
	// Walks gri->m_Deployables (global, survives pawn respawn) and PawnList
	// (catches deployed bot pawns like the Grizzly drone). Use this instead of
	// Call(bAll=1) on team-change / profile-switch paths.
	static void KillAllOwned(ATgPawn* Pawn);

	// Destroy only what one equipped device produced — its live deployables
	// (dep->r_Owner == Device) and its pet pawns (s_nSpawnerDeviceInstId ==
	// device invId). Call BEFORE Inventory::Unequip destroys the device actor.
	static void KillFromDevice(ATgPawn* Pawn, ATgDevice* Device);
};
