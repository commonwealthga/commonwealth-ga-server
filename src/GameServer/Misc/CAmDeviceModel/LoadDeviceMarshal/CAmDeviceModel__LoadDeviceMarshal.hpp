#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmDeviceModel__LoadDeviceMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094f970,
	CAmDeviceModel__LoadDeviceMarshal> {
public:
	static bool bPopulateDatabaseDevices;
	static void __fastcall Call(void* CAmDeviceModelRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmDeviceModelRow, void* edx, void* Marshal) {
		m_original(CAmDeviceModelRow, edx, Marshal);
	};
};

