#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Revives all defenders in the wave revive queue and re-arms the repeating timer.
// Called from PostBeginPlay (initial setup) and then periodically via SetTimer.
void __fastcall TgGame__ReviveDefendersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();

	if (Game->s_DefenderReviveList.Data != nullptr && Game->s_DefenderReviveList.Num() > 0) {
		Logger::Log("revive", "ReviveDefendersTimer: reviving %d defenders\n", Game->s_DefenderReviveList.Num());
		for (int i = 0; i < Game->s_DefenderReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_DefenderReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("revive", "  Reviving defender %s\n", PlayerController->GetFullName());
				PlayerController->eventRevive();
			}
		}
		Game->s_DefenderReviveList.Clear();
	}

	// Re-arm repeating timer for the next wave
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr && GRI->r_nSecsToAutoReleaseDefenders > 0) {
		Game->SetTimer((float)GRI->r_nSecsToAutoReleaseDefenders, 1, FName("ReviveDefendersTimer"), nullptr);
	}

	LogCallEnd();
}

