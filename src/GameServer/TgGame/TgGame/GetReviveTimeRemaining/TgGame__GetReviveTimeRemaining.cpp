#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Returns the number of seconds until the next wave revive fires for this controller.
// Called from TgPlayerController::Dead.BeginState after RegisterForWaveRevive.
// The result is sent to the client via ClientUpdateTimeRemaining so the death screen
// shows a countdown.
int __fastcall TgGame__GetReviveTimeRemaining::Call(ATgGame* Game, void* edx, AController* Controller) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) {
		LogCallEnd();
		return 0;
	}

	// Determine which timer applies to this controller
	ATgPlayerController* PC = (ATgPlayerController*)Controller;
	ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)PC->PlayerReplicationInfo;

	bool bIsAttacker = false;
	if (RepInfo != nullptr && RepInfo->r_TaskForce != nullptr) {
		bIsAttacker = RepInfo->r_TaskForce->IsAttacker();
	}

	FName TimerName = bIsAttacker ? FName("ReviveAttackersTimer") : FName("ReviveDefendersTimer");
	int autoReleaseSecs = bIsAttacker ? GRI->r_nSecsToAutoReleaseAttackers : GRI->r_nSecsToAutoReleaseDefenders;

	float remaining = Game->GetRemainingTimeForTimer(TimerName, nullptr);

	Logger::Log("revive", "GetReviveTimeRemaining controller=%s isAttacker=%d remaining=%.1f autoRelease=%d\n",
		Controller->GetName(), bIsAttacker, remaining, autoReleaseSecs);

	// If the timer isn't running yet (returns -1 or 0), return the full auto-release interval
	if (remaining <= 0.0f) {
		LogCallEnd();
		return autoReleaseSecs;
	}

	LogCallEnd();
	return (int)remaining;
}

