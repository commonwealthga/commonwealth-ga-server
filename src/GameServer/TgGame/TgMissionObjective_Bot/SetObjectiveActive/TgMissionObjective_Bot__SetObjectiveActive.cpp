#include "src/GameServer/TgGame/TgMissionObjective_Bot/SetObjectiveActive/TgMissionObjective_Bot__SetObjectiveActive.hpp"
#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
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
// up with the parent path. On top of that, keep the bot pawn's lifecycle in
// lockstep with the active state — spawn on activate, despawn on deactivate —
// reproducing the stripped bot-subclass native. Despawn mirrors ResetObjective's
// teardown (TgMissionObjective_Bot.uc:77-82).
//
// Scripted bosses (s_b_auto_spawn=0) must NOT be part of the priority unlock
// sequence, or PostBeginPlay's UnlockObjective(1) + TgGame_Defense's per-objective
// UnlockObjective() loop would activate (and thus spawn) them at map load. Their
// map_object_config n_priority must be <= 0 so both unlock paths skip them; the
// map kismet ("turn boss off" / "activate boss") is then the only thing that
// flips their active state.
void __fastcall TgMissionObjective_Bot__SetObjectiveActive::Call(ATgMissionObjective_Bot* Obj, void* edx, unsigned long bActive) {
	LogCallBegin();
	if (Obj == nullptr) { LogCallEnd(); return; }

	if (bActive) {
		if (Obj->r_ObjectiveBot == nullptr) {
			TgMissionObjective_Bot__SpawnObjectiveBot::Call(Obj, nullptr);
		}
	} else if (Obj->r_ObjectiveBot != nullptr) {
		Obj->eventOnDespawnBots(nullptr);
		Obj->r_ObjectiveBot     = nullptr;
		Obj->r_ObjectiveBotInfo = nullptr;
	}

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
