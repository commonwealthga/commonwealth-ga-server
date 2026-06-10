#include "src/GameServer/TgGame/TgAIController/RadioAlarm/TgAIController__RadioAlarm.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of the stripped TgAIController::RadioAlarm native.
//
// Fired by UC ExecuteAction case 620 ("CALL ALARM / CALL RESPONDERS / CALL IN
// SUPPORT" actions — bosses, scanners, colony eyes, champions). UC stamps
// m_fLastRadioAlarmTime first, so the data-driven cooldown (test 629
// TIME_SINCE_RADIO_ALARM) gates re-fires; no dedupe needed here.
//
// The retail body delegates to the INTACT TgGame::ActivateAlarm (0x10a75740):
// fires the alarm kismet events for this id, then drives the CLOSEST
// bSpawnOnAlarm factory at current priority — ResetQueue(alarmId) +
// SpawnNextBot() + SetTarget(originator pawn, bAlarmBot=true).
void __fastcall TgAIController__RadioAlarm::Call(ATgAIController* AIC, void* edx) {
	if (!AIC || !AIC->Pawn) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return;

	Logger::Log("alarm",
		"[%s] RadioAlarm: caller=%s alarmId=%d\n",
		Logger::GetTime(), AIC->Pawn->GetName(), AIC->m_nGlobalAlarmId);

	TgGame__ActivateAlarm::Call(Game, nullptr,
		(AActor*)AIC->Pawn, AIC->m_nGlobalAlarmId,
		/*sTypeCode_Data=*/ 0, /*sTypeCode_Count=*/ 0, /*sTypeCode_Max=*/ 0);
}
