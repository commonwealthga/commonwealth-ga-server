#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native `Function TgGame.TgMissionObjective.LoadObjectConfig` at 0x10a7bed0.
// Unlike the *PlayerStart variant, this one is NOT a stub — the binary body
// reads an asm_data row keyed off 0x1199f868 and copies a fixed set of fields
// (m_fProximityRadius, m_fTimeToCapture, m_bPauseOnCapture, m_nPoints, …)
// into the actor. It does NOT touch r_eStatus, bEnabled, r_eDefaultCoalition,
// or nDefaultOwnerTaskForce — those come from the placeable's editor data.
//
// Our hook calls the original first, then applies map_object_config overrides
// (currently just r_eStatus) so we can tune per-instance initial state without
// re-baking the upk. Driven by columns dumped via MapDataDumper for the
// TgMissionObjective table.
class TgMissionObjective__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgMissionObjective*, void*),
	0x10a7bed0,
	TgMissionObjective__LoadObjectConfig> {
public:
	static void __fastcall Call(ATgMissionObjective* Obj, void* edx);
	static inline void __fastcall CallOriginal(ATgMissionObjective* Obj, void* edx) {
		m_original(Obj, edx);
	}
};
