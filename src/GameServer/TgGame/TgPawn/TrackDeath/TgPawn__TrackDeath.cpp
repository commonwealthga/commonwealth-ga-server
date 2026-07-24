#include "src/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
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

		// Team death counter (replicated r_nNumDeaths) — feeds the PvE
		// "Challenge Bonus" HUD element, which compares the local player's
		// taskforce r_nNumDeaths against r_nVictoryBonusLives. Real player
		// deaths only; the original native's filter is stripped. AddDeath
		// (0x109ee420) is intact in the binary.
		if (PRI->r_TaskForce != nullptr
		 && ObjectClassCache::ClassNameContains(Pawn->Controller, "PlayerController")) {
			using AddDeath_t = void(__fastcall*)(ATgRepInfo_TaskForce*, void*);
			reinterpret_cast<AddDeath_t>(0x109ee420)(PRI->r_TaskForce, nullptr);
			Logger::Log("matchstats",
				"[TrackDeath] taskforce %d team deaths -> %d\n",
				(int)PRI->r_TaskForce->r_nTaskForce,
				PRI->r_TaskForce->r_nNumDeaths);
		}
	}

	// Escape-phase ambush pacing (SuperAgent death tax). Human deaths only.
	if (Pawn->Controller != nullptr
	 && ObjectClassCache::ClassNameContains(Pawn->Controller, "PlayerController")) {
		SuperAgent::NotifyHumanDeath();
	} else {
		// Bot death: ragdoll suppression for the explode-on-death roster.
		SuperAgent::NotifyBotDeath(Pawn);
	}

	// Pending death — claimed by a KILL within 1s, else flushed as DEATH.
	MatchStats::OnDeath(Pawn);

	LogCallEnd();
}
