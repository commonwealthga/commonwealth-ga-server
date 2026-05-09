#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__ProcessReactiveSkillBasedEffectGroup : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*, int, unsigned long),
	0x10a6ef60,
	TgEffectManager__ProcessReactiveSkillBasedEffectGroup> {
public:
	static void __fastcall Call(ATgEffectManager* Manager, void* edx, int nCategory, unsigned long bRemove);
};
