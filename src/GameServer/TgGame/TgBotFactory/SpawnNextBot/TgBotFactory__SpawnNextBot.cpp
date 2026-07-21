#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

std::map<int, ATgPawn*> TgBotFactory__SpawnNextBot::m_lastSpawnedBot;
std::map<int, std::vector<TgBotFactory__SpawnNextBot::EscortDroneRef>>
	TgBotFactory__SpawnNextBot::m_escortDrones;

void TgBotFactory__SpawnNextBot::ClearEscort(int mid) {
	m_escortDrones.erase(mid);
}

namespace {

// Raid_DomeCityDefense_P juggernaut spawn table. Its repair drones idle after
// spawn because Colony_RepairDrone_AI is owner-driven (target type 344 "Owner")
// and r_Owner is unset until we wire it to the juggernaut they spawn alongside.
constexpr int kJuggernautSpawnTable = 149;

// A tracked drone is still wireable only if its slot wasn't recycled (pawnId
// still matches), it isn't pending destruction, and it's alive.
bool EscortDroneValid(const TgBotFactory__SpawnNextBot::EscortDroneRef& r) {
	return r.pawn != nullptr && !r.pawn->bDeleteMe &&
	       r.pawn->r_nPawnId == r.pawnId && r.pawn->Health > 0;
}

// Jitter radius (UE units) applied to a spawn's XY around the picked
// NavigationPoint when other bots of the same group are already out — keeps
// grouped bots from stacking. The first bot of a group anchors at the nav
// point exactly (designer-placed, guaranteed in-bounds).
constexpr float kSpawnJitterRadius = 120.0f;

// Lower bound for SetTimer delays. UC PostBeginPlay clamps fSpawnDelay to
// >= 0.2 already; belt-and-suspenders so tiny overrides don't melt the CPU.
constexpr float kMinSpawnDelay = 0.1f;

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

void ApplyXYJitter(FVector& loc) {
	const float angle  = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
	const float radius = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * kSpawnJitterRadius;
	loc.X += std::cos(angle) * radius;
	loc.Y += std::sin(angle) * radius;
}

// First queue index whose fSpawnTime is due. Returns -1 and the earliest
// future spawn time via outNextDue when nothing is due yet.
int FindDueEntry(ATgBotFactory* f, float now, float* outNextDue) {
	*outNextDue = -1.0f;
	for (int i = 0; i < f->m_SpawnQueue.Num(); i++) {
		const float t = f->m_SpawnQueue.Data[i].fSpawnTime;
		if (t <= now) return i;
		if (*outNextDue < 0.0f || t < *outNextDue) *outNextDue = t;
	}
	return -1;
}

// TArray<FSpawnQueueEntry> has no RemoveAt — entries are POD, shift down and
// shrink Count. Allocation (Max) is untouched; GAllocator still owns Data.
void RemoveQueueEntry(ATgBotFactory* f, int idx) {
	const int n = f->m_SpawnQueue.Num();
	if (idx < 0 || idx >= n) return;
	if (idx < n - 1) {
		std::memmove(&f->m_SpawnQueue.Data[idx], &f->m_SpawnQueue.Data[idx + 1],
			sizeof(FSpawnQueueEntry) * (n - 1 - idx));
	}
	f->m_SpawnQueue.Count = n - 1;
}

}  // namespace

// Retail model: drain the m_SpawnQueue scheduler. Each entry = one pending
// bot {group INDEX, due time, bRespawn, table}. ResetQueue builds the
// activation roster; the INTACT BotDied appends timed replacements (bRespawn
// && bAutoSpawn) and re-kicks us via SetTimer. We spawn the first due entry,
// remove it, and self-schedule while entries remain (bBulkSpawn drains all
// due entries in one call; pet factories never self-schedule — the
// TgAIController::SpawnPets pacemaker drives them).
void __fastcall TgBotFactory__SpawnNextBot::Call(ATgBotFactory* BotFactory, void* edx) {
	if (BotFactory == nullptr) return;

	const int mid = BotFactory->m_nMapObjectId;
	const bool isPetFactory =
		ObjectClassCache::ClassNameContains(BotFactory, "TgBotFactorySpawnable");

	Logger::Log("tgbotfactory",
		"[%s] %s SpawnNextBot mapObjectId=%d (queue=%d locations=%d "
		"current=%d active=%d totalSpawns=%d)\n",
		Logger::GetTime(), BotFactory->GetName(), mid,
		BotFactory->m_SpawnQueue.Num(), BotFactory->LocationList.Num(),
		BotFactory->nCurrentCount, BotFactory->nActiveCount, BotFactory->nTotalSpawns);

	// Despawn() parks the factory with bAutoSpawn=false + ClearQueue; a stray
	// timer firing afterwards must not spawn.
	if (!BotFactory->bAutoSpawn) {
		Logger::Log("tgbotfactory", "  bAutoSpawn=0 — factory parked\n");
		return;
	}
	if (BotFactory->m_SpawnQueue.Num() == 0 || BotFactory->LocationList.Num() == 0) {
		Logger::Log("tgbotfactory", "  empty queue or LocationList — nothing to spawn\n");
		return;
	}
	if (BotFactory->m_SpawnQueue.Data == nullptr ||
	    BotFactory->LocationList.Data == nullptr) {
		Logger::Log("tgbotfactory", "  null Data pointer — skipping\n");
		return;
	}

	AWorldInfo* WI = BotFactory->WorldInfo;
	const float now = WI ? WI->TimeSeconds : 0.0f;

	// Bounded drain: one pass over at most the current queue length (bulk
	// mode consumes every due entry; single mode breaks after one spawn).
	const int maxIterations = BotFactory->m_SpawnQueue.Num();
	int spawnedThisCall = 0;
	for (int iter = 0; iter < maxIterations; iter++) {
		// Concurrency brake. Leftover entries stay queued; BotDied's timer
		// (bRespawn factories) or the next alarm/encounter kick resumes.
		if (BotFactory->nActiveCount > 0 &&
		    BotFactory->nCurrentCount >= BotFactory->nActiveCount) {
			Logger::Log("tgbotfactory",
				"  at active cap (%d/%d) — %d entries left queued\n",
				BotFactory->nCurrentCount, BotFactory->nActiveCount,
				BotFactory->m_SpawnQueue.Num());
			return;
		}

		float nextDue = -1.0f;
		const int qIdx = FindDueEntry(BotFactory, now, &nextDue);
		if (qIdx < 0) {
			// Nothing due — wake up when the earliest replacement matures.
			if (!isPetFactory && nextDue > 0.0f) {
				float delay = nextDue - now;
				if (delay < kMinSpawnDelay) delay = kMinSpawnDelay;
				Actor__SetTimer::SetTimer(
					(AActor*)BotFactory, delay, /*bLoop=*/ false,
					FName("SpawnNextBot"), nullptr);
				Logger::Log("tgbotfactory",
					"  no due entries — next replacement in %.1fs\n", delay);
			}
			return;
		}

		const FSpawnQueueEntry entry = BotFactory->m_SpawnQueue.Data[qIdx];
		const int groupIdx    = entry.nGroupNumber;
		const int tableId     = entry.nSpawnTableId > 0 ? entry.nSpawnTableId
		                                                : BotFactory->nSpawnTableId;
		const int groupNumber =
			TgBotFactory__LoadObjectConfig::GetGroupNumberByIndex(tableId, groupIdx);

		// Vanilla: the group's bot type was rolled ONCE at ResetQueue — every
		// entry (and BotDied replacements) spawns that same bot. 0 = roster
		// group (gmin/gmax, incubators) → fresh per-entry roll.
		int botId = TgBotFactory__LoadObjectConfig::GetFactoryGroupRoll(BotFactory, groupIdx);
		if (botId <= 0) {
			botId = groupNumber >= 0
				? TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(tableId, groupNumber)
				: 0;
		}
		if (botId <= 0) {
			Logger::Log("tgbotfactory",
				"  entry (table=%d groupIdx=%d group=%d) yields no bot at current "
				"difficulty — dropping entry\n", tableId, groupIdx, groupNumber);
			RemoveQueueEntry(BotFactory, qIdx);
			continue;
		}

		const int lIdx = PickNextLocationIndex(BotFactory);
		if (lIdx < 0) {
			Logger::Log("tgbotfactory", "  no valid location for factory %d — bailing\n", mid);
			return;
		}
		BotFactory->s_nCurLocationIndex = lIdx;
		ANavigationPoint* BotStart = BotFactory->LocationList.Data[lIdx];
		if (BotStart == nullptr) return;

		FSpawnGroupDetail* gd =
			(groupIdx >= 0 && groupIdx < BotFactory->m_SpawnGroups.Num())
				? &BotFactory->m_SpawnGroups.Data[groupIdx] : nullptr;

		// Nav point + cylinder lift; XY jitter only when the group already
		// has live bots (stack avoidance) and never for pet factories — pets
		// must emerge at the exact socket location (spit animation).
		FVector loc = BotStart->Location;
		{
			float radius = 0.0f, halfHeight = 0.0f;
			TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
			if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
		}
		const bool jittered = !isPetFactory && gd != nullptr && gd->nCurrentCount > 0;
		if (jittered) ApplyXYJitter(loc);

		// Facing: nav point rotation, with optional per-factory yaw override.
		FRotator spawnRot = BotStart->Rotation;
		if (MapObjectConfig::Has(mid, "spawn_rotation_yaw")) {
			spawnRot.Yaw = MapObjectConfig::GetInt(mid, "spawn_rotation_yaw", spawnRot.Yaw);
		}

		ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
		ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
			botId, loc, spawnRot,
			/*bKillController=*/   false,
			/*pFactory=*/          BotFactory,
			/*bIgnoreCollision=*/  true,
			/*pOwnerPawn=*/        nullptr,
			/*bIsDecoy=*/          false,
			/*deviceFire=*/        nullptr,
			/*fDeployAnimLength=*/ 0.0f);
		m_lastSpawnedBot[mid] = Bot;

		// Entry is consumed either way — a persistently failing spawn (class
		// not loaded) must not wedge the queue into an infinite retry.
		RemoveQueueEntry(BotFactory, qIdx);

		if (Bot == nullptr || Bot->Controller == nullptr) {
			Logger::Log("tgbotfactory",
				"  SpawnBotById returned null pawn/controller for bot=%d (factory %d)\n",
				botId, mid);
			continue;
		}

		ATgAIController* AIController = (ATgAIController*)Bot->Controller;

		// The intact BotDied reads this INDEX to decrement the right group
		// and to build the replacement entry.
		AIController->m_nFactorySpawnGroup = groupIdx;

		// Juggernaut escort wiring (Raid_DomeCityDefense_P, table 149). Drones
		// spawn ahead of the juggernaut (last group); accumulate them per
		// factory, then adopt every still-valid drone as the juggernaut's
		// owner-driven follower when it spawns. Pointers are batch-scoped
		// (ResetQueue clears the list) and re-validated by r_nPawnId on use.
		if (tableId == kJuggernautSpawnTable) {
			std::vector<EscortDroneRef>& pending = m_escortDrones[mid];
			if (ObjectClassCache::ClassNameContains(Bot, "TgPawn_Juggernaut")) {
				int wired = 0;
				for (size_t d = 0; d < pending.size(); d++) {
					const EscortDroneRef& r = pending[d];
					if (!EscortDroneValid(r)) continue;
					r.pawn->r_Owner = (AActor*)Bot;
					if (r.pawn->Controller != nullptr) {
						((ATgAIController*)r.pawn->Controller)->m_pOwner = Bot;
					}
					wired++;
				}
				pending.clear();
				Logger::Log("tgbotfactory",
					"  juggernaut pawnId=%d spawned (factory=%d) — adopted %d repair drones\n",
					Bot->r_nPawnId, mid, wired);
			} else if (ObjectClassCache::ClassNameContains(Bot, "TgPawn_HoverShieldSphere")) {
				EscortDroneRef ref = { Bot->r_nPawnId, Bot };
				pending.push_back(ref);
				Logger::Log("tgbotfactory",
					"  tracked repair drone pawnId=%d (factory=%d, pending=%d)\n",
					Bot->r_nPawnId, mid, (int)pending.size());
			}
		}

		// Alarm responders: bSpawnOnAlarm factories never auto-spawn, so any
		// spawn here came from ActivateAlarm — mark for the IS_ALARM_BOT
		// behavior tests (stand-down actions). SuperAgent Unleash waves on the
		// same factory are the assault itself, not responders — no mark (the
		// mark makes them eligible for behavior despawn, e.g. DespawnAlarmBots).
		if (BotFactory->bSpawnOnAlarm) {
			if (SuperAgent::Adds::SuppressAlarmMark(BotFactory)) {
				Logger::Log("tgbotfactory",
					"  Unleash wave spawn — m_bAlarmBot NOT set (bot=%d factory=%d)\n",
					botId, mid);
			} else {
				AIController->m_bAlarmBot = 1;
				Logger::Log("alarm",
					"  marked spawned bot=%d (factory=%d) as alarm responder\n", botId, mid);
			}
		}

		if (BotFactory->bAlwaysPatrol) {
			for (int j = 0; j < BotFactory->PatrolPath.Num(); j++) {
				AIController->m_PatrolPath.Add(BotFactory->PatrolPath.Data[j]);
			}
			if (AIController->m_PatrolPath.Num() == 0) {
				AIController->m_PatrolPath.Add(BotStart);
			}
			AIController->m_bPatrolLoop = BotFactory->bPatrolLoop;
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
		BotFactory->m_nLastGroup   = groupNumber;
		BotFactory->m_bFirstSpawn  = 0;
		if (gd != nullptr) gd->nCurrentCount += 1;

		// Pet-spawner factories: notify the owner pawn. Only AttackTransport
		// has an intact OnPetSpawned body (0x109d6af0 — rappel choreography);
		// the incubator overrides are stubs.
		if (isPetFactory && BotFactory->Owner &&
		    ObjectClassCache::ClassNameContains(BotFactory->Owner, "AttackTransport")) {
			ATgPawn* spawner = (ATgPawn*)BotFactory->Owner;
			using OnPetSpawnedFn = void(__fastcall*)(ATgPawn*, void*, ATgPawn*);
			((OnPetSpawnedFn)0x109d6af0)(spawner, nullptr, Bot);
		}

		Logger::Log("tgbotfactory",
			"  spawned bot=%d at loc[%d] (%s) factory=%d group=%d(idx %d) "
			"alive=%d/%d  remaining=%d  totalSpawns=%d\n",
			botId, lIdx, jittered ? "jittered" : "anchored", mid,
			groupNumber, groupIdx,
			BotFactory->nCurrentCount, BotFactory->nActiveCount,
			BotFactory->m_SpawnQueue.Num(), BotFactory->nTotalSpawns);

		spawnedThisCall++;

		// Pets: exactly one per paced SpawnPets call. Single mode: one per
		// timer tick. Bulk mode: keep draining due entries.
		if (isPetFactory) return;
		if (!BotFactory->bBulkSpawn) break;
	}

	// Self-schedule while work remains (replacements or paced roster).
	if (!isPetFactory && BotFactory->m_SpawnQueue.Num() > 0) {
		float delay = BotFactory->fSpawnDelay;
		if (delay < kMinSpawnDelay) delay = kMinSpawnDelay;
		Actor__SetTimer::SetTimer(
			(AActor*)BotFactory, delay, /*bLoop=*/ false,
			FName("SpawnNextBot"), nullptr);
	} else if (spawnedThisCall > 0 && BotFactory->m_SpawnQueue.Num() == 0) {
		Logger::Log("tgbotfactory",
			"  factory %d roster fully spawned (alive=%d)\n",
			mid, BotFactory->nCurrentCount);
	}
}
