#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgInventoryManager__SetInventoryDirty : public HookBase<
	void(__fastcall*)(ATgInventoryManager*, void*),
	0x10a12320,
	TgInventoryManager__SetInventoryDirty> {
public:
	static void __fastcall Call(ATgInventoryManager* Manager, void* edx);
	static inline void __fastcall CallOriginal(ATgInventoryManager* Manager, void* edx) {
		m_original(Manager, edx);
	};
};
