#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__ReviveAttackersTimer::Call(ATgGame *Game, void *edx) {
	Logger::Log("debug", "TgGame::ReviveAttackersTimer called\n");

	if (Game->s_AttackerReviveList.Data != nullptr) {
		for (int i = 0; i < Game->s_AttackerReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_AttackerReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("debug", "Reviving player %s\n", PlayerController->GetFullName());
				// Game->RestartPlayer(PlayerController);
				PlayerController->eventRevive();
			}
		}

		Game->s_AttackerReviveList.Clear();
	}
}

