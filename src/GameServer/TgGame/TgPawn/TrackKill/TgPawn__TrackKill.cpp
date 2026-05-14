#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Called on the VICTIM pawn from the native kill pipeline.
// Killer is the pawn that killed this pawn.
//
// Credits one of two counters on the killer's PRI depending on victim type:
//   - Player victim → r_Scores[STYPE_KILLS]      (index 1)
//   - Bot victim    → r_Scores[STYPE_KILLS_BOT]  (index 10)
//
// Pet credit: if Killer is owned (r_Owner != null), the owning player's PRI
// gets the credit instead of the pet's own PRI. Mirrors the UC pet-kill
// branches in TgPawn.Died.
//
// Checks victim's m_LastDamager/m_SecondToLastDamager for assist credit.
// m_LastDamager is set by UpdateDamagers which is called from the damage
// pipeline, so it's always recent (only set during active combat on this pawn).
void __fastcall TgPawn__TrackKill::Call(ATgPawn* Pawn, void* edx, ATgPawn* Killer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Killer);

	if (Killer == nullptr) {
		LogCallEnd();
		return;
	}

	// Resolve pet → owner so the player gets credit for their pet's kill.
	ATgPawn* CreditPawn = Killer;
	if (Killer->r_Owner != nullptr) {
		CreditPawn = Killer->r_Owner;
	}

	// Choose counter by victim type. `Controller->bIsPlayer` is unreliable
	// in this build (AI bots default it to true; clearing it breaks turret
	// targeting). Use class-name strstr — same pattern as SendCombatMessage.
	bool victimWasPlayer = false;
	if (Pawn->Controller && Pawn->Controller->Class) {
		const char* clsName = Pawn->Controller->Class->GetFullName();
		victimWasPlayer = clsName && strstr(clsName, "PlayerController") != nullptr;
	}
	const int  scoreIndex = victimWasPlayer ? 1 : 10;  // KILLS vs KILLS_BOT

	ATgRepInfo_Player* KillerPRI = (ATgRepInfo_Player*)CreditPawn->PlayerReplicationInfo;
	if (KillerPRI != nullptr) {
		KillerPRI->r_Scores[scoreIndex]++;
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

		if (Logger::IsChannelEnabled("stats")) {
			Logger::Log("stats", "Assist credited to %s for kill on %s (%s)\n",
				candidate->GetName(), Pawn->GetName(),
				(i < 2) ? "damage" : "healing");
		}
	}

	LogCallEnd();
}
