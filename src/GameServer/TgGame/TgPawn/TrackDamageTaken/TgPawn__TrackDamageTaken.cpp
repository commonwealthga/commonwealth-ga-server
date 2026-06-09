#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the VICTIM pawn when they take damage (native stripped in the binary).
// Accumulates damage-taken score, and on a stealthed victim queues a ~1s reveal.
void __fastcall TgPawn__TrackDamageTaken::Call(ATgPawn* Pawn, void* edx, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDamage);

	if (nDamage > 0 && Pawn) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[3] += nDamage;  // STYPE_DAMAGETAKEN
			PRI->bNetDirty = 1;
		}

		// Reveal-on-damage: hold the stealth reveal for ~1s, then full stealth returns.
		const bool queued = Pawn->r_bIsStealthed != 0;
		if (queued) {
			TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(Pawn->r_nPawnId, 1.0f);
		}
		// DIAGNOSTIC (channel "stealth"): full stealth state of whatever was hit, so we
		// can tell how the ENEMY's invisibility works (r_bIsStealthed/make-visible system
		// vs. something else) and whether team/initialIsEnemy resolve as expected.
		// CheckIsEnemy compares PlayerReplicationInfo->r_TaskForce->r_nTaskForce (the task-force
		// number). Log THAT — that's what actually decides friend/enemy (and the MIC + reticle).
		ATgRepInfo_Player* P = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		ATgRepInfo_TaskForce* rtf = P ? P->r_TaskForce : nullptr;
		// Also dump the task-force actor's replication state: if RemoteRole=0 (ROLE_None)
		// or bAlwaysRelevant=0, the client never receives the team object and CheckIsEnemy
		// resolves everyone to "teamless → friendly".
		const int tfRemote = rtf ? (int)rtf->RemoteRole : -1;
		const int tfAlwaysRel = rtf ? (int)((*(unsigned int*)((char*)rtf + 0xAC) >> 21) & 1) : -1;
		Logger::Log("stealth",
			"[damage] pawn=%d dmg=%d stealthed=%d tfNum=%d rTaskForce=%p tfRemoteRole=%d tfAlwaysRel=%d queued=%d\n",
			Pawn->r_nPawnId, nDamage, (int)Pawn->r_bIsStealthed,
			rtf ? (int)rtf->r_nTaskForce : -1, (void*)rtf, tfRemote, tfAlwaysRel, (int)queued);
	}

	LogCallEnd();
}
