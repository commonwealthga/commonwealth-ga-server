#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Client-side friendship re-evaluation trigger.  Called whenever the material
// should be recalculated — from DRI ReplicatedEvent(r_DeployableOwner /
// r_InstigatorInfo / r_TaskforceInfo / r_bOwnedByTaskforce) and from the
// deployable's own ReplicatedEvent(r_bInitialIsEnemy).
//
// Native signature: void TgDeployable::NotifyGroupChanged()
// Address: 0x10a19430
//
// Decompiled body (from Ghidra):
//   guard: (c_bInitialized=bit0 of +0x1D0) AND ((!m_bInDestroyedState) OR s_bDestroyedThisTick)
//   then:  bFriendly = IsFriendlyWithLocalPawn()
//          RecalculateMaterial(bFriendly)
//
// We hook it to LOG the state at the moment it's called, so we can see whether
// it fires at all, and what r_DRI / task-force state is visible client-side.
class TgDeployable__NotifyGroupChanged : public HookBase<
	void(__fastcall*)(ATgDeployable*, void*),
	0x10a19430,
	TgDeployable__NotifyGroupChanged> {
public:
	static void __fastcall Call(ATgDeployable* Deployable, void* edx);
	static inline void __fastcall CallOriginal(ATgDeployable* Deployable, void* edx) {
		m_original(Deployable, edx);
	};
};
