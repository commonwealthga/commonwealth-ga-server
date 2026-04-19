#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// CLIENT-SIDE DIAGNOSTIC (not a real fix).
//
// Sibling of the GetTaskForceFor client hook.  Needed because when the DRI
// fails to replicate at all (medstation: r_DRI is NULL on client), the
// native IsFriendlyWithLocalPawn short-circuits BEFORE GetTaskForceFor is
// ever called:
//
//   if (localPawn != null && deployable->r_DRI != null) {
//       return !Actor::IsEnemy(deployable, localPawn);   // DRI path
//   }
//   return !deployable->r_bInitialIsEnemy;               // fallback (r_DRI null)
//
// The fallback ignores actual team membership and just returns the cached
// "is enemy" bit we set to 1 on the server → everything renders as enemy.
// We intercept: when r_DRI is null, compute friendship ourselves by walking
// deployable.Instigator.PlayerReplicationInfo.r_TaskForce and comparing
// against the local pawn's PRI.r_TaskForce.  All three fields replicate
// reliably, unlike the DRI.
//
// Native signature: uint __thiscall IsFriendlyWithLocalPawn(ATgDeployable* this)
// Address: 0x10a19460
class TgDeployable__IsFriendlyWithLocalPawn : public HookBase<
	unsigned int(__fastcall*)(ATgDeployable*, void*),
	0x10a19460,
	TgDeployable__IsFriendlyWithLocalPawn> {
public:
	static unsigned int __fastcall Call(ATgDeployable* deployable, void* edx);
	static inline unsigned int __fastcall CallOriginal(ATgDeployable* deployable, void* edx) {
		return m_original(deployable, edx);
	};
};
