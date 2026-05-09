#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__SubmitMitigationDamage : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*, int, int),
	0x10a70f50,
	TgEffectManager__SubmitMitigationDamage> {
public:
	static void __fastcall Call(ATgEffectManager* Manager, void* edx, int nProtectionType, int nDamage);
};
