#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/SDK/SdkHeaders.h"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

void __fastcall TgGame__RegisterForWaveRevive::Call(ATgGame *Game, void *edx, AController *Controller) {
	TARRAY_INIT(Game, AttackerReviveList, AController*, 0x418, 32);
	TARRAY_INIT(Game, DefenderReviveList, AController*, 0x424, 32);

	// Reject non-player controllers. TgAIController.uc:4570 unconditionally
	// registers any henchman-flagged bot for wave revive, but our reimpl of
	// ReviveAttackersTimer assumes everything on the list is an
	// ATgPlayerController — it casts blindly and calls SDK's eventRevive(),
	// which dispatches TgPlayerController.Dead.Revive even on an AIController
	// (wrong UFunction). The player-style revive path then runs
	// SetLocation/PlayRespawn on a TgPawn_Turret and ultimately crashes
	// (calltree: ReviveAttackersTimer → TgPlayerController.Dead.Revive
	// [TgAIController] → TgFindPlayerStart → Pawn.Dying.BaseChange
	// [TgPawn_Turret]). Player-deployed turrets/pets aren't supposed to be
	// wave-revived in the original game anyway — pickups + redeploys handle
	// that lifecycle.
	if (Controller == nullptr || Controller->Class == nullptr ||
	    strstr(Controller->Class->GetFullName(), "PlayerController") == nullptr) {
		if (Logger::IsChannelEnabled("revive")) {
			Logger::Log("revive",
				"RegisterForWaveRevive: rejecting non-PlayerController %s (class=%s) — wave revive is player-only\n",
				Controller ? Controller->GetName() : "NULL",
				(Controller && Controller->Class) ? Controller->Class->GetFullName() : "NULL");
		}
		return;
	}

	ATgPlayerController* PlayerController = (ATgPlayerController*)Controller;
	ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)PlayerController->PlayerReplicationInfo;
	if (RepInfo == nullptr || RepInfo->r_TaskForce == nullptr) {
		Logger::Log("revive",
			"RegisterForWaveRevive: %s has no PRI/TaskForce — skipping\n",
			PlayerController->GetName());
		return;
	}
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
