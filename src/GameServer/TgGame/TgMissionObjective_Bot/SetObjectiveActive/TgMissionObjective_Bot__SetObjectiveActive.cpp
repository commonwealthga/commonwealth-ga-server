#include "src/GameServer/TgGame/TgMissionObjective_Bot/SetObjectiveActive/TgMissionObjective_Bot__SetObjectiveActive.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native — TgMissionObjective_Bot.uc declares its own `native function
// SetObjectiveActive(bool bActive)` override (UC:28), so UC dispatch on a bot
// objective resolves to THIS function, not the parent. Without this hook the
// parent's SetObjectiveActive hook never fires for boss objectives, and the
// canonical activation chain (TgGame::PostBeginPlay → UnlockObjective(1) →
// eventUnlockObjective → SetObjectiveActive(true)) silently no-ops:
//   - bEnabled stays at 0 → super.UpdateObjectiveStatus gate at
//     `if(... || !bEnabled) return false;` (TgMissionObjective.uc:484) blocks
//     every ServerCalcStatus status flip → boss-death → status=8 transition
//     never reaches OnObjectiveStatusChange → ObjectiveTaken → BeginEndMission.
//   - r_bIsActive stays at 0 → client-side Tick gate
//     (TgMissionObjective_Bot.uc:163) blocks ClientCalcCapturePerc → boss HUD
//     healthbar reads stale c_fPercentage and never tracks damage.
//
// Mirror the parent hook (TgMissionObjective::SetObjectiveActive): flip both
// flags, force a net update, and fire OnActiveStateChanged so client-side
// repnotify side effects (InitLocalPawnForActiveState retry timer, etc) line
// up with the parent path. No bot-specific despawn-on-deactivate handling: the
// only call site that fires SetObjectiveActive(false) on a boss is the
// post-kill m_bCaptureOnlyOnce lockout inside TgMissionObjective_Bot's own
// UpdateObjectiveStatus, which runs AFTER the bot is already dead — nothing
// to despawn.
void __fastcall TgMissionObjective_Bot__SetObjectiveActive::Call(ATgMissionObjective_Bot* Obj, void* edx, unsigned long bActive) {
	LogCallBegin();
	if (Obj == nullptr) { LogCallEnd(); return; }

	bool bStateChanged = (Obj->r_bIsActive != bActive);

	Obj->r_bIsActive     = bActive ? 1 : 0;
	Obj->bEnabled        = bActive ? 1 : 0;
	Obj->bNetDirty       = 1;
	Obj->bForceNetUpdate = 1;

	if (bStateChanged) {
		Obj->OnActiveStateChanged();
	}

	LogCallEnd();
}
