#pragma once

#include "src/pch.hpp"

// Apply the placing-pawn's buff-registry modifiers (m_EffectBuffInfo) to the
// freshly-spawned deployable's s_Properties. Bridges the gap that the binary's
// TgDeviceFire::GetPropertyValueById path leaves open for deployable-spawned
// fire modes — the deployable's own props (radius, cooldown, activation time)
// don't go through the player-pawn buff math when read by the deployable's
// internal m_FireMode, so we mutate them directly here at spawn.
//
// Mapping is the inverse of ConvertPropToPropList(BUFF_DEVICE) (FUN_109e5220):
// for every modifier prop_id in the player's registry, find the corresponding
// base prop on the deployable and apply the layered formula:
//
//   v1 = base * (1 + IP/100) + IM         // ITEM layer
//   v2 = v1   * (1 + SP/100) + SM         // SKILL layer
//   v3 = v2   * (1 + (LP+GP)/100) + LM+GM // SELF + GENERIC layer
//
// where IP/IM = sum of fItemPercentModifier / fItemModifier across all
// matching FBuffInfo entries on the pawn, etc.
//
// Idempotent: rolled-mod buffs only fire once per spawn, but this function
// also runs every spawn so multiple deployable casts each get their own
// fresh scaling.
//
// Skip when pawn is null or has no m_EffectBuffInfo.
namespace ApplyPlayerModsToDeployable {
	void Apply(ATgPawn* pawn, ATgDeployable* deployable);
}
