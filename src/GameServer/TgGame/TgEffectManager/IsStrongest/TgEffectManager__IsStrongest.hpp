#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__IsStrongest : public HookBase<
	bool(__fastcall*)(ATgEffectManager*, void*, UTgEffectGroup*, unsigned long),
	0x10a6ef40,
	TgEffectManager__IsStrongest> {
public:
	static bool __fastcall Call(ATgEffectManager* Manager, void* edx, UTgEffectGroup* eg, unsigned long bConsiderLifetime);
	static inline bool __fastcall CallOriginal(ATgEffectManager* Manager, void* edx, UTgEffectGroup* eg, unsigned long bConsiderLifetime) {
		return m_original(Manager, edx, eg, bConsiderLifetime);
	};
};
