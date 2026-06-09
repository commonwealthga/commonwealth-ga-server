#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// CLIENT-SIDE DIAGNOSTIC (not a fix). Pawn sibling of the deployable
// IsFriendlyWithLocalPawn hook. Logs which of the three branches the native
// takes for a stealthed pawn and the raw pointers that decide it, so we can
// see why a cloaked enemy resolves "friendly" (friend stealth MIC). Returns
// the native's result unchanged.
//
//   1. c_LocalPC == null                       -> friendly
//   2. this->PRI && c_LocalPC->PRI both set     -> !IsEnemy(localPawn)
//   3. otherwise                                -> !r_bInitialIsEnemy
//
// Native signature: uint __thiscall IsFriendlyWithLocalPawn(ATgPawn* this)
// Address: 0x109e5ac0
class TgPawn__IsFriendlyWithLocalPawn : public HookBase<
	unsigned int(__fastcall*)(ATgPawn*, void*),
	0x109e5ac0,
	TgPawn__IsFriendlyWithLocalPawn> {
public:
	static unsigned int __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline unsigned int __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		return m_original(Pawn, edx);
	};
};
