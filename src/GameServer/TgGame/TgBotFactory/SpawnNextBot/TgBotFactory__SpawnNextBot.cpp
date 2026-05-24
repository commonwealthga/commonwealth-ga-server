#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <cstdlib>

std::map<int, ATgPawn*> TgBotFactory__SpawnNextBot::m_lastSpawnedBot;

namespace {

// Jitter radius (UE units) applied to each spawn's XY around the picked
// NavigationPoint. Keeps grouped bots from stacking exactly on top of each
// other (which the engine treats as collision when m_bIgnoreCollisionOnSpawn
// is false — bots get despawned immediately).
constexpr float kSpawnJitterRadius = 200.0f;

// Lower bound for fSpawnDelay when scheduling the next spawn. UC PostBeginPlay
// clamps fSpawnDelay to >= 0.2 already, but we belt-and-suspenders it here so
// factories with f_spawn_delay overridden to a tiny value don't melt the CPU.
constexpr float kMinSpawnDelay = 0.05f;

int PickNextLocationIndex(ATgBotFactory* f) {
	const int n = f->LocationList.Num();
	if (n <= 0 || f->LocationList.Data == nullptr) return -1;

	int startIdx;
	if (f->LocationSelection == 0) {       // RANDOM
		startIdx = rand() % n;
	} else {                                // SEQUENTIAL
		startIdx = f->s_nCurLocationIndex + 1;
		if (startIdx >= n) startIdx = 0;
	}
	for (int probe = 0; probe < n; probe++) {
		const int idx = (startIdx + probe) % n;
		if (f->LocationList.Data[idx] != nullptr) return idx;
	}
	return -1;
}

// Sentinel-aware state lookup. m_SpawnGroups[i].nMaxCount semantics:
//   0  = first visit, not yet picked / locked
//   >0 = locked to this target; full when nCurrentCount >= nMaxCount
//   <0 = skip sentinel (PickBot returned 0 for this group at this difficulty)
bool GroupNeedsMore(const FSpawnGroupDetail& g) {
	if (g.nMaxCount == 0) return true;   // unvisited, may have bots
	if (g.nMaxCount  < 0) return false;  // sentinel: skip
	return g.nCurrentCount < g.nMaxCount;
}

// Find the next queue index needing a bot, starting from current cursor.
// Returns -1 when every group is filled or sentineled.
int FindNextNeedyGroup(ATgBotFactory* f) {
	const int n = f->m_SpawnQueue.Num();
	if (n <= 0 || f->m_SpawnGroups.Num() != n) return -1;

	int startQ = f->s_nCurListIndex;
	if (startQ < 0 || startQ >= n) startQ = 0;

	// First, see if the current group still needs more (mid-fill).
	if (f->s_nCurListIndex >= 0 && f->s_nCurListIndex < n &&
	    GroupNeedsMore(f->m_SpawnGroups.Data[f->s_nCurListIndex])) {
		return f->s_nCurListIndex;
	}

	// Otherwise, probe forward (wrapping) for the next group that needs bots.
	for (int probe = 1; probe <= n; probe++) {
		const int q = (startQ + probe) % n;
		if (GroupNeedsMore(f->m_SpawnGroups.Data[q])) return q;
	}
	return -1;
}

bool AnyGroupNeedsMore(ATgBotFactory* f) {
	for (int i = 0; i < f->m_SpawnGroups.Num(); i++) {
		if (GroupNeedsMore(f->m_SpawnGroups.Data[i])) return true;
	}
	return false;
}

// Factory-wide cap. Per-group cap is implicit in GroupNeedsMore.
bool RespectsFactoryCap(ATgBotFactory* f) {
	if (f->nActiveCount > 0 && f->nCurrentCount >= f->nActiveCount) {
		Logger::Log("tgbotfactory",
			"  factory %d at active cap (%d/%d) — stopping spawn chain\n",
			f->m_nMapObjectId, f->nCurrentCount, f->nActiveCount);
		return false;
	}
	return true;
}

void ApplyXYJitter(FVector& loc) {
	const float angle  = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
	const float radius = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * kSpawnJitterRadius;
	loc.X += std::cos(angle) * radius;
	loc.Y += std::sin(angle) * radius;
}

}  // namespace

// Per-bot state machine. Each call spawns exactly ONE bot, then schedules
// itself via SetTimer(fSpawnDelay, 'SpawnNextBot') if more bots remain.
// UC PostBeginPlay / UnlockObjective / ActivateAlarm / OnToggle all kick the
// chain with a single SpawnNextBot call; the chain spreads the rest over
// time. State lives on the actor (s_nCurListIndex, m_SpawnGroups counters),
// so external re-entry (alarm during boss fight, ResetQueue) cleanly resumes.
void __fastcall TgBotFactory__SpawnNextBot::Call(ATgBotFactory* BotFactory, void* edx) {
	if (BotFactory == nullptr) return;

	const int mid = BotFactory->m_nMapObjectId;

	Logger::Log("tgbotfactory",
		"[%s] %s SpawnNextBot mapObjectId=%d (queueSize=%d locations=%d "
		"current=%d active=%d totalSpawns=%d cursor=%d)\n",
		Logger::GetTime(), BotFactory->GetName(), mid,
		BotFactory->m_SpawnQueue.Num(), BotFactory->LocationList.Num(),
		BotFactory->nCurrentCount, BotFactory->nActiveCount, BotFactory->nTotalSpawns,
		BotFactory->s_nCurListIndex);

	if (BotFactory->m_SpawnQueue.Num() == 0 || BotFactory->LocationList.Num() == 0) {
		Logger::Log("tgbotfactory", "  empty queue or LocationList — nothing to spawn\n");
		return;
	}
	if (BotFactory->m_SpawnQueue.Data == nullptr ||
	    BotFactory->LocationList.Data == nullptr) {
		Logger::Log("tgbotfactory", "  null Data pointer — skipping\n");
		return;
	}
	if (!RespectsFactoryCap(BotFactory)) return;

	const int qIdx = FindNextNeedyGroup(BotFactory);
	if (qIdx < 0) {
		Logger::Log("tgbotfactory", "  all groups filled/sentineled — spawn chain done\n");
		return;
	}
	BotFactory->s_nCurListIndex = qIdx;

	const int lIdx = PickNextLocationIndex(BotFactory);
	if (lIdx < 0) {
		Logger::Log("tgbotfactory", "  no valid location for factory %d — bailing chain\n", mid);
		return;
	}
	BotFactory->s_nCurLocationIndex = lIdx;

	FSpawnQueueEntry* entry   = &BotFactory->m_SpawnQueue.Data[qIdx];
	ANavigationPoint* BotStart = BotFactory->LocationList.Data[lIdx];
	if (BotStart == nullptr) {
		Logger::Log("tgbotfactory", "  unexpected null location after pick — bailing\n");
		return;
	}

	const int groupIndex = qIdx;
	FSpawnGroupDetail& gd = BotFactory->m_SpawnGroups.Data[groupIndex];

	// Pick a fresh bot for this group every visit. PickBot returns 0 if the
	// table has no rows at the current difficulty for this (table, group);
	// in that case we sentinel the group with nMaxCount=-1 so the chain
	// skips it on the next iteration instead of looping forever.
	int botCountForGroup = 1;
	const int botId = TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(
		entry->nSpawnTableId, entry->nGroupNumber, &botCountForGroup);
	if (botId <= 0) {
		Logger::Log("tgbotfactory",
			"  PickBot(table=%d group=%d) returned 0 — sentineling group_index=%d\n",
			entry->nSpawnTableId, entry->nGroupNumber, groupIndex);
		gd.nMaxCount = -1;
		// Try next group in the chain immediately (no delay) — no point
		// waiting fSpawnDelay just to skip a sentineled group.
		if (AnyGroupNeedsMore(BotFactory)) BotFactory->SpawnNextBot();
		return;
	}

	// First visit to this group — lock nMin/nMaxCount to the picked row's
	// bot_count so BotDied + respawn refills back to that level (and
	// CanSpawn doesn't blow past it on alarm re-triggers).
	if (gd.nMaxCount == 0) {
		gd.nMinCount = botCountForGroup;
		gd.nMaxCount = botCountForGroup;
	}

	// Build spawn location: nav point + cylinder lift + XY jitter so bots
	// don't stack and instantly despawn from collision overlap when
	// m_bIgnoreCollisionOnSpawn is off.
	FVector loc = BotStart->Location;
	{
		float radius = 0.0f, halfHeight = 0.0f;
		TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
		if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
	}
	ApplyXYJitter(loc);

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
		botId, loc, BotStart->Rotation,
		/*bKillController=*/   false,
		/*pFactory=*/          BotFactory,
		/*bIgnoreCollision=*/  true,
		/*pOwnerPawn=*/        nullptr,
		/*bIsDecoy=*/          false,
		/*deviceFire=*/        nullptr,
		/*fDeployAnimLength=*/ 0.0f);
	m_lastSpawnedBot[mid] = Bot;


	if (Bot == nullptr || Bot->Controller == nullptr) {
		Logger::Log("tgbotfactory",
			"  SpawnBotById returned null pawn/controller for bot=%d (factory %d) — skipping wiring\n",
			botId, mid);
	} else {

		ATgAIController* AIController = (ATgAIController*)Bot->Controller;
		if (BotFactory->bAlwaysPatrol) {
			for (int j = 0; j < BotFactory->PatrolPath.Num(); j++) {
				AIController->m_PatrolPath.Add(BotFactory->PatrolPath.Data[j]);
			}
			if (AIController->m_PatrolPath.Num() == 0) {
				AIController->m_PatrolPath.Add(BotStart);
			}
		} else {
			if (BotFactory->s_nTaskForce == 1 && GTeamsData.Attackers->r_CurrActiveObjective != nullptr) {
				ATgMissionObjective* Obj = (ATgMissionObjective*)GTeamsData.Attackers->r_CurrActiveObjective;
				std::string objclass = Obj->Class->GetFullName();
				if (objclass == "Class TgGame.TgMissionObjective_Bot") {
					ATgMissionObjective_Bot* BotObj = (ATgMissionObjective_Bot*)Obj;
					AIController->m_pTriggerTarget = BotObj->r_ObjectiveBot;
					AIController->m_pTriggerDestination = BotObj->r_ObjectiveBot;
					AIController->m_vTriggerLocation = BotObj->r_ObjectiveBot->Location;
					AIController->m_bInterrupt = 1;
				}
			} else if (BotFactory->s_nTaskForce == 2 && GTeamsData.Defenders->r_CurrActiveObjective != nullptr) {
				ATgMissionObjective* Obj = (ATgMissionObjective*)GTeamsData.Defenders->r_CurrActiveObjective;
				std::string objclass = Obj->Class->GetFullName();
				if (objclass == "Class TgGame.TgMissionObjective_Bot") {
					ATgMissionObjective_Bot* BotObj = (ATgMissionObjective_Bot*)Obj;
					AIController->m_pTriggerTarget = BotObj->r_ObjectiveBot;
					AIController->m_pTriggerDestination = BotObj->r_ObjectiveBot;
					AIController->m_vTriggerLocation = BotObj->r_ObjectiveBot->Location;
					AIController->m_bInterrupt = 1;
				}
			}
        }

		BotFactory->nCurrentCount += 1;
		BotFactory->nTotalSpawns  += 1;
		BotFactory->m_nSpawnOrder += 1;
		BotFactory->m_nLastGroup   = entry->nGroupNumber;
		gd.nCurrentCount          += 1;

		Logger::Log("tgbotfactory",
			"  spawned bot=%d at loc[%d] (jittered) for factory=%d  group=%d "
			"(%d/%d in group)  current=%d/%d  totalSpawns=%d\n",
			botId, lIdx, mid, entry->nGroupNumber,
			gd.nCurrentCount, gd.nMaxCount,
			BotFactory->nCurrentCount, BotFactory->nActiveCount,
			BotFactory->nTotalSpawns);
	}

	// Schedule the next bot via SetTimer if there's more to spawn. UC
	// PostBeginPlay clamps fSpawnDelay to >= 0.2; we floor to kMinSpawnDelay
	// as a safety. SetTimer fires 'SpawnNextBot' on this actor → ProcessEvent
	// → our hook again.
	if (AnyGroupNeedsMore(BotFactory) && RespectsFactoryCap(BotFactory)) {
		float delay = BotFactory->fSpawnDelay;
		if (delay < kMinSpawnDelay) delay = kMinSpawnDelay;
		Actor__SetTimer::SetTimer(
			(AActor*)BotFactory, delay, /*bLoop=*/ false,
			FName("SpawnNextBot"), nullptr);
	} else {
		Logger::Log("tgbotfactory",
			"  factory %d spawn chain complete (no more groups need bots)\n", mid);
	}
}
