#include "src/GameServer/TgGame/TgAIController/TargetInLOS/TgAIController__TargetInLOS.hpp"
#include "src/GameServer/Utils/BotSight/BotSight.hpp"
#include "src/GameServer/PvpBots/PvpBots.hpp"
#include "src/GameServer/WorldGroups/WorldGroups.hpp"

int __fastcall TgAIController__TargetInLOS::Call(ATgAIController* aic, void* edx) {
	// Force-field domes are handled inside TgAIController__LOSTrace.
	int result = CallOriginal(aic, edx);
	if (result == 0 && BotSight::TargetInRadius(aic)) return 1;
	if (result == 0 && PvpBots::FireSight(aic)) return PvpBots::FireAllowed(aic, true) ? 1 : 0;
	// Engaged world-group members (big scaled pawns) whose eye trace fails.
	if (WorldGroups::FireSight(aic, result != 0)) return 1;
	if (result != 0 && !PvpBots::FireAllowed(aic, true)) return 0;  // Skill pacing
	return result;
}
