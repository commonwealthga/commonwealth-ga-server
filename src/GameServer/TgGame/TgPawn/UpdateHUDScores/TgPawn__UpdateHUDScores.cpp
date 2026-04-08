#include "src/GameServer/TgGame/TgPawn/UpdateHUDScores/TgPawn__UpdateHUDScores.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called to refresh the HUD score display. Forces replication of r_Scores.
// Called from PostBeginPlay, Unhacked flow, and ServerUpdateStats.
void __fastcall TgPawn__UpdateHUDScores::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	// Force replication of the scores array to all clients
	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->bNetDirty = 1;
		PRI->bForceNetUpdate = 1;
	}

	LogCallEnd();
}
