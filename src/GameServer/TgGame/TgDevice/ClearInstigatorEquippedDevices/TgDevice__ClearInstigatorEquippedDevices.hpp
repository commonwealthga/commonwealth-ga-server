#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDevice__ClearInstigatorEquippedDevices : public HookBase<
	void(__fastcall*)(ATgDevice*, void*),
	0x10a19a30,
	TgDevice__ClearInstigatorEquippedDevices> {
public:
	static void __fastcall Call(ATgDevice* Device, void* edx);
	static inline void __fastcall CallOriginal(ATgDevice* Device, void* edx) {
		m_original(Device, edx);
	};
};

