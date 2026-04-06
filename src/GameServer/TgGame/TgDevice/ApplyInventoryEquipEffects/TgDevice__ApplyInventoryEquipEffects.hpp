#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDevice__ApplyInventoryEquipEffects : public HookBase<
	void(__fastcall*)(ATgDevice*, void*, unsigned long),
	0x10a19a70,
	TgDevice__ApplyInventoryEquipEffects> {
public:
	static void __fastcall Call(ATgDevice* Device, void* edx, unsigned long bRemove);
	static inline void __fastcall CallOriginal(ATgDevice* Device, void* edx, unsigned long bRemove) {
		m_original(Device, edx, bRemove);
	};
};
