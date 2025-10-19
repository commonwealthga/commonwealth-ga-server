#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgGame__GetReviveTimeRemaining::Call(ATgGame* Game, void* edx, AController* Controller) {

	if (Game->s_AttackerReviveList.Data != nullptr) {
		for (int i = 0; i < Game->s_AttackerReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_AttackerReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("debug", "Reviving attacker player %s\n", PlayerController->GetFullName());
				PlayerController->eventRevive();
			}
		}

		Game->s_AttackerReviveList.Clear();
	}

	if (Game->s_DefenderReviveList.Data != nullptr) {
		for (int i = 0; i < Game->s_DefenderReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_DefenderReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("debug", "Reviving defender player %s\n", PlayerController->GetFullName());
				PlayerController->eventRevive();
			}
		}

		Game->s_DefenderReviveList.Clear();
	}

	// int remaining = (int)Game->GetRemainingTimeForTimer(FName("ReviveAttackersTimer"), nullptr);
	//
	// Logger::Log("debug", "TgGame::GetReviveTimeRemaining called, remaining time: %d\n", remaining);
	//
	// if (remaining == -1) {
	// 	Game->SetTimer(((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseAttackers, 1, FName("ReviveAttackersTimer"), nullptr);
	// 	return ((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseAttackers;
	// }
	//
	// return remaining;
}

