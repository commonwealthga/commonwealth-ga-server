#include "src/GameServer/TgGame/TgDevice_Morale/SendMoraleBoostMessage/TgDevice_Morale__SendMoraleBoostMessage.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Broadcasts "@@player_name@@ rallies your team!" (msgId 0x653F) to every
// member of the activator's task force (the player themselves included).
// Priority/Type chosen to match kill alerts: APT_Normal + ATT_Beneficial.
void __fastcall TgDevice_Morale__SendMoraleBoostMessage::Call(ATgDevice_Morale* Device, void* edx) {
	LogCallBegin();

	APawn* Instigator = Device ? Device->Instigator : nullptr;
	ATgRepInfo_Player* PRI = (Instigator && Instigator->PlayerReplicationInfo)
		? (ATgRepInfo_Player*)Instigator->PlayerReplicationInfo
		: nullptr;

	if (PRI != nullptr && PRI->r_TaskForce != nullptr) {
		wchar_t* playerName = PRI->PlayerName.Data;
		if (playerName && playerName[0]) {
			SendAlert::BroadcastToTaskforce(
				PRI->r_TaskForce,
				/*msgId=*/0x653F,
				/*priority=APT_Normal*/   1,
				/*type=ATT_Beneficial*/   1,
				/*duration*/              10.0f,
				/*name*/                  nullptr,
				/*playerName*/            playerName);
			Logger::Log(GetLogChannel(),
				"Morale: '%ls' rallies tf=%d\n",
				playerName, (int)PRI->r_TaskForce->r_nTaskForce);
		}
	}

	LogCallEnd();
}
