#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDevice__HasMinimumPowerPool : public HookBase<
	bool(__fastcall*)(ATgDevice*, void*, int),
	0x10a1e560,
	TgDevice__HasMinimumPowerPool> {
public:
	static bool __fastcall Call(ATgDevice* Device, void* edx, int FireModeNum);
	static inline bool __fastcall CallOriginal(ATgDevice* Device, void* edx, int FireModeNum) {
		return m_original(Device, edx, FireModeNum);
	};
};

