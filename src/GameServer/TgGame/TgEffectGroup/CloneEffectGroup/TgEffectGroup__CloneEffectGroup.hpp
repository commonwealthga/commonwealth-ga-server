#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgEffectGroup::CloneEffectGroup — virtual method, vtable @ 0x117072e0 slot 0x118.
// Original is an empty stub at 0x10a6f3c0 that just returns 0. UC's
// TgEffectManager::ProcessEffect → GetNewEffectGroupByApp → GetClonedEffectGroup
// chain depends on this returning a non-null new instance to apply any DoT/HoT/
// rest-style effect. Without it, nothing persistent happens.
//
// __thiscall: ECX = this (UTgEffectGroup*). Returns UTgEffectGroup* in EAX.
// Wrapped here as __fastcall(this, edx) per HookBase convention.
class TgEffectGroup__CloneEffectGroup : public HookBase<
	UTgEffectGroup* (__fastcall*)(UTgEffectGroup*, void*),
	0x10a6f3c0,
	TgEffectGroup__CloneEffectGroup> {
public:
	static UTgEffectGroup* __fastcall Call(UTgEffectGroup* eg, void* edx);
	static inline UTgEffectGroup* __fastcall CallOriginal(UTgEffectGroup* eg, void* edx) {
		return m_original(eg, edx);
	};
};
