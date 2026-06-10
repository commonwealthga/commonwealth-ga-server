#include "src/GameServer/TgGame/TgPawn_Scanner/DoAlarm/TgPawn_Scanner__DoAlarm.hpp"
// #include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
// #include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Scanner__DoAlarm::Call(ATgPawn_Scanner* Scanner, void* edx) {
	// Run the binary body first — client-side FX (alarm light, sound, etc).
	CallOriginal(Scanner, edx);

	if (Scanner == nullptr) return;

	Logger::Log("alarm",
		"[%s] DoAlarm  scanner=%s (factory drive comes from behavior action 620 RadioAlarm)\n",
		Logger::GetTime(), Scanner->GetName());

	// Pre-RadioAlarm workaround — duplicated the alarm wave once behavior
	// action 10668 (RadioAlarm) started working: each detection fired
	// ActivateAlarm twice (ResetQueue+SpawnNextBot twice on the same factory).
	// ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	// if (Game == nullptr) return;
	// TgGame__ActivateAlarm::Call(Game, nullptr,
	// 	(AActor*)Scanner, /*nGlobalAlarmId=*/ 0,
	// 	/*sTypeCode_Data=*/ 0, /*sTypeCode_Count=*/ 0, /*sTypeCode_Max=*/ 0);
}
