#include "src/GameServer/TgGame/TgBotFactory/BotDied/TgBotFactory__BotDied.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// ‼️ RETIRED — NOT in the Makefile, Install() commented out in dllmain.
// The 2026-06-10 scheduler rewrite made our queue model match retail's, so
// the INTACT native (0x10a8cbf0) runs unhooked now. Kept on disk for
// reference only.
//
// Historical: reimplementation of TgBotFactory::BotDied (0x10a8cbf0). The
// binary's copy is INTACT but operates retail's m_SpawnQueue model, where the
// queue is a pending-respawn scheduler it APPENDS timed entries to on every
// death. Our pre-rewrite factory stack kept m_SpawnQueue as a static
// one-entry-per-group list parallel to m_SpawnGroups, so the intact appends
// grew the queue past the groups array and permanently wedged the factory
// (alarm wave 2 never spawned). This version did the same observable work in
// the OLD model:
//   - decrement factory + group counters (clamped: fallback-factory bots die
//     against factories that never spawned them — see ActorCache backfill in
//     SpawnBotById)
//   - stamp fLastKillTime, fire TgSeqEvent_BotDied / TgSeqEvent_FactoryEmpty
//     kismet pins
//   - schedule the respawn tick for bRespawn factories; the decremented
//     group counts make GroupNeedsMore() true so SpawnNextBot refills.
void __fastcall TgBotFactory__BotDied::Call(ATgBotFactory* BotFactory, void* edx, ATgPawn* Pawn, ATgAIController* AIC) {
	if (BotFactory == nullptr) return;

	if (BotFactory->nCurrentCount > 0) {
		BotFactory->nCurrentCount -= 1;
	} else {
		Logger::Log("tgbotfactory",
			"BotDied: factory %d count already 0 — death of a bot it never "
			"spawned (fallback m_pFactory), ignoring\n", BotFactory->m_nMapObjectId);
	}
	if (BotFactory->WorldInfo != nullptr) {
		BotFactory->fLastKillTime = BotFactory->WorldInfo->TimeSeconds;
	}

	// Map kismet pins (mirrors the intact body's GeneratedEvents loop).
	TArray<int> Indices;
	for (int i = 0; i < BotFactory->GeneratedEvents.Num(); i++) {
		USequenceEvent* Evt = BotFactory->GeneratedEvents.Data[i];
		if (Evt == nullptr) continue;
		if (ObjectClassCache::ClassNameContains(Evt, "TgSeqEvent_BotDied")) {
			Evt->CheckActivate((AActor*)BotFactory, (AActor*)Pawn, 0, 0, Indices);
		}
		if (BotFactory->nCurrentCount == 0 &&
		    BotFactory->nBotCount <= BotFactory->nTotalSpawns &&
		    ObjectClassCache::ClassNameContains(Evt, "TgSeqEvent_FactoryEmpty")) {
			Evt->CheckActivate((AActor*)BotFactory, (AActor*)Pawn, 0, 0, Indices);
		}
	}

	// Per-group decrement via the spawn-group index stamped by SpawnNextBot.
	if (BotFactory->nSpawnTableId > 0 && AIC != nullptr) {
		const int g = AIC->m_nFactorySpawnGroup;
		if (g >= 0 && g < BotFactory->m_SpawnGroups.Num() &&
		    BotFactory->m_SpawnGroups.Data[g].nCurrentCount > 0) {
			BotFactory->m_SpawnGroups.Data[g].nCurrentCount -= 1;
		}
	}

	Logger::Log("tgbotfactory",
		"BotDied: factory %d  current=%d/%d totalSpawns=%d  group=%d  respawn=%d\n",
		BotFactory->m_nMapObjectId, BotFactory->nCurrentCount,
		BotFactory->nActiveCount, BotFactory->nTotalSpawns,
		AIC ? AIC->m_nFactorySpawnGroup : -1, (int)BotFactory->bRespawn);

	// Retail schedules the refill tick only for bRespawn factories. Alarm
	// factories (bRespawn=0) refill via the next ActivateAlarm's ResetQueue.
	if (BotFactory->bRespawn) {
		float delay = BotFactory->fRespawnDelay;
		if (delay < 0.2f) delay = 0.2f;
		Actor__SetTimer::SetTimer(
			(AActor*)BotFactory, delay, /*bLoop=*/ false,
			FName("SpawnNextBot"), nullptr);
	}
}
