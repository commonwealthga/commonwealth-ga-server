#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Revives all attackers in the wave revive queue and re-arms the repeating timer.
// Called from PostBeginPlay (initial setup) and then periodically via SetTimer.
void __fastcall TgGame__ReviveAttackersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();

	if (Game->s_AttackerReviveList.Data != nullptr && Game->s_AttackerReviveList.Num() > 0) {
		Logger::Log("revive", "ReviveAttackersTimer: reviving %d attackers\n", Game->s_AttackerReviveList.Num());
		for (int i = 0; i < Game->s_AttackerReviveList.Num(); i++) {
			ATgPlayerController* PlayerController = (ATgPlayerController*)Game->s_AttackerReviveList.Data[i];
			if (PlayerController != nullptr) {
				Logger::Log("revive", "  Reviving attacker %s\n", PlayerController->GetFullName());
				PlayerController->eventRevive();
			}
		}
		Game->s_AttackerReviveList.Clear();
	}

	// Re-arm repeating timer for the next wave
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr && GRI->r_nSecsToAutoReleaseAttackers > 0) {
		Game->SetTimer((float)GRI->r_nSecsToAutoReleaseAttackers, 1, FName("ReviveAttackersTimer"), nullptr);
	}

	LogCallEnd();
}

