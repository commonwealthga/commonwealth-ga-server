#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

// UC signature: native function ResetQueue(optional int nOverrideSpawnTableId = 0);
//
// Retail model (anchored on the INTACT BotDied/GetRemainingTotalSpawns):
// m_SpawnQueue is a scheduler — ONE entry per pending bot spawn
// {nGroupNumber = group INDEX, fSpawnTime, bRespawn, nSpawnTableId}.
// ResetQueue rolls the activation roster: per group, a chance-weighted row
// decides which bot family fields the group (or nothing, when chances sum
// below 1.0), and the roster size is rand[gmin..gmax] (or the row's
// bot_count). SpawnNextBot drains due entries; the intact BotDied appends
// timed replacements when bRespawn && bAutoSpawn.
//
// Callers: UC PostBeginPlay / OnToggle / StartEncounter (override 0),
// ActivateAlarm (override = nGlobalAlarmId — a nonzero alarm id retargets
// the responder table), InitSpawnPets (override = pet spawn table),
// UnlockObjective (override 0).
void __fastcall TgBotFactory__ResetQueue::Call(ATgBotFactory* BotFactory, void* edx, int nOverrideSpawnTableId) {
	if (BotFactory == nullptr) return;

	// Resolve the table. ResetQueue(0) restores the designer default after a
	// nonzero alarm id retargeted nSpawnTableId (nDefaultSpawnTableId is 0
	// until UC PostBeginPlay stamps it — fall back to nSpawnTableId then).
	int tableId = nOverrideSpawnTableId;
	if (tableId <= 0) {
		tableId = BotFactory->nDefaultSpawnTableId > 0
			? BotFactory->nDefaultSpawnTableId
			: BotFactory->nSpawnTableId;
	}

	Logger::Log("tgbotfactory",
		"[%s] %s ResetQueue(override=%d) mapObjectId=%d table=%d->%d alive=%d\n",
		Logger::GetTime(), BotFactory->GetName(), nOverrideSpawnTableId,
		BotFactory->m_nMapObjectId, BotFactory->nSpawnTableId, tableId,
		BotFactory->nCurrentCount);

	const bool sameTable = (tableId == BotFactory->nSpawnTableId);
	BotFactory->nSpawnTableId = tableId;

	BotFactory->m_SpawnQueue.Clear();
	// New batch — drop any escort drones tracked for the prior queue so their
	// pointers never outlive the wave they were spawned in.
	TgBotFactory__SpawnNextBot::ClearEscort(BotFactory->m_nMapObjectId);
	if (tableId <= 0) {
		BotFactory->m_SpawnGroups.Clear();
		BotFactory->nBotCount = 0;
		return;
	}

	// Preserve alive per-group counts across a same-table rebuild (second
	// alarm while wave-1 responders live; encounter restart) — the intact
	// BotDied decrements them later and they must not go stale-negative.
	std::vector<int> aliveByIndex;
	if (sameTable) {
		for (int i = 0; i < BotFactory->m_SpawnGroups.Num(); i++) {
			aliveByIndex.push_back(BotFactory->m_SpawnGroups.Data[i].nCurrentCount);
		}
	}

	const auto plan = TgBotFactory__LoadObjectConfig::RollSpawnPlan(tableId);

	BotFactory->m_SpawnGroups.Clear();
	std::vector<int> groupRolls;
	int totalEntries = 0;
	for (size_t i = 0; i < plan.size(); ++i) {
		FSpawnGroupDetail gd = plan[i].Detail;
		if (i < aliveByIndex.size()) gd.nCurrentCount = aliveByIndex[i];
		BotFactory->m_SpawnGroups.Add(gd);
		groupRolls.push_back(plan[i].RolledBotId);

		for (int k = 0; k < plan[i].EntryCount; ++k) {
			FSpawnQueueEntry entry;
			entry.nGroupNumber  = static_cast<int>(i);  // group INDEX (BotDied convention)
			entry.fSpawnTime    = 0.0f;                 // due immediately
			entry.bRespawn      = 0;
			entry.nSpawnTableId = tableId;
			BotFactory->m_SpawnQueue.Add(entry);
			totalEntries++;
		}

		Logger::Log("tgbotfactory",
			"  ResetQueue: groupIdx=%zu (group=%d) -> %s bot=%d x%d\n",
			i, plan[i].GroupNumber,
			plan[i].EntryCount == 0 ? "EMPTY roll" :
				(plan[i].RolledBotId > 0 ? "rolled" : "roster (per-entry roll)"),
			plan[i].RolledBotId, plan[i].EntryCount);
	}

	// Vanilla group semantics: the whole group — including BotDied replacement
	// entries — spawns the ONE bot its roll picked (0 = roster group).
	TgBotFactory__LoadObjectConfig::SetFactoryGroupRolls(BotFactory, groupRolls);

	// The intact BotDied's FactoryEmpty kismet pin fires when
	// nCurrentCount==0 && nBotCount <= nTotalSpawns. nTotalSpawns accumulates
	// across activations, so anchor the target to the running total: the pin
	// fires once THIS activation's roster has fully spawned and died.
	BotFactory->nBotCount = BotFactory->nTotalSpawns + totalEntries;

	// Cursors back to start.
	BotFactory->s_nCurListIndex     = -1;
	BotFactory->s_nCurLocationIndex = -1;
	BotFactory->m_nLastGroup        = 0;

	Logger::Log("tgbotfactory",
		"  ResetQueue: table=%d groups=%zu entries=%d\n",
		tableId, plan.size(), totalEntries);
}
