#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__RemoveAllEffectGroups : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*, UTgEffectGroup*),
	0x10a6ef30,
	TgEffectManager__RemoveAllEffectGroups> {
public:
	static void __fastcall Call(ATgEffectManager* pThis, void* edx, UTgEffectGroup* EffectGroup);
	static inline void __fastcall CallOriginal(ATgEffectManager* pThis, void* edx, UTgEffectGroup* EffectGroup) {
		m_original(pThis, edx, EffectGroup);
	};
};
