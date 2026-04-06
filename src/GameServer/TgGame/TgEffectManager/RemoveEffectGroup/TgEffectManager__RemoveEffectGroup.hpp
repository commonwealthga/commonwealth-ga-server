#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__RemoveEffectGroup : public HookBase<
	bool(__fastcall*)(ATgEffectManager*, void*, UTgEffectGroup*),
	0x10a6ef10,
	TgEffectManager__RemoveEffectGroup> {
public:
	static bool __fastcall Call(ATgEffectManager* Manager, void* edx, UTgEffectGroup* EffectGroup);
	static inline bool __fastcall CallOriginal(ATgEffectManager* Manager, void* edx, UTgEffectGroup* EffectGroup) {
		return m_original(Manager, edx, EffectGroup);
	};
};
