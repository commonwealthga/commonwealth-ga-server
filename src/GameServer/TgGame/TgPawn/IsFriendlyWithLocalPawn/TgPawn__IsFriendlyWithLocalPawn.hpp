#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// CLIENT-SIDE DIAGNOSTIC (not a real fix). Install only via dllmainclient.cpp
// on a developer's own test client (e.g. the spectator test account) — the
// server never has a meaningful c_LocalPC, so this is uninteresting there.
//
// Ghidra-confirmed native (2026-07-21 session). Reversed logic (param_1 =
// target pawn being evaluated, "this"):
//
//   if (this->c_LocalPC == null) return true;               // headless/server viewer
//   localPawn = Cast_TgPawn(c_LocalPC->Pawn);
//   if (this->PlayerReplicationInfo && c_LocalPC->PlayerReplicationInfo) {
//       other = localPawn;
//       if (!other) other = Cast_TgRepInfo_Player(c_LocalPC->PlayerReplicationInfo);
//       if (!other) return true;
//       return !this->IsEnemy(other);   // vtable slot 0x140, likely ATgPawn::IsEnemy(AActor*)
//   }
//   return !this->r_bInitialIsEnemy;    // cached fallback, same shape as the deployable sibling
//
// Critically: when the local viewer has no pawn (our spectator's exact
// case), this does NOT fall straight into the cached-bit fallback like the
// deployable native does — it substitutes the viewer's OWN
// PlayerReplicationInfo for `other` instead. So team-assigning a spectator's
// PRI (r_TaskForce) SHOULD reach the real IsEnemy comparison rather than the
// "everything is enemy" fallback. Empirically that still didn't unlock
// health bars in a live test, so either IsEnemy (vt[0x140]) doesn't handle a
// PlayerReplicationInfo-typed argument the way it handles a Pawn argument,
// or something upstream (PostRenderedActors population) never calls this at
// all for a pawn-less viewer. This hook logs CallOriginal's actual answer
// plus every field read above so we can tell which without further Ghidra
// archaeology.
//
// Native signature: uint __fastcall IsFriendlyWithLocalPawn(ATgPawn* this)
// Address: 0x109e5ac0
class TgPawn__IsFriendlyWithLocalPawn : public HookBase<
	unsigned int(__fastcall*)(ATgPawn*, void*),
	0x109e5ac0,
	TgPawn__IsFriendlyWithLocalPawn> {
public:
	static unsigned int __fastcall Call(ATgPawn* pawn, void* edx);
	static inline unsigned int __fastcall CallOriginal(ATgPawn* pawn, void* edx) {
		return m_original(pawn, edx);
	};
};
