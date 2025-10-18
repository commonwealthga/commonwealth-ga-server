#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgGame__GetReviveTimeRemaining::Call(ATgGame* Game, void* edx, AController* Controller) {
	int remaining = (int)Game->GetRemainingTimeForTimer(FName("ReviveAttackersTimer"), nullptr);

	Logger::Log("debug", "TgGame::GetReviveTimeRemaining called, remaining time: %d\n", remaining);

	if (remaining == -1) {
		Game->SetTimer(((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseAttackers, 1, FName("ReviveAttackersTimer"), nullptr);
		return ((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseAttackers;
	}

	return remaining;
}

