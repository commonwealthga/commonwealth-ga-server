#include "src/GameServer/TgGame/TgMissionObjective/SetObjectivePending/TgMissionObjective__SetObjectivePending.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of the stripped `SetObjectivePending` native. See header
// for context. Body is intentionally minimal — mirrors what the original was
// expected to do based on how SetObjectiveActive (sister native at +0x10) is
// structured, plus the constraint that any rep delta we want clients to act
// on within the s_nObjectiveUnlockDelay window (30s for PointRotation) needs
// `bForceNetUpdate=1` to ride the next net tick rather than the periodic
// rep schedule.
//
// Note: unlike SetObjectiveActive, we do NOT touch `bEnabled`. The UC gate
// at TgMissionObjective.uc:484 — `if((r_bIsLocked || r_bIsPending) || !bEnabled)`
// — treats r_bIsPending and !bEnabled as parallel reasons to refuse capture
// progression; flipping bEnabled here would be the wrong answer.
void __fastcall TgMissionObjective__SetObjectivePending::Call(ATgMissionObjective* Obj, void* edx, unsigned long bPending) {
	LogCallBegin();

	if (Obj == nullptr) { LogCallEnd(); return; }

	const unsigned int prev1E4 = *(unsigned int*)((char*)Obj + 0x1E4);
	const unsigned int prevAC  = *(unsigned int*)((char*)Obj + 0xAC);
	const unsigned int prevB0  = *(unsigned int*)((char*)Obj + 0xB0);

	Obj->r_bIsPending     = bPending ? 1 : 0;
	Obj->bNetDirty        = 1;
	Obj->bForceNetUpdate  = 1;

	const unsigned int new1E4 = *(unsigned int*)((char*)Obj + 0x1E4);
	const unsigned int newAC  = *(unsigned int*)((char*)Obj + 0xAC);
	const unsigned int newB0  = *(unsigned int*)((char*)Obj + 0xB0);

	const char* rawName = Obj->GetFullName();
	const std::string objName(rawName ? rawName : "<null>");

	Logger::Log("pending",
		"SetObjectivePending: obj=%s bPending=%lu "
		"0x1E4 %08X->%08X (r_bIsPending bit9: %d->%d, r_bIsLocked bit7: %d, r_bIsActive bit8: %d, bEnabled bit6: %d, r_bUsePendingState bit1: %d) "
		"0xAC %08X->%08X (bNetDirty bit20: %d, bAlwaysRelevant bit21: %d) "
		"0xB0 %08X->%08X (bForceNetUpdate bit8: %d) "
		"RemoteRole=%d NetUpdateFreq=%.2f r_eStatus=%d\n",
		objName.c_str(), bPending,
		prev1E4, new1E4,
		(prev1E4 >> 9) & 1, (new1E4 >> 9) & 1,
		(new1E4 >> 7) & 1, (new1E4 >> 8) & 1, (new1E4 >> 6) & 1, (new1E4 >> 1) & 1,
		prevAC, newAC,
		(newAC >> 20) & 1, (newAC >> 21) & 1,
		prevB0, newB0,
		(newB0 >> 8) & 1,
		(int)Obj->RemoteRole, Obj->NetUpdateFrequency, (int)Obj->r_eStatus);

	// Fire the "Selected" / "Locked" pin on map kismet's TgSeqEvent_ObjectiveStatusChanged
	// nodes for this objective. This is what drives the per-map smoke FX (and
	// any other designer-wired indicators) on PointRotation maps — the official
	// server's stripped SetObjectivePending native did this. Output indices
	// (see TgBaseObjective_KOTH UC OnObjectiveStatusChange + map kismet text):
	//   [0]=Captured, [1]=StartInProgress, [2]=StartInHold, [3]=Paused,
	//   [4]=Unlocked (fired by eventUnlockObjective), [5]=Locked, [6]=Selected.
	const int activateIndex = bPending ? 6 /* Selected */ : 5 /* Locked */;
	TArray<int> Indices;
	Indices.Clear();
	Indices.Add(activateIndex);

	int firedCount = 0;
	for (int i = 0; i < Obj->GeneratedEvents.Num(); i++) {
		USequenceEvent* Evt = Obj->GeneratedEvents.Data[i];
		if (Evt == nullptr || Evt->Class == nullptr) continue;
		const char* rawClsName = Evt->Class->GetFullName();
		if (rawClsName == nullptr) continue;
		const std::string clsName(rawClsName);
		if (clsName.find("TgSeqEvent_ObjectiveStatusChanged") == std::string::npos) continue;
		Evt->CheckActivate((AActor*)Obj, (AActor*)Obj, 0, 0, Indices);
		firedCount++;
	}
	Logger::Log("pending",
		"SetObjectivePending: fired TgSeqEvent_ObjectiveStatusChanged pin=%d on %d kismet node(s) for %s\n",
		activateIndex, firedCount, objName.c_str());

	LogCallEnd();
}
