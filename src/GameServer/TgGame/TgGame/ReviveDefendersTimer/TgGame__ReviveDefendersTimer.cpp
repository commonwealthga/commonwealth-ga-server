#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__ReviveDefendersTimer::Call(ATgGame *Game, void *edx) {
	Logger::Log("debug", "TgGame::ReviveDefendersTimer called\n");

	if (Game->s_DefenderReviveList.Data != nullptr) {
		for (int i = 0; i < Game->s_DefenderReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_DefenderReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("debug", "Reviving player %s\n", PlayerController->GetFullName());
				// Game->RestartPlayer(PlayerController);
				PlayerController->eventRevive();
			}
		}

		Game->s_DefenderReviveList.Clear();
	}

	// Game->SetTimer(((ATgRepInfo_Game*)Game->GameReplicationInfo)->r_nSecsToAutoReleaseDefenders, 1, FName("ReviveDefendersTimer"), nullptr);
}

