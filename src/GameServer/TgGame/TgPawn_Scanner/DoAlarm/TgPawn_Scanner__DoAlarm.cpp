#include "src/GameServer/TgGame/TgPawn_Scanner/DoAlarm/TgPawn_Scanner__DoAlarm.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Scanner__DoAlarm::Call(ATgPawn_Scanner* Scanner, void* edx) {
	// Run the binary body first — client-side FX (alarm light, sound, etc).
	CallOriginal(Scanner, edx);

	if (Scanner == nullptr) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game == nullptr) return;

	// Scanner has no nGlobalAlarmId field in UC — pass 0, which matches
	// every factory whose n_global_alarm_id wasn't explicitly overridden
	// (i.e. all of them, currently). sTypeCode is passed as an empty FString
	// (Data=0, Count=0, Max=0) — the native treats it as default "a".
	Logger::Log("alarm",
		"[%s] DoAlarm hook firing ActivateAlarm  scanner=%s alarmId=0\n",
		Logger::GetTime(), Scanner->GetName());

	TgGame__ActivateAlarm::Call(Game, nullptr,
		(AActor*)Scanner, /*nGlobalAlarmId=*/ 0,
		/*sTypeCode_Data=*/ 0, /*sTypeCode_Count=*/ 0, /*sTypeCode_Max=*/ 0);
}
