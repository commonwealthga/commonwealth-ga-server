#include "src/GameServer/TgGame/TgAIController/RadioAlarm/TgAIController__RadioAlarm.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <set>

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
// bSpawnOnAlarm factory at current priority — the id goes STRAIGHT into
// ResetQueue(nOverrideSpawnTableId) (factory vtable+0x380) + SpawnNextBot()
// + SetTarget(originator pawn, bAlarmBot=true).
//
// Id selection: the current action's alarm_bot_spawn_table_id (38 of the 41
// type-620 rows in asm_data_set_bot_actions carry one — e.g. Think-Tank's
// PHASE 1 CALL IN SUPPORT rows use 64/65), falling back to the controller's
// m_nGlobalAlarmId when the action carries none. The action row is read via
// m_pAction (the native row InstallChosenAction stamps at AIC+0x740);
// alarm_bot_spawn_table_id sits at +0x50 (between audio_chance +0x4C and
// generic_event_number +0x54 per the InstallChosenAction field map). The
// value is validated against the DB's set of known type-620 table ids before
// use so a wrong-offset read can never drive a bogus spawn table.

// Server-side per-alarm-id cooldown: the data-driven test-629 gate is per
// CONTROLLER, so multiple bots sharing an alarm id each fire it → add spam.
static const float kAlarmCooldownSecs = 40.0f;
static std::map<int, float> s_lastAlarmFireTime;  // alarmId -> WorldInfo.TimeSeconds

// Known alarm_bot_spawn_table_id values on type-620 actions (validation set).
static const std::set<int>& KnownAlarmSpawnTables() {
	static std::set<int> s_tables;
	static bool s_loaded = false;
	if (!s_loaded) {
		s_loaded = true;
		if (sqlite3* db = Database::GetConnection()) {
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT DISTINCT alarm_bot_spawn_table_id FROM asm_data_set_bot_actions "
				"WHERE action_type_value_id = 620 AND alarm_bot_spawn_table_id > 0;",
				-1, &stmt, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					s_tables.insert(sqlite3_column_int(stmt, 0));
				}
				sqlite3_finalize(stmt);
			}
		}
	}
	return s_tables;
}

void __fastcall TgAIController__RadioAlarm::Call(ATgAIController* AIC, void* edx) {
	if (!AIC || !AIC->Pawn) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return;

	// Current action's alarm_bot_spawn_table_id; m_nGlobalAlarmId fallback.
	int alarmId = AIC->m_nGlobalAlarmId;
	const char* actionRow = (const char*)AIC->m_pAction.Dummy;
	if (actionRow != nullptr) {
		const int tableId = *(const int*)(actionRow + 0x50);
		if (tableId > 0 && KnownAlarmSpawnTables().count(tableId)) {
			alarmId = tableId;
		}
	}

	const float now = AIC->WorldInfo ? AIC->WorldInfo->TimeSeconds : 0.0f;
	auto it = s_lastAlarmFireTime.find(alarmId);
	// `now >= it->second` defends against map travel resetting TimeSeconds.
	if (it != s_lastAlarmFireTime.end() && now >= it->second
			&& now - it->second < kAlarmCooldownSecs) {
		Logger::Log("alarm",
			"[%s] RadioAlarm SUPPRESSED (cooldown %.1fs/%.0fs): caller=%s alarmId=%d\n",
			Logger::GetTime(), now - it->second, kAlarmCooldownSecs,
			AIC->Pawn->GetName(), alarmId);
		return;
	}
	s_lastAlarmFireTime[alarmId] = now;

	Logger::Log("alarm",
		"[%s] RadioAlarm: caller=%s alarmId=%d (globalAlarmId=%d)\n",
		Logger::GetTime(), AIC->Pawn->GetName(), alarmId, AIC->m_nGlobalAlarmId);

	TgGame__ActivateAlarm::Call(Game, nullptr,
		(AActor*)AIC->Pawn, alarmId,
		/*sTypeCode_Data=*/ 0, /*sTypeCode_Count=*/ 0, /*sTypeCode_Max=*/ 0);
}
