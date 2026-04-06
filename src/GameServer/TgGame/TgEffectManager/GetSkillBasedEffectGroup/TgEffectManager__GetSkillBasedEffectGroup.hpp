#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__GetSkillBasedEffectGroup : public HookBase<
	UTgEffectGroup*(__fastcall*)(ATgEffectManager*, void*, int, int*),
	0x10a6ef50,
	TgEffectManager__GetSkillBasedEffectGroup> {
public:
	static UTgEffectGroup* __fastcall Call(ATgEffectManager* Manager, void* edx, int nType, int* nIndex);
	static inline UTgEffectGroup* __fastcall CallOriginal(ATgEffectManager* Manager, void* edx, int nType, int* nIndex) {
		return m_original(Manager, edx, nType, nIndex);
	};
};
