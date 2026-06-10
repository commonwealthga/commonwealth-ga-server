#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <string>

void __fastcall TgGame__ActivateAlarm::Call(ATgGame* Game, void* edx,
	AActor* Originator, int nGlobalAlarmId,
	int sTypeCode_Data, int sTypeCode_Count, int sTypeCode_Max) {

	// Diagnostic snapshot BEFORE the native runs — Originator details +
	// counts of factories that COULD activate (matching bSpawnOnAlarm). Then
	// run the native; the per-factory SpawnNextBot logs will reveal which
	// ones actually fired as a side-effect.
	std::string origName = "<null>";
	std::string origClass = "<null>";
	if (Originator != nullptr) {
		const char* n = Originator->GetName();
		if (n) origName = n;
		if (Originator->Class != nullptr) {
			const char* c = Originator->Class->GetFullName();
			if (c) origClass = c;
		}
	}

	int alarmEligible = 0, alarmMatchingId = 0;
	if (Game != nullptr) {
		for (int i = 0; i < Game->s_ActorFactories.Count; i++) {
			ATgActorFactory* af = Game->s_ActorFactories.Data[i];
			if (af == nullptr || af->Class == nullptr) continue;
			const char* cn = af->Class->GetFullName();
			if (cn == nullptr) continue;
			if (std::string(cn).find("TgBotFactory") == std::string::npos) continue;
			ATgBotFactory* bf = (ATgBotFactory*)af;
			if (!bf->bSpawnOnAlarm) continue;
			alarmEligible++;
			if (bf->nGlobalAlarmId == nGlobalAlarmId) alarmMatchingId++;
		}
	}

	Logger::Log("alarm",
		"[%s] ActivateAlarm  originator=%s (%s)  alarmId=%d  "
		"factories: alarm-eligible=%d  matching-id=%d  s_nCurrentPriority=%d\n",
		Logger::GetTime(), origName.c_str(), origClass.c_str(), nGlobalAlarmId,
		alarmEligible, alarmMatchingId, Game ? Game->s_nCurrentPriority : -999);

	CallOriginal(Game, edx, Originator, nGlobalAlarmId,
		sTypeCode_Data, sTypeCode_Count, sTypeCode_Max);

	// Post-native dump of every alarm-eligible factory: the native picked the
	// closest one with nPriority==s_nCurrentPriority (or -1) and called
	// ResetQueue + SpawnNextBot on it — totalSpawns reveals which (if any).
	if (Game != nullptr) {
		for (int i = 0; i < Game->s_ActorFactories.Count; i++) {
			ATgActorFactory* af = Game->s_ActorFactories.Data[i];
			if (af == nullptr || af->Class == nullptr) continue;
			const char* cn = af->Class->GetFullName();
			if (cn == nullptr) continue;
			if (std::string(cn).find("TgBotFactory") == std::string::npos) continue;
			ATgBotFactory* bf = (ATgBotFactory*)af;
			if (!bf->bSpawnOnAlarm) continue;
			float dist = -1.0f;
			if (Originator != nullptr) {
				const float dx = bf->Location.X - Originator->Location.X;
				const float dy = bf->Location.Y - Originator->Location.Y;
				const float dz = bf->Location.Z - Originator->Location.Z;
				dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			}
			Logger::Log("alarm",
				"  factory mid=%d prio=%d alarmId=%d dist=%.0f  queue=%d groups=%d "
				"current=%d/%d totalSpawns=%d table=%d\n",
				bf->m_nMapObjectId, bf->nPriority, bf->nGlobalAlarmId, dist,
				bf->m_SpawnQueue.Num(), bf->m_SpawnGroups.Num(),
				bf->nCurrentCount, bf->nActiveCount, bf->nTotalSpawns,
				bf->nSpawnTableId);
		}
	}
}
