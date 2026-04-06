#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__ApplyAllEnhancementEffects : public HookBase<
	void(__fastcall*)(ATgInventoryManager*, void*, unsigned long),
	0x10a12330,
	TgInventoryManager__ApplyAllEnhancementEffects> {
public:
	static void __fastcall Call(ATgInventoryManager* Manager, void* edx, unsigned long bRemove);
	static inline void __fastcall CallOriginal(ATgInventoryManager* Manager, void* edx, unsigned long bRemove) {
		m_original(Manager, edx, bRemove);
	};
};
