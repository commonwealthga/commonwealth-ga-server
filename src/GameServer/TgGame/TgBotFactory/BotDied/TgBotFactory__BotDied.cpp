#include "src/GameServer/TgGame/TgBotFactory/BotDied/TgBotFactory__BotDied.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// ACTIVE — thin POST-hook only. The intact native (0x10a8cbf0) does all the
// real work (counters, kismet pins, appending timed respawn entries); we call
// it through CallOriginal and then re-arm the SpawnNextBot drain timer.
//
// Why the re-arm is needed: SpawnNextBot self-schedules only while
// m_SpawnQueue.Num() > 0. Once a roster is fully spawned the queue is empty
// and no timer is armed, so nothing is left to drain the replacement entries
// BotDied appends when the bots later die — a bRespawn factory whose bots are
// all killed while the player stays in the encounter volume never comes back.
// Every b_respawn=1 factory in map_tg_bot_factory ships f_respawn_delay=0.0,
// and UE3's AActor::SetTimer treats Rate==0 as "clear this timer"
// (UnLevTic.cpp UpdateTimers removes any entry with Rate==0), so a
// fRespawnDelay-driven kick inside the native cancels itself.
//
// Deliberately conservative: only re-arms when the factory is actually a
// respawner AND the native left pending entries behind, so it can never
// invent spawns of its own.
void __fastcall TgBotFactory__BotDied::Call(ATgBotFactory* BotFactory, void* edx, ATgPawn* Pawn, ATgAIController* AIC) {
	if (BotFactory == nullptr) return;

	CallOriginal(BotFactory, edx, Pawn, AIC);

	const int pending = BotFactory->m_SpawnQueue.Num();
	Logger::Log("tgbotfactory",
		"BotDied(post): factory %d current=%d/%d totalSpawns=%d pending=%d "
		"respawn=%d autoSpawn=%d respawnDelay=%.2f\n",
		BotFactory->m_nMapObjectId, BotFactory->nCurrentCount,
		BotFactory->nActiveCount, BotFactory->nTotalSpawns, pending,
		(int)BotFactory->bRespawn, (int)BotFactory->bAutoSpawn,
		BotFactory->fRespawnDelay);

	if (!BotFactory->bRespawn || !BotFactory->bAutoSpawn) return;
	if (pending <= 0) return;

	// SpawnNextBot honours each entry's own fSpawnTime — if nothing is due yet
	// it re-arms itself for the earliest one. So the kick delay only has to be
	// nonzero (see the Rate==0 note above); the same 0.2s floor UC
	// PostBeginPlay clamps fSpawnDelay to.
	float delay = BotFactory->fRespawnDelay;
	if (delay < 0.2f) delay = 0.2f;
	Actor__SetTimer::SetTimer(
		(AActor*)BotFactory, delay, /*bLoop=*/ false,
		FName("SpawnNextBot"), nullptr);
}

/* ‼️ RETIRED full reimplementation — superseded by the post-hook above.
// The 2026-06-10 scheduler rewrite made our queue model match retail's, so
// the INTACT native (0x10a8cbf0) does the counter/kismet work itself.
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
			(AActor*)BotFactory, delay, false,   // bLoop
			FName("SpawnNextBot"), nullptr);
	}
}
*/
