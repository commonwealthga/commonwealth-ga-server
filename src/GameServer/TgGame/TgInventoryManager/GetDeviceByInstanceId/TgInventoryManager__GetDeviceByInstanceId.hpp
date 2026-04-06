#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__GetDeviceByInstanceId : public HookBase<
	ATgDevice*(__fastcall*)(ATgInventoryManager*, void*, int),
	0x10a12310,
	TgInventoryManager__GetDeviceByInstanceId> {
public:
	static ATgDevice* __fastcall Call(ATgInventoryManager* Manager, void* edx, int nDeviceInstanceId);
	static inline ATgDevice* __fastcall CallOriginal(ATgInventoryManager* Manager, void* edx, int nDeviceInstanceId) {
		return m_original(Manager, edx, nDeviceInstanceId);
	};
};
