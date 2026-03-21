#include "src/GameServer/TgGame/TgGame_Arena/FinalizeRoundScore/TgGame_Arena__FinalizeRoundScore.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called when a round ends with a winner. Increments that team's round win count.
// If Winner is nullptr, it's a draw — neither team scores.
void __fastcall TgGame_Arena__FinalizeRoundScore::Call(ATgGame_Arena* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
	LogCallBegin();

	if (Winner != nullptr) {
		Winner->r_nCurrentPointCount++;
		Winner->bNetDirty = 1;
		Winner->bForceNetUpdate = 1;
	}

	LogCallEnd();
}


