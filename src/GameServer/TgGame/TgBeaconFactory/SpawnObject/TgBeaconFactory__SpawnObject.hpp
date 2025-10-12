#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBeaconFactory__SpawnObject : public HookBase<
	void(__fastcall*)(ATgBeaconFactory*, void*),
	0x10A8C260,
	TgBeaconFactory__SpawnObject> {
public:
	static void __fastcall* Call(ATgBeaconFactory* BeaconFactory, void* edx);
	static inline void __fastcall* CallOriginal(ATgBeaconFactory* BeaconFactory, void* edx) {
		m_original(BeaconFactory, edx);
	}
};

