#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmEffectModel__LoadEffectMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x109467f0,
	CAmEffectModel__LoadEffectMarshal> {
public:
	static bool bPopulateDatabaseEffects;
	static void __fastcall Call(void* CAmEffectModelRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmEffectModelRow, void* edx, void* Marshal) {
		m_original(CAmEffectModelRow, edx, Marshal);
	};
};

