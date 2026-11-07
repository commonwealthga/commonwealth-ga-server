#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/TgPawn/KillCredit/KillCredit.hpp"
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
		if (f->LocationList.Data[idx] == nullptr) continue;
		// SuperAgent escape waves pin the factory to their selected points.
		if (!SuperAgent::Adds::LocationAllowed(f, idx)) continue;
		return idx;
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

// ---- 0985/0986 open-zone respawns ------------------------------------
// Open zones (gameplay type 1554: Sonoran Desert, North Sonora) only.
// Missions keep their scripted spawns untouched.
//  0985 floor: a replacement for a killed bot is never due before its spawn
//       group's own respawn time from the game data
//       (spawn_group_respawn_sec -> FSpawnGroupDetail.nRespawnSeconds).
//  0986 hold: a due replacement waits while any living player is within
//       kClearRadius of one of the factory's spawn points, or has a clear
//       line of sight to one within kSightRadius; re-checked every
//       kRecheckSeconds until clear, so enemies never pop in on top of or in
//       view of a player.
// Not held: friendly NPC factories (task force 2: guards, traders) and
// scripted event attackers (map_object_config no_kill_reward, the North
// Sonora gate fight). Per-factory override: map_object_config
// respawn_clear_radius (uu, 0 = no hold).
// Radius: 4000 uu = the bot_sight_radius we already give the Dome guards
// (how far a zone bot picks a target), so a player who could be fighting
// at a camp keeps it empty; the 8000 uu sight check covers the open
// desert, where a camp is visible from far away.
constexpr float kClearRadius   = 4000.0f;
constexpr float kSightRadius   = 8000.0f;
constexpr float kRecheckSeconds = 5.0f;

std::map<int, float> g_heldSince;   // factory -> TimeSeconds of the first hold

int ClearRadiusFor(ATgBotFactory* f) {
	if (!KillCredit::InOpenZone() || !f->bRespawn) return 0;
	if (f->s_nTaskForce == 2) return 0;
	const int mid = f->m_nMapObjectId;
	const int def = MapObjectConfig::GetInt(mid, "no_kill_reward", 0) ? 0 : (int)kClearRadius;
	return MapObjectConfig::GetInt(mid, "respawn_clear_radius", def);
}

bool ClearLine(AActor* from, const FVector& start, const FVector& end) {
	FVector hl, hn; FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
	return from->Trace(end, start, 0, FVector(0, 0, 0), 0, &hl, &hn, &hi) == nullptr;
}

// True (and *why filled) when a living player keeps the factory's spawn
// points busy.
bool PlayerKeepsSpawnBusy(ATgBotFactory* f, float radius, char* why, size_t whyLen) {
	AWorldInfo* wi = f->WorldInfo;
	if (wi == nullptr || f->LocationList.Data == nullptr) return false;
	const float r2 = radius * radius;
	const float s  = radius > kSightRadius ? radius : kSightRadius;
	const float s2 = s * s;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) continue;
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe || p->Health <= 0) continue;
		FVector eye = p->Location; eye.Z += p->BaseEyeHeight;
		for (int i = 0; i < f->LocationList.Num(); i++) {
			ANavigationPoint* n = f->LocationList.Data[i];
			if (n == nullptr) continue;
			const float dx = p->Location.X - n->Location.X;
			const float dy = p->Location.Y - n->Location.Y;
			const float dz = p->Location.Z - n->Location.Z;
			const float d2 = dx * dx + dy * dy + dz * dz;
			if (d2 < r2) {
				std::snprintf(why, whyLen, "player %.0f uu from loc[%d]", std::sqrt(d2), i);
				return true;
			}
			if (d2 < s2) {
				FVector tgt = n->Location; tgt.Z += 60.0f;
				if (ClearLine((AActor*)p, eye, tgt)) {
					std::snprintf(why, whyLen, "player sees loc[%d] at %.0f uu", i, std::sqrt(d2));
					return true;
				}
			}
		}
	}
	return false;
}

}  // namespace

void TgBotFactory__SpawnNextBot::TagReplacements(ATgBotFactory* f, int firstNew) {
	if (f == nullptr || ClearRadiusFor(f) <= 0) return;
	const float now = f->WorldInfo ? f->WorldInfo->TimeSeconds : 0.0f;
	for (int i = firstNew < 0 ? 0 : firstNew; i < f->m_SpawnQueue.Num(); i++) {
		FSpawnQueueEntry& e = f->m_SpawnQueue.Data[i];
		e.bRespawn = 1;   // ours: only BotDied replacements are held
		const int g = e.nGroupNumber;
		const int dataSec = (g >= 0 && g < f->m_SpawnGroups.Num())
			? f->m_SpawnGroups.Data[g].nRespawnSeconds : 0;
		const float native = e.fSpawnTime - now;
		if (e.fSpawnTime < now + (float)dataSec) e.fSpawnTime = now + (float)dataSec;
		const ANavigationPoint* l0 = (f->LocationList.Num() > 0 && f->LocationList.Data)
			? f->LocationList.Data[0] : nullptr;
		const APawn* p0 = nullptr;   // first living player, for the log only
		if (f->WorldInfo) {
			for (AController* c = f->WorldInfo->ControllerList; c != nullptr; c = c->NextController) {
				if (c->Pawn == nullptr || c->Pawn->Health <= 0) continue;
				if (!ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) continue;
				p0 = c->Pawn; break;
			}
		}
		Logger::Log("tgbotfactory",
			"  [Respawn] factory %d replacement group idx %d due in %.1fs "
			"(native %.1fs, data %ds) loc0=(%.0f,%.0f,%.0f) p0=(%.0f,%.0f,%.0f)\n",
			f->m_nMapObjectId, g, e.fSpawnTime - now, native, dataSec,
			l0 ? l0->Location.X : 0.0f, l0 ? l0->Location.Y : 0.0f, l0 ? l0->Location.Z : 0.0f,
			p0 ? p0->Location.X : 0.0f, p0 ? p0->Location.Y : 0.0f, p0 ? p0->Location.Z : 0.0f);
	}
}

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

		// 0986: a due replacement waits while a player is near / in sight.
		if (entry.bRespawn) {
			const int radius = ClearRadiusFor(BotFactory);
			char why[96] = "";
			if (radius > 0 && PlayerKeepsSpawnBusy(BotFactory, (float)radius, why, sizeof(why))) {
				BotFactory->m_SpawnQueue.Data[qIdx].fSpawnTime = now + kRecheckSeconds;
				auto it = g_heldSince.find(mid);
				if (it == g_heldSince.end()) {
					g_heldSince[mid] = now;
					Logger::Log("tgbotfactory",
						"  [Respawn] factory %d replacement HELD: %s (radius %d, recheck %.0fs)\n",
						mid, why, radius, kRecheckSeconds);
				}
				continue;
			}
			auto it = g_heldSince.find(mid);
			if (it != g_heldSince.end()) {
				Logger::Log("tgbotfactory",
					"  [Respawn] factory %d clear after %.0fs held - respawning\n",
					mid, now - it->second);
				g_heldSince.erase(it);
			}
		}

		const int groupIdx    = entry.nGroupNumber;
		const int tableId     = entry.nSpawnTableId > 0 ? entry.nSpawnTableId
		                                                : BotFactory->nSpawnTableId;
		int groupNumber =
			TgBotFactory__LoadObjectConfig::GetFactoryGroupValue(BotFactory, groupIdx);
		if (groupNumber < 0) {
			groupNumber =
				TgBotFactory__LoadObjectConfig::GetGroupNumberByIndex(tableId, groupIdx);
		}

		// Vanilla: the group's bot type was rolled ONCE at ResetQueue — every
		// entry (and BotDied replacements) spawns that same bot. 0 = roster
		// group (gmin/gmax, incubators) → fresh per-entry roll.
		// skal: replace botid by {botid,bbm}
		BotIdEntry Entry = TgBotFactory__LoadObjectConfig::GetFactoryGroupRoll(BotFactory, groupIdx);
		if ((Entry.BotId <= 0)&&(groupNumber >= 0)) {
			Entry = TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(tableId, groupNumber);
		}
		if (Entry.BotId <= 0) {
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
			TgGame__SpawnBotById::GetBotCollisionCylinder(Entry.BotId, &radius, &halfHeight);
			if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
		}
		const bool jittered = !isPetFactory && gd != nullptr && gd->nCurrentCount > 0;
		if (jittered) ApplyXYJitter(loc);

		// Facing: nav point rotation, with optional per-factory yaw override.
		FRotator spawnRot = BotStart->Rotation;
		if (MapObjectConfig::Has(mid, "spawn_rotation_yaw")) {
			spawnRot.Yaw = MapObjectConfig::GetInt(mid, "spawn_rotation_yaw", spawnRot.Yaw);
		}

		// skal: per spawn-table bot balance modifier
		if (Entry.BBM>0.0f) {
			TgPawn__InitializeDefaultProps::fPendingSpawnTableBalance = Entry.BBM;
			TgPawn__InitializeDefaultProps::bPendingEnemyScaling = true;
		}
		//Logger::Log("skal", "[SpawnNextBot]: bbm='%f'\n",Entry.BBM);

		ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
		ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
			Entry.BotId, loc, spawnRot,
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
				Entry.BotId, mid);
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
					Entry.BotId, mid);
			} else {
				AIController->m_bAlarmBot = 1;
				Logger::Log("alarm",
					"  marked spawned bot=%d (factory=%d) as alarm responder\n", Entry.BotId, mid);
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
				// r_ObjectiveBot is null until StampObjective runs (and after the
				// objective bot dies) — guard the Location deref.
				if (objclass == "Class TgGame.TgMissionObjective_Bot"
						&& ((ATgMissionObjective_Bot*)Obj)->r_ObjectiveBot != nullptr) {
					ATgMissionObjective_Bot* BotObj = (ATgMissionObjective_Bot*)Obj;
					AIController->m_pTriggerTarget = BotObj->r_ObjectiveBot;
					AIController->m_pTriggerDestination = BotObj->r_ObjectiveBot;
					AIController->m_vTriggerLocation = BotObj->r_ObjectiveBot->Location;
					AIController->m_bInterrupt = 1;
				}
			} else if (BotFactory->s_nTaskForce == 2 && GTeamsData.Defenders->r_CurrActiveObjective != nullptr) {
				ATgMissionObjective* Obj = (ATgMissionObjective*)GTeamsData.Defenders->r_CurrActiveObjective;
				std::string objclass = Obj->Class->GetFullName();
				if (objclass == "Class TgGame.TgMissionObjective_Bot"
						&& ((ATgMissionObjective_Bot*)Obj)->r_ObjectiveBot != nullptr) {
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
			Entry.BotId, lIdx, jittered ? "jittered" : "anchored", mid,
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
