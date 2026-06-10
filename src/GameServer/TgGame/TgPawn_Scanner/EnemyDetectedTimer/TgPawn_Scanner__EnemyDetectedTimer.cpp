#include "src/GameServer/TgGame/TgPawn_Scanner/EnemyDetectedTimer/TgPawn_Scanner__EnemyDetectedTimer.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// asm_data_set_msg_translations 32881: "@@player_name@@ has been detected."
static constexpr int kMsgIdPlayerDetected = 32881;

void __fastcall TgPawn_Scanner__EnemyDetectedTimer::Call(ATgPawn_Scanner* Scanner, void* edx) {
	CallOriginal(Scanner, edx);

	if (Scanner == nullptr) return;

	ATgPawn* Target = Scanner->r_Target;
	if (Target == nullptr || Target->PlayerReplicationInfo == nullptr) return;
	if (Target->Controller == nullptr ||
	    !ObjectClassCache::ClassNameContains(Target->Controller, "PlayerController")) return;

	Logger::Log("alarm",
		"[%s] EnemyDetectedTimer: broadcasting detected-alert for player '%ls'\n",
		Logger::GetTime(),
		Target->PlayerReplicationInfo->PlayerName.Data
			? Target->PlayerReplicationInfo->PlayerName.Data : L"<null>");

	SendAlert::Broadcast(kMsgIdPlayerDetected,
	                     /*priority=APT_Normal*/   1,
	                     /*type=ATT_Detrimental*/  2,
	                     /*duration*/              5.0f,
	                     /*name*/                  nullptr,
	                     /*playerName=detected*/   Target->PlayerReplicationInfo->PlayerName.Data);
}
