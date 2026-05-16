#include "src/GameServer/TgGame/TgMissionObjective/SetObjectiveActive/TgMissionObjective__SetObjectiveActive.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native is a stripped stub. We mirror what the original was expected to do:
// flip both `r_bIsActive` (replicated, drives client-side UI) AND `bEnabled`
// (server-only gate). The `bEnabled` write is critical — `var bool bEnabled`
// in TgMissionObjective.uc:54 is declared without parens, so it's NOT
// editor-exposed and can only be set programmatically. Without this write
// `bEnabled` stays at the UC default of `false`, and every UC call to
// `UpdateObjectiveStatus` early-returns at the
// `if((r_bIsLocked || r_bIsPending) || !bEnabled) return false;` gate
// (TgMissionObjective.uc:484) — meaning capture-point status transitions
// silently no-op and `r_eStatus` stays stuck at its default
// TGMOS_DEFENDER_CAPTURED. Capture progression never starts.
void __fastcall TgMissionObjective__SetObjectiveActive::Call(ATgMissionObjective* Obj, void* edx, unsigned long bActive) {
	LogCallBegin();

	bool bStateChanged = Obj->r_bIsActive != bActive;

	Obj->r_bIsActive = bActive ? 1 : 0;
	Obj->bEnabled    = bActive ? 1 : 0;
	Obj->bNetDirty = 1;
	Obj->bForceNetUpdate = 1;

	if (bStateChanged) {
		Obj->OnActiveStateChanged();
	}

	LogCallEnd();
}


