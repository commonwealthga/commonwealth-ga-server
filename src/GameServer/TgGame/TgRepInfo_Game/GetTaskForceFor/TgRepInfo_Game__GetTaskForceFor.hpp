#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// CLIENT-SIDE DIAGNOSTIC — not a real fix.
//
// The real problem is that ATgRepInfo_Deployable property replication never
// reaches the client (verified via replicated_event.txt: 91 PRI repnotifies,
// 10 TaskForce, 27 GRI, but ZERO DRI).  Root cause is somewhere in the UE3
// net channel / bunch serialization for dynamically-spawned ReplicationInfo
// subclasses on this binary — we haven't located it yet.
//
// This hook exists so the client can still resolve team colors while that
// replication bug is being investigated.  It wraps
// `TgRepInfo_Game::GetTaskForceFor` @ 0x109f1fa0 and, when the native
// returns NULL for a deployable target (because DRI fields are unreplicated
// on client), walks the deployable's Instigator → PRI → r_TaskForce chain
// instead — all of which DO replicate.
//
// IMPORTANT: install this ONLY in dllmainclient.cpp.  The server has the
// DRI fields set correctly via direct writes in SpawnDeployableActor, so
// server-side game logic (damage attribution, IsEnemy checks for AI, etc.)
// already works via the native path and should NOT be re-routed through
// this fallback.
//
// Signature:
//   ATgRepInfo_TaskForce* __thiscall GetTaskForceFor(ATgRepInfo_Game* this, AActor* target)
class TgRepInfo_Game__GetTaskForceFor : public HookBase<
	ATgRepInfo_TaskForce*(__fastcall*)(ATgRepInfo_Game*, void*, AActor*),
	0x109f1fa0,
	TgRepInfo_Game__GetTaskForceFor> {
public:
	static ATgRepInfo_TaskForce* __fastcall Call(ATgRepInfo_Game* pThis, void* edx, AActor* target);
	static inline ATgRepInfo_TaskForce* __fastcall CallOriginal(ATgRepInfo_Game* pThis, void* edx, AActor* target) {
		return m_original(pThis, edx, target);
	};
};
