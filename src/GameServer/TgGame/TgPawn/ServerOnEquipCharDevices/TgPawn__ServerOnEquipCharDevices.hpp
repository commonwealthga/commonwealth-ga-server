#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnEquipCharDevices : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, UTgSeqAct_EquipCharDevices*),
	0x109c0d40,
	TgPawn__ServerOnEquipCharDevices> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action) {
		m_original(Pawn, edx, Action);
	};

	// Tutorial (Inception) gear. The native is an empty stub in the
	// shipped exe (Hi-Rez's backend did the work); the map's Kismet strips all
	// hand/travel devices on arrival (m_bRemoveOtherDevices, empty list), then
	// grants the class jetpack (part 2) and class melee (part 3). Grants are
	// in-instance only - ga_character_devices is never touched, so the agent's
	// own kit is back in Dome City.
	static bool IsTutorialStripSlot(int slot);
	// Re-equip the grants this character already earned in this instance
	// (respawn after death). Call before Inventory::Finalize.
	static void ReplayTutorialGrants(ATgPawn* Pawn);
};
