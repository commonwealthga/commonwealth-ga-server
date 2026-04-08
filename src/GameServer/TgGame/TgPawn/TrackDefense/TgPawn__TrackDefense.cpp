#include "src/GameServer/TgGame/TgPawn/TrackDefense/TgPawn__TrackDefense.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the DEFENDER pawn when they mitigate damage (shields, etc.).
// Accumulates defense value in r_Scores[STYPE_DEFENSE].
void __fastcall TgPawn__TrackDefense::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, nDamage);

	if (nDamage > 0) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[7] += nDamage;  // STYPE_DEFENSE
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
