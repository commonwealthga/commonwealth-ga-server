#include "src/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the dying pawn from TgPawn::Died().
// Increments death count in r_Scores[STYPE_DEATHS].
void __fastcall TgPawn__TrackDeath::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	// -changeteam / autobalance suicide: administrative respawn, not a
	// death — no counter, no event (design 2026-06-12).
	if (Pawn != nullptr && MatchStats::ConsumeDeathSuppression(Pawn->r_nPawnId)) {
		Logger::Log("matchstats",
			"[TrackDeath] suppressed team-change death pawnId=%d\n",
			(int)Pawn->r_nPawnId);
		LogCallEnd();
		return;
	}

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->r_Scores[8]++;  // STYPE_DEATHS
		// PRI->bNetDirty = 1;
		// PRI->bForceNetUpdate = 1;
	}

	// Pending death — claimed by a KILL within 1s, else flushed as DEATH.
	MatchStats::OnDeath(Pawn);

	LogCallEnd();
}
