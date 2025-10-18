#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__PrepopulateInventoryId : public HookBase<
	int(__fastcall*)(void*, void*, int, void*),
	0x109bdd50,
	TgInventoryManager__PrepopulateInventoryId> {
public:
	static int __fastcall Call(void* InventoryManager, void* edx, int nInventoryId, void* InventoryObject);
	static inline int __fastcall CallOriginal(void* InventoryManager, void* edx, int nInventoryId, void* InventoryObject) {
		return m_original(InventoryManager, edx, nInventoryId, InventoryObject);
	};
};

