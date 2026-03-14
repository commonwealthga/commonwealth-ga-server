#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__NonPersistRemoveDevice : public HookBase<
	void(__fastcall*)(ATgInventoryManager*, void*, int),
	0x10a12300,
	TgInventoryManager__NonPersistRemoveDevice> {
public:
	static void __fastcall Call(	ATgInventoryManager* InventoryManager, void* edx, int nEquipPoint);
	static inline void __fastcall CallOriginal(	ATgInventoryManager* InventoryManager, void* edx, int nEquipPoint) {
		m_original(InventoryManager, edx, nEquipPoint);
	};
};


