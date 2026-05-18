#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native `Function TgGame.TgMissionObjective.SetObjectivePending` at 0x10a79650.
// Stripped in the shipped binary (the stub is `void(void) { return; }`), so a
// direct write to `r_bIsPending` from server code never replicates in time for
// the client to fire the "pending" smoke/audio FX on the next-up objective.
//
// Sister to `TgMissionObjective::SetObjectiveActive` (0x10a79660), same UC
// declaration shape (`native function SetObjectivePending(bool bPending);`),
// same calling convention. Body just mirrors what the original native almost
// certainly did: dirty the actor and force an immediate net update so the
// `r_bIsPending` delta reaches clients within one tick instead of waiting on
// the periodic rep schedule. Client-side `ReplicatedEvent('r_bIsPending')`
// then dispatches to native `UpdateFxVisibility` which activates
// `c_fxPending` on the objective mesh.
class TgMissionObjective__SetObjectivePending : public HookBase<
	void(__fastcall*)(ATgMissionObjective*, void*, unsigned long),
	0x10a79650,
	TgMissionObjective__SetObjectivePending> {
public:
	static void __fastcall Call(ATgMissionObjective* Obj, void* edx, unsigned long bPending);
	static inline void __fastcall CallOriginal(ATgMissionObjective* Obj, void* edx, unsigned long bPending) {
		m_original(Obj, edx, bPending);
	};
};
