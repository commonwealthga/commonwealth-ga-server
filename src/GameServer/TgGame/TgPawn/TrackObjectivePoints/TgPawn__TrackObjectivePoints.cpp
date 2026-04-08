#include "src/GameServer/TgGame/TgPawn/TrackObjectivePoints/TgPawn__TrackObjectivePoints.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on a pawn when they earn objective points.
// Accumulates in r_Scores[STYPE_OBJS].
void __fastcall TgPawn__TrackObjectivePoints::Call(ATgPawn* Pawn, void* edx, int nPoints) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nPoints);

	if (nPoints > 0) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[9] += nPoints;  // STYPE_OBJS
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
