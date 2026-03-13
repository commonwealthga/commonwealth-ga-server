#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmEffectModel__LoadEffectGroupMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094a3a0,
	CAmEffectModel__LoadEffectGroupMarshal> {
public:
	static bool bPopulateDatabaseEffectGroups;
	static void __fastcall Call(void* CAmEffectGroupModelRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmEffectGroupModelRow, void* edx, void* Marshal) {
		m_original(CAmEffectGroupModelRow, edx, Marshal);
	};
};

