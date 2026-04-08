#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the VICTIM pawn from the native kill pipeline.
// Killer is the pawn that killed this pawn.
// Credits the killer with a kill in r_Scores[STYPE_KILLS].
// Checks victim's m_LastDamager/m_SecondToLastDamager for assist credit.
// m_LastDamager is set by UpdateDamagers which is called from the damage pipeline,
// so it's always recent (only set during active combat on this pawn).
void __fastcall TgPawn__TrackKill::Call(ATgPawn* Pawn, void* edx, ATgPawn* Killer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Killer);

	if (Killer == nullptr) {
		LogCallEnd();
		return;
	}

	// Credit the killer with a kill
	ATgRepInfo_Player* KillerPRI = (ATgRepInfo_Player*)Killer->PlayerReplicationInfo;
	if (KillerPRI != nullptr) {
		KillerPRI->r_Scores[1]++;  // STYPE_KILLS
		KillerPRI->bNetDirty = 1;
		KillerPRI->bForceNetUpdate = 1;
	}

	// Check for assists from two sources:
	// 1. Players who damaged the victim (m_LastDamager/m_SecondToLastDamager on victim)
	// 2. Players who healed the killer (m_LastHealer on killer) — healing assist
	ATgPawn* assistCandidates[3] = {
		Pawn->m_LastDamager,
		Pawn->m_SecondToLastDamager,
		Killer->m_LastHealer
	};

	for (int i = 0; i < 3; i++) {
		ATgPawn* candidate = assistCandidates[i];
		if (candidate == nullptr || candidate == Killer || candidate == Pawn) continue;

		// Deduplicate: skip if same as an earlier candidate we already credited
		bool duplicate = false;
		for (int j = 0; j < i; j++) {
			if (assistCandidates[j] == candidate) { duplicate = true; break; }
		}
		if (duplicate) continue;

		ATgRepInfo_Player* candidatePRI = (ATgRepInfo_Player*)candidate->PlayerReplicationInfo;
		if (candidatePRI == nullptr) continue;

		candidatePRI->r_Scores[2]++;  // STYPE_ASSISTS
		candidatePRI->bNetDirty = 1;
		candidatePRI->bForceNetUpdate = 1;

		Logger::Log("stats", "Assist credited to %s for kill on %s (%s)\n",
			candidate->GetName(), Pawn->GetName(),
			(i < 2) ? "damage" : "healing");
	}

	LogCallEnd();
}
