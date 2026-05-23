#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Called on the VICTIM pawn from the kill pipeline (TgEffect::TrackStats
// invokes us when Victim->Health hits 0). Killer is the pawn that landed the
// killing blow.
//
// Kill counter (always credited):
//   - Player victim → CreditPawn.r_Scores[STYPE_KILLS]      (index 1)
//   - Bot victim    → CreditPawn.r_Scores[STYPE_KILLS_BOT]  (index 10)
// CreditPawn is Killer, resolved one level to r_Owner if Killer is a pet so
// the deploying player gets the credit instead of the pet's own PRI.
//
// Assist policy (per user spec): assists are credited only when the VICTIM
// is human, and only to human assisters. The killer can be anyone — if you
// heal an enemy bot that goes on to kill a human, you assisted that kill;
// if you heal a human teammate whose turret kills someone, you assisted
// that kill too. The only thing that matters is that a real player died,
// because bot deaths aren't scoreboard-meaningful and don't deserve
// assist points.
void __fastcall TgPawn__TrackKill::Call(ATgPawn* Pawn, void* edx, ATgPawn* Killer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Killer);

	if (Killer == nullptr) {
		LogCallEnd();
		return;
	}

	// "Has client connection" — class-name strstr per
	// `feedback_bIsPlayer_unreliable`. AI bot controllers default
	// bIsPlayer=true on this build so we can't gate on that flag.
	auto IsRealPlayer = [](AController* ctrl) -> bool {
		if (!ctrl || !ctrl->Class) return false;
		const char* name = ctrl->Class->GetFullName();
		return name && strstr(name, "PlayerController") != nullptr;
	};

	// Pet → owner resolution for credit recipient.
	ATgPawn* CreditPawn = Killer;
	if (Killer->r_Owner != nullptr) {
		CreditPawn = Killer->r_Owner;
	}

	const bool victimWasHuman = IsRealPlayer(Pawn->Controller);
	const int  scoreIndex     = victimWasHuman ? 1 : 10;  // STYPE_KILLS vs STYPE_KILLS_BOT

	ATgRepInfo_Player* KillerPRI = (ATgRepInfo_Player*)CreditPawn->PlayerReplicationInfo;
	if (KillerPRI != nullptr) {
		KillerPRI->r_Scores[scoreIndex]++;
		KillerPRI->bNetDirty = 1;
		// KillerPRI->bForceNetUpdate = 1;
	}

	// Assists: only when a real player died. Killer's human-ness doesn't
	// matter — a bot killing a player can still produce human assists.
	if (!victimWasHuman) {
		LogCallEnd();
		return;
	}

	// Two assist sources:
	// 1. Players who damaged the victim recently (m_LastDamager / m_SecondToLastDamager;
	//    rotated each damage event by TgEffect::TrackStats).
	// 2. Player who last healed the killer (CreditPawn->m_LastHealer).
	ATgPawn* assistCandidates[3] = {
		Pawn->m_LastDamager,
		Pawn->m_SecondToLastDamager,
		CreditPawn->m_LastHealer
	};

	for (int i = 0; i < 3; i++) {
		ATgPawn* candidate = assistCandidates[i];
		if (candidate == nullptr || candidate == CreditPawn || candidate == Pawn) continue;

		// Only credit human assisters.
		if (!IsRealPlayer(candidate->Controller)) continue;

		// Deduplicate against earlier candidates we already credited.
		bool duplicate = false;
		for (int j = 0; j < i; j++) {
			if (assistCandidates[j] == candidate) { duplicate = true; break; }
		}
		if (duplicate) continue;

		ATgRepInfo_Player* candidatePRI = (ATgRepInfo_Player*)candidate->PlayerReplicationInfo;
		if (candidatePRI == nullptr) continue;

		candidatePRI->r_Scores[2]++;  // STYPE_ASSISTS
		candidatePRI->bNetDirty = 1;
		// candidatePRI->bForceNetUpdate = 1;

		if (Logger::IsChannelEnabled("stats")) {
			Logger::Log("stats", "Assist credited to %s for kill on %s (%s)\n",
				candidate->GetName(), Pawn->GetName(),
				(i < 2) ? "damage" : "healing");
		}
	}

	LogCallEnd();
}
