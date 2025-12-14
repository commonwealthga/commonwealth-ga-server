#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerAcceptNewProfileFromEquipScreen : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, FTGEQUIP_SLOTS_STRUCT),
	0x10963040,
	TgPlayerController__ServerAcceptNewProfileFromEquipScreen> {
public:
	static void __fastcall Call(ATgPlayerController* PlayerController, void* edx, int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray);
	static inline void __fastcall CallOriginal(ATgPlayerController* PlayerController, void* edx, int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray) {
		m_original(PlayerController, edx, nProfileId, DeviceArray);
	}
};

