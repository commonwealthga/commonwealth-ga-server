#include "src/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the dying pawn from TgPawn::Died().
// Increments death count in r_Scores[STYPE_DEATHS].
void __fastcall TgPawn__TrackDeath::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->r_Scores[8]++;  // STYPE_DEATHS
		PRI->bNetDirty = 1;
		PRI->bForceNetUpdate = 1;
	}

	LogCallEnd();
}
