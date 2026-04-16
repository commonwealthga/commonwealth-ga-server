#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgEffectGroup::RemoveEffects — original is an empty stub @ 0x10a6f3d0.
//
// UC declaration:
//   native function RemoveEffects(Actor Target, optional bool bResetToFollow);
//
// Contract: iterate m_Effects and dispatch each effect's UC `Remove(target,
// bResetToFollow)` event. That UC event reverses any property modifier the
// effect's matching `ApplyEffect` installed (TgEffect.uc:615 calls
// `ApplyToProperty(target, m_nPropertyId, m_fCurrent, true)` with bRemove=true,
// which flips the calc-method math at TgEffect.uc:413-444).
//
// Without this, our TgEffectManager::RemoveEffectGroup tears down the group
// container (HUD slot, timers, s_AppliedEffectGroups) but leaves every
// modifier the group's effects installed (power regen at 0, ground speed
// reduction, damage protection buffs, etc.) — those property values stay
// frozen forever on the target.
//
// __thiscall: ECX = this (UTgEffectGroup*). bResetToFollow is `:1` bitfield in
// the UC parm struct but the exec stub passes it as a regular dword.
class TgEffectGroup__RemoveEffects : public HookBase<
	void(__fastcall*)(UTgEffectGroup*, void*, AActor*, unsigned long),
	0x10a6f3d0,
	TgEffectGroup__RemoveEffects> {
public:
	static void __fastcall Call(UTgEffectGroup* eg, void* edx, AActor* Target, unsigned long bResetToFollow);
	static inline void __fastcall CallOriginal(UTgEffectGroup* eg, void* edx, AActor* Target, unsigned long bResetToFollow) {
		m_original(eg, edx, Target, bResetToFollow);
	};
};
