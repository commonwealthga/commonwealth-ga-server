#include "src/GameServer/TgGame/TgAIController/RadioAlarm/TgAIController__RadioAlarm.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of the stripped TgAIController::RadioAlarm native.
//
// Fired by UC ExecuteAction case 620 ("CALL ALARM / CALL RESPONDERS / CALL IN
// SUPPORT" actions — bosses, scanners, colony eyes, champions). UC stamps
// m_fLastRadioAlarmTime first, so the data-driven cooldown (test 629
// TIME_SINCE_RADIO_ALARM) gates re-fires per controller only — a shared
// per-alarm-id cooldown below prevents cross-controller spam.
//
// The retail body delegates to the INTACT TgGame::ActivateAlarm (0x10a75740):
// fires the alarm kismet events for this id, then drives the CLOSEST
// bSpawnOnAlarm factory at current priority — ResetQueue(alarmId) +
// SpawnNextBot() + SetTarget(originator pawn, bAlarmBot=true).
// Server-side per-alarm-id cooldown: the data-driven test-629 gate is per
// CONTROLLER, so multiple bots sharing an alarm id each fire it → add spam.
static const float kAlarmCooldownSecs = 40.0f;
static std::map<int, float> s_lastAlarmFireTime;  // alarmId -> WorldInfo.TimeSeconds

void __fastcall TgAIController__RadioAlarm::Call(ATgAIController* AIC, void* edx) {
	if (!AIC || !AIC->Pawn) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return;

	const float now = AIC->WorldInfo ? AIC->WorldInfo->TimeSeconds : 0.0f;
	auto it = s_lastAlarmFireTime.find(AIC->m_nGlobalAlarmId);
	// `now >= it->second` defends against map travel resetting TimeSeconds.
	if (it != s_lastAlarmFireTime.end() && now >= it->second
			&& now - it->second < kAlarmCooldownSecs) {
		Logger::Log("alarm",
			"[%s] RadioAlarm SUPPRESSED (cooldown %.1fs/%.0fs): caller=%s alarmId=%d\n",
			Logger::GetTime(), now - it->second, kAlarmCooldownSecs,
			AIC->Pawn->GetName(), AIC->m_nGlobalAlarmId);
		return;
	}
	s_lastAlarmFireTime[AIC->m_nGlobalAlarmId] = now;

	Logger::Log("alarm",
		"[%s] RadioAlarm: caller=%s alarmId=%d\n",
		Logger::GetTime(), AIC->Pawn->GetName(), AIC->m_nGlobalAlarmId);

	TgGame__ActivateAlarm::Call(Game, nullptr,
		(AActor*)AIC->Pawn, AIC->m_nGlobalAlarmId,
		/*sTypeCode_Data=*/ 0, /*sTypeCode_Count=*/ 0, /*sTypeCode_Max=*/ 0);
}
