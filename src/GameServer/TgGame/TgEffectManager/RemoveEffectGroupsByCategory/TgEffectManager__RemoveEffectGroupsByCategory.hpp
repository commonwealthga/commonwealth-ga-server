#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__RemoveEffectGroupsByCategory : public HookBase<
	bool(__fastcall*)(ATgEffectManager*, void*, int, int),
	0x10a6ef20,
	TgEffectManager__RemoveEffectGroupsByCategory> {
public:
	static bool __fastcall Call(ATgEffectManager* Manager, void* edx, int nCategoryCode, int nQuantity);
	static inline bool __fastcall CallOriginal(ATgEffectManager* Manager, void* edx, int nCategoryCode, int nQuantity) {
		return m_original(Manager, edx, nCategoryCode, nQuantity);
	};
};
