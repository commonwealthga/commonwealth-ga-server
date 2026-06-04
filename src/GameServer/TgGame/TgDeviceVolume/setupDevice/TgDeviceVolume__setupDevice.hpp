#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

namespace DomeVrHealPad {
	ATgDeviceVolume* GetRegisteredVolume();
}

class TgDeviceVolume_setupDevice : public HookBase<
	bool(__fastcall*)(ATgDeviceVolume*, void*),
	0x109aeec0,
	TgDeviceVolume_setupDevice> {
public:
	static bool __fastcall Call(ATgDeviceVolume* Volume, void* edx);
	static inline bool __fastcall CallOriginal(ATgDeviceVolume* Volume, void* edx) {
		return m_original(Volume, edx);
	};
};

