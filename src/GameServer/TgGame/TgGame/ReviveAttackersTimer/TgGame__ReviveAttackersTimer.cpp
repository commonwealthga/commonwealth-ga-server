#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__ReviveAttackersTimer::Call(ATgGame *Game, void *edx) {
	Logger::Log("debug", "TgGame::ReviveAttackersTimer called\n");

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

	// Game->SetTimer(((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseAttackers, 1, FName("ReviveAttackersTimer"), nullptr);
}

