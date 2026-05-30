#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerAcceptNewProfileFromEquipScreen : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, FTGEQUIP_SLOTS_STRUCT),
	0x10963040,
	TgPlayerController__ServerAcceptNewProfileFromEquipScreen> {
public:
	// Route all logging from this hook (LogCallBegin/End + the SlotIndices /
	// MiscItems / equip-save status lines inside Call) to the "armor" channel.
	// Lets the user enable a single channel ("armor") and see the full
	// equip-screen-save flow alongside Armor::ApplyDefaultArmor's per-piece
	// log lines — the swap-variant work depends on correlating MiscItems
	// values to which armor invId the client dragged.
	static const char* GetLogChannel() { return "armor"; }

	static void __fastcall Call(ATgPlayerController* PlayerController, void* edx, int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray);
	static inline void __fastcall CallOriginal(ATgPlayerController* PlayerController, void* edx, int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray) {
		m_original(PlayerController, edx, nProfileId, DeviceArray);
	}
};

