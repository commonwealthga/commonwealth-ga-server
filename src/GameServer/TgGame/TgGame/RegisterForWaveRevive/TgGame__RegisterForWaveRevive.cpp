#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/SDK/SdkHeaders.h"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

void __fastcall TgGame__RegisterForWaveRevive::Call(ATgGame *Game, void *edx, AController *Controller) {
	TARRAY_INIT(Game, AttackerReviveList, AController*, 0x418, 32);
	TARRAY_INIT(Game, DefenderReviveList, AController*, 0x424, 32);

	ATgPlayerController* PlayerController = (ATgPlayerController*)Controller;
	ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)PlayerController->PlayerReplicationInfo;
	bool isAttacker = RepInfo->r_TaskForce->IsAttacker();

	if (isAttacker) {
		TARRAY_ADD(AttackerReviveList, Controller);
		Logger::Log("revive", "RegisterForWaveRevive: %s -> AttackerList (count=%d)\n",
			Controller->GetName(), *AttackerReviveListCountPtr);
	} else {
		TARRAY_ADD(DefenderReviveList, Controller);
		Logger::Log("revive", "RegisterForWaveRevive: %s -> DefenderList (count=%d)\n",
			Controller->GetName(), *DefenderReviveListCountPtr);
	}

	// Ensure wave revive timers are running. Some game modes (e.g. TgGame_Arena)
	// set s_UseCustomReviveTimer=true which skips timer setup in PostBeginPlay.
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr) {
		if (isAttacker && GRI->r_nSecsToAutoReleaseAttackers > 0) {
			float remaining = Game->GetRemainingTimeForTimer(FName("ReviveAttackersTimer"), nullptr);
			if (remaining < 0.0f) {
				Logger::Log("revive", "ReviveAttackersTimer not running — starting it now (%d sec)\n",
					GRI->r_nSecsToAutoReleaseAttackers);
				Actor__SetTimer::SetTimer(Game, (float)GRI->r_nSecsToAutoReleaseAttackers, true, FName("ReviveAttackersTimer"), nullptr);
			}
		}
		if (!isAttacker && GRI->r_nSecsToAutoReleaseDefenders > 0) {
			float remaining = Game->GetRemainingTimeForTimer(FName("ReviveDefendersTimer"), nullptr);
			if (remaining < 0.0f) {
				Logger::Log("revive", "ReviveDefendersTimer not running — starting it now (%d sec)\n",
					GRI->r_nSecsToAutoReleaseDefenders);
				Actor__SetTimer::SetTimer(Game, (float)GRI->r_nSecsToAutoReleaseDefenders, true, FName("ReviveDefendersTimer"), nullptr);
			}
		}
	}
}
