#include "src/GameServer/TgGame/TgGame_PointRotation/UnlockObjective/TgGame_PointRotation__UnlockObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called by TgGame_Arena::RoundInProgress::ObjectiveUnlock() with nPriority=1 after the 30s delay.
// Unlocks and activates m_NextObjective (set by CalcNextObjective).
// Delegates to eventUnlockObjective which handles: r_bIsLocked, SetObjectiveActive,
// ForceNetRelevant, and TriggerEventClass(TgSeqEvent_ObjectiveStatusChanged, 4).
void __fastcall TgGame_PointRotation__UnlockObjective::Call(ATgGame_PointRotation* Game, void* edx, int nPriority) {
	LogCallBegin();

	ATgMissionObjective* Obj = Game->m_NextObjective;
	if (Obj == nullptr) {
		Logger::Log(GetLogChannel(), "Next objective is nullptr\n");
		LogCallEnd();
		return;
	}

	char className[256];
	strncpy(className, Obj->Class->GetFullName(), sizeof(className) - 1);
	className[sizeof(className) - 1] = '\0';
	Logger::Log(GetLogChannel(), "Unlocking objective %s (class: %s)\n", Obj->GetFullName(), className);
	Obj->eventUnlockObjective(1);

	// eventUpdateObjectiveStatus must dispatch to the actual class's UFunction.
	// The base TgMissionObjective version has a !bEnabled early return (bEnabled=0 by default)
	// so it returns without setting r_eStatus. Subclass overrides (e.g. KOTH) bypass that check.
	// Each SDK eventXxx wrapper looks up its own class name, so casting to the right type
	// selects the correct UFunction.
	if (strcmp(className, "Class TgGame.TgBaseObjective_KOTH") == 0)
		((ATgBaseObjective_KOTH*)Obj)->eventUpdateObjectiveStatus(1);
	else if (strcmp(className, "Class TgGame.TgMissionObjective_Proximity") == 0)
		((ATgMissionObjective_Proximity*)Obj)->eventUpdateObjectiveStatus(1);
	else if (strcmp(className, "Class TgGame.TgMissionObjective_Kismet") == 0)
		((ATgMissionObjective_Kismet*)Obj)->eventUpdateObjectiveStatus(1);
	else if (strcmp(className, "Class TgGame.TgMissionObjective_Bot") == 0)
		((ATgMissionObjective_Bot*)Obj)->eventUpdateObjectiveStatus(1);
	else
		Obj->eventUpdateObjectiveStatus(1);

	// Diagnostics: dump field values after unlock
	unsigned int flags1E4 = *(unsigned int*)((char*)Obj + 0x1E4);
	unsigned int flagsAC  = *(unsigned int*)((char*)Obj + 0xAC);
	unsigned int flagsB0  = *(unsigned int*)((char*)Obj + 0xB0);
	unsigned char eStatus = *(unsigned char*)((char*)Obj + 0x1FB);
	Logger::Log(GetLogChannel(), "Post-unlock: r_eStatus=%d 0x1E4=0x%08X (r_bIsLocked bit=0x%X r_bIsActive bit=0x%X) bNetDirty=%d bAlwaysRelevant=%d bForceNetUpdate=%d\n",
		eStatus,
		flags1E4,
		(flags1E4 >> 7) & 1,   // bit 0x80
		(flags1E4 >> 8) & 1,   // bit 0x100
		(flagsAC >> 20) & 1,   // bNetDirty bit 0x100000
		(flagsAC >> 21) & 1,   // bAlwaysRelevant bit 0x200000
		(flagsB0 >> 8) & 1     // bForceNetUpdate bit 0x100
	);

	LogCallEnd();
}


