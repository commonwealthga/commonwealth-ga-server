#include "src/GameServer/TgGame/TgGame_Defense/FinalizeRoundScore/TgGame_Defense__FinalizeRoundScore.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense mode round scoring. Same as Arena: increment winner's point count.
// The binary stub was _notimplemented, so CallOriginal does nothing.
void __fastcall TgGame_Defense__FinalizeRoundScore::Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
	LogCallBegin();

	if (Winner != nullptr) {
		Winner->r_nCurrentPointCount++;
		Winner->bNetDirty = 1;
		Winner->bForceNetUpdate = 1;

		Logger::Log("defense", "FinalizeRoundScore: winner taskForce=%d, points=%d\n",
			Winner->r_nTaskForce, Winner->r_nCurrentPointCount);
	}

	LogCallEnd();
}
