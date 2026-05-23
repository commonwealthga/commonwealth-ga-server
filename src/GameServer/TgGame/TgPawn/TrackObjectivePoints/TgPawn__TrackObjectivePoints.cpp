#include "src/GameServer/TgGame/TgPawn/TrackObjectivePoints/TgPawn__TrackObjectivePoints.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Credits objective points to a pawn's scoreboard and shows the blue ^N^
// floating-number popup over the pawn (event 0xa49b — style-1 generic obj
// popup; see SendCombatMessage::Type::OBJ_POINTS).
//
// UC caller surface in our build: only TgEffectDamage.uc:261 (beacon-carrier
// kill: 2 × victim's max HP). Capture-point ticks and damage-near-point
// route through our own hooks (TgMissionObjective_Proximity::ScoreObjectiveProgress
// reimpl + TgEffect::TrackStats) which call SendCombatMessage themselves —
// we don't double-credit since they DON'T call TrackObjectivePoints.
void __fastcall TgPawn__TrackObjectivePoints::Call(ATgPawn* Pawn, void* edx, int nPoints) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nPoints);

	if (nPoints > 0 && Pawn != nullptr) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[9] += nPoints;  // STYPE_OBJS
			PRI->bNetDirty = 1;
		}

		ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
		if (Game != nullptr) {
			Game->eventSendCombatMessage(0x59F4, nullptr, Pawn, nPoints, 0);
		}

		// SendCombatMessage::Call(Pawn, /*Source=*/nullptr, /*Target=*/Pawn,
		//                         nPoints, SendCombatMessage::Type::OBJ_POINTS);
	}

	LogCallEnd();
}
