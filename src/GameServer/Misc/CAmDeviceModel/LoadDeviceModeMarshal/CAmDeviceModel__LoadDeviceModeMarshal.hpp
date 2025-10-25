#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmDeviceModel__LoadDeviceModeMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094ae20,
	CAmDeviceModel__LoadDeviceModeMarshal> {
public:
	static bool bPopulateDatabaseDeviceModes;
	static void __fastcall Call(void* CAmDeviceModelRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmDeviceModelRow, void* edx, void* Marshal) {
		m_original(CAmDeviceModelRow, edx, Marshal);
	};
};

