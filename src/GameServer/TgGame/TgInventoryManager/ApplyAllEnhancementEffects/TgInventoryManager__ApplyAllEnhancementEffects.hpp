#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__ApplyAllEnhancementEffects : public HookBase<
	void(__fastcall*)(ATgInventoryManager*, void*, int),
	0x10ad9bf0,
	TgInventoryManager__ApplyAllEnhancementEffects> {
public:
	static void __fastcall Call(ATgInventoryManager* InventoryManager, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgInventoryManager* InventoryManager, void* edx, int nPriority) {
		m_original(InventoryManager, edx, nPriority);
	};
};


