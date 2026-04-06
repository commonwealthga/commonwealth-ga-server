#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerCraftItem : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963a40,
	TgPlayerController__ServerCraftItem> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nBlueprintId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nBlueprintId) {
		m_original(Controller, edx, nBlueprintId);
	};
};
