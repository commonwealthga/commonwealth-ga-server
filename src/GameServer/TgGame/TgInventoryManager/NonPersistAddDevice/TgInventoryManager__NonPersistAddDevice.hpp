#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__NonPersistAddDevice : public HookBase<
	ATgDevice*(__fastcall*)(ATgInventoryManager*, void*, int, int),
	0x10A122F0,
	TgInventoryManager__NonPersistAddDevice> {
public:
	static ATgDevice* __fastcall Call(ATgInventoryManager* InventoryManager, void* edx, int nDeviceId, int nEquipPoint);
	static inline ATgDevice* __fastcall CallOriginal(ATgInventoryManager* InventoryManager, void* edx, int nDeviceId, int nEquipPoint) {
		return m_original(InventoryManager, edx, nDeviceId, nEquipPoint);
	}
};

