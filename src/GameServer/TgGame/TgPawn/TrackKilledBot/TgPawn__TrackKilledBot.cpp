#include "src/GameServer/TgGame/TgPawn/TrackKilledBot/TgPawn__TrackKilledBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the KILLER pawn when they kill a bot.
// Increments bot kill count in r_Scores[STYPE_KILLS_BOT].
void __fastcall TgPawn__TrackKilledBot::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->r_Scores[10]++;  // STYPE_KILLS_BOT
		PRI->bNetDirty = 1;
	}

	LogCallEnd();
}
