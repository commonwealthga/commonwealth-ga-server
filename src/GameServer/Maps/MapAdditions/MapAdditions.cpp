#include "src/GameServer/Maps/MapAdditions/MapAdditions.hpp"

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>

namespace {
constexpr const char* CH = "mapadditions";

ATgGame* s_AppliedGame = nullptr;  // once per game instance

struct PendingBotFactory {
	MapAdditions::BotFactorySpec Spec;
	std::vector<ANavigationPoint*> SpawnNodes;
	std::vector<ANavigationPoint*> PatrolNodes;
};
// Keyed by synthetic map object id; consumed by ApplyPendingBotFactory.
std::map<int, PendingBotFactory> s_PendingBotFactories;

struct BossAlarmState {
	MapAdditions::BossAlarmSpec Spec;
	ATgMissionObjective_Bot* Objective = nullptr;  // resolved lazily, cached
	float TargetSince   = -1.0f;
	float NoTargetSince = -1.0f;
	float LastFire      = -1.0f;
	bool  bAddsOut      = false;
	bool  bUnresolvable = false;  // auto-pick found several bot objectives
};
std::vector<BossAlarmState> s_BossAlarms;
float s_LastTick = -1.0f;

// Clears bStatic/bNoDelete on a class default object so a dynamic Spawn
// succeeds; restores on scope exit.
struct CdoSpawnable {
	AActor* Cdo;
	unsigned long SavedStatic = 0, SavedNoDelete = 0;
	explicit CdoSpawnable(const char* cdoName)
		: Cdo((AActor*)ClassPreloader::GetObject(cdoName)) {
		if (!Cdo) return;
		SavedStatic   = Cdo->bStatic;
		SavedNoDelete = Cdo->bNoDelete;
		Cdo->bStatic   = 0;
		Cdo->bNoDelete = 0;
	}
	~CdoSpawnable() {
		if (!Cdo) return;
		Cdo->bStatic   = SavedStatic;
		Cdo->bNoDelete = SavedNoDelete;
	}
};

std::vector<ANavigationPoint*> SpawnNavSpots(ATgGame* Game,
		const std::vector<MapAdditions::NavSpot>& spots) {
	std::vector<ANavigationPoint*> out;
	UClass* cls = ClassPreloader::GetClass("Class Engine.PathNode");
	if (!cls) { Logger::Log(CH, "no PathNode class\n"); return out; }
	CdoSpawnable cdo("PathNode Engine.Default__PathNode");
	for (const auto& s : spots) {
		FRotator rot; rot.Pitch = 0; rot.Yaw = s.Yaw; rot.Roll = 0;
		ANavigationPoint* nav = (ANavigationPoint*)Game->Spawn(
			cls, (AActor*)Game, FName(), s.Location, rot, nullptr, 1);
		if (!nav) {
			Logger::Log(CH, "  PathNode spawn FAILED at (%.0f,%.0f,%.0f)\n",
				s.Location.X, s.Location.Y, s.Location.Z);
			continue;
		}
		out.push_back(nav);
	}
	return out;
}

void AddBotFactory(ATgGame* Game, const MapAdditions::BotFactorySpec& spec) {
	if (spec.nMapObjectId == 0 || spec.SpawnPoints.empty()) {
		Logger::Log(CH, "AddBotFactory: mid=%d needs a map object id and spawn points\n",
			spec.nMapObjectId);
		return;
	}
	UClass* cls = ClassPreloader::GetClass("Class TgGame.TgBotFactory");
	if (!cls) { Logger::Log(CH, "no TgBotFactory class\n"); return; }

	PendingBotFactory& p = s_PendingBotFactories[spec.nMapObjectId];
	p.Spec        = spec;
	p.SpawnNodes  = SpawnNavSpots(Game, spec.SpawnPoints);
	p.PatrolNodes = SpawnNavSpots(Game, spec.PatrolPoints);

	// m_nMapObjectId goes through the CDO so PreBeginPlay's LoadObjectConfig
	// (fired inside Spawn) already sees the synthetic id.
	CdoSpawnable cdo("TgBotFactory TgGame.Default__TgBotFactory");
	ATgActorFactory* fcdo = (ATgActorFactory*)cdo.Cdo;
	const int savedMid = fcdo ? fcdo->m_nMapObjectId : 0;
	if (fcdo) fcdo->m_nMapObjectId = spec.nMapObjectId;

	const MapAdditions::NavSpot& first = spec.SpawnPoints.front();
	FRotator rot; rot.Pitch = 0; rot.Yaw = first.Yaw; rot.Roll = 0;
	ATgBotFactory* f = (ATgBotFactory*)Game->Spawn(
		cls, (AActor*)Game, FName(), first.Location, rot, nullptr, 1);

	if (fcdo) fcdo->m_nMapObjectId = savedMid;

	Logger::Log(CH, "AddBotFactory mid=%d table=%d alarm=%d prio=%d nodes=%zu/%zu -> 0x%p\n",
		spec.nMapObjectId, spec.nSpawnTableId, (int)spec.bSpawnOnAlarm, spec.nPriority,
		p.SpawnNodes.size(), p.PatrolNodes.size(), f);
	s_PendingBotFactories.erase(spec.nMapObjectId);
}

void AddBossAlarm(const MapAdditions::BossAlarmSpec& spec) {
	BossAlarmState st;
	st.Spec = spec;
	s_BossAlarms.push_back(st);
	Logger::Log(CH, "AddBossAlarm objective=%d table=%d factories=%zu minPlayers=%d\n",
		spec.nObjectiveMapObjectId, spec.nSpawnTableId, spec.FactoryIds.size(), spec.nMinPlayers);
}

ATgBotFactory* FindBotFactory(ATgGame* Game, int mid) {
	for (int i = 0; i < Game->s_ActorFactories.Count; i++) {
		ATgActorFactory* af = Game->s_ActorFactories.Data[i];
		if (!af || af->m_nMapObjectId != mid) continue;
		if (!ObjectClassCache::ClassNameContains(af, "TgBotFactory")) continue;
		return (ATgBotFactory*)af;
	}
	return nullptr;
}

int CountPlayers(ATgGame* Game) {
	int n = 0;
	for (AController* C = Game->WorldInfo->ControllerList; C; C = C->NextController) {
		if (ObjectClassCache::ClassNameContains(C, "PlayerController")) n++;
	}
	return n;
}

ATgPawn* ResolveBoss(BossAlarmState& a) {
	if (!a.Objective) {
		if (a.bUnresolvable) return nullptr;
		// Id 0 = auto-pick: the map's only bot objective.
		const int wanted = a.Spec.nObjectiveMapObjectId;
		int matches = 0;
		ActorCache::CacheMapActors();
		for (ATgMissionObjective* obj : ActorCache::MissionObjectives) {
			if (!obj || (wanted != 0 && obj->m_nMapObjectId != wanted)) continue;
			if (!ObjectClassCache::ClassNameContains(obj, "TgMissionObjective_Bot")) continue;
			if (++matches == 1) a.Objective = (ATgMissionObjective_Bot*)obj;
			if (wanted != 0) break;
		}
		if (wanted == 0 && matches > 1) {
			Logger::Log(CH, "BossAlarm: %d bot objectives on this map — set nObjectiveMapObjectId; alarm disabled\n",
				matches);
			a.Objective = nullptr;
			a.bUnresolvable = true;
		}
		if (!a.Objective) return nullptr;
		Logger::Log(CH, "BossAlarm: boss objective resolved mid=%d\n", a.Objective->m_nMapObjectId);
	}
	ATgPawn* boss = a.Objective->r_ObjectiveBot;
	return (boss && boss->Health > 0) ? boss : nullptr;
}

// Same drive as ActivateAlarm on the responder, minus the closest-factory pick:
// bulk-spawn the whole roster now, then aim it at the boss's target.
void FireFactory(ATgBotFactory* f, int tableId, ATgPawn* target) {
	const unsigned long savedAuto   = f->bAutoSpawn;
	const unsigned long savedBulk   = f->bBulkSpawn;
	const int           savedActive = f->nActiveCount;
	f->bAutoSpawn   = 1;
	f->bBulkSpawn   = 1;
	f->nActiveCount = 0;
	TgBotFactory__ResetQueue::Call(f, nullptr, tableId);
	TgBotFactory__SpawnNextBot::Call(f, nullptr);
	f->nActiveCount = savedActive;
	f->bBulkSpawn   = savedBulk;
	f->bAutoSpawn   = savedAuto;
	f->eventSetTarget(target, 1);
}

void TickBossAlarm(ATgGame* Game, BossAlarmState& a, float now) {
	const MapAdditions::BossAlarmSpec& s = a.Spec;

	ATgPawn* boss = ResolveBoss(a);
	if (!boss) { a.TargetSince = -1.0f; return; }

	ATgPawn* target = boss->r_Target;
	if (!target || target->Health <= 0) {
		a.TargetSince = -1.0f;
		if (!a.bAddsOut) return;
		if (a.NoTargetSince < 0.0f) a.NoTargetSince = now;
		if (now - a.NoTargetSince < s.fWipeDespawnDelay) return;
		for (int mid : s.FactoryIds) {
			if (ATgBotFactory* f = FindBotFactory(Game, mid)) f->eventKillBots(1);
		}
		a.bAddsOut = false;
		a.NoTargetSince = -1.0f;
		Logger::Log(CH, "BossAlarm objective=%d: boss targetless %.0fs — adds despawned\n",
			s.nObjectiveMapObjectId, s.fWipeDespawnDelay);
		return;
	}
	a.NoTargetSince = -1.0f;
	if (a.TargetSince < 0.0f) a.TargetSince = now;

	if (now - a.TargetSince < s.fFirstDelay) return;
	if (a.LastFire >= 0.0f && now - a.LastFire < s.fCooldown) return;
	const int players = CountPlayers(Game);
	if (players < s.nMinPlayers) return;

	std::vector<ATgBotFactory*> factories;
	for (int mid : s.FactoryIds) {
		if (ATgBotFactory* f = FindBotFactory(Game, mid)) factories.push_back(f);
	}
	if (factories.empty()) {
		Logger::Log(CH, "BossAlarm objective=%d: none of the factories found\n",
			s.nObjectiveMapObjectId);
		a.LastFire = now;
		return;
	}
	ATgBotFactory* f = factories[rand() % factories.size()];
	FireFactory(f, s.nSpawnTableId, target);
	a.LastFire = now;
	a.bAddsOut = true;
	Logger::Log(CH, "BossAlarm objective=%d FIRED table=%d factory=%d players=%d queueLeft=%d totalSpawns=%d\n",
		s.nObjectiveMapObjectId, s.nSpawnTableId, f->m_nMapObjectId, players,
		f->m_SpawnQueue.Num(), f->nTotalSpawns);
}

}  // namespace

void MapAdditions::ApplyPendingBotFactory(ATgBotFactory* f) {
	if (!f) return;
	auto it = s_PendingBotFactories.find(f->m_nMapObjectId);
	if (it == s_PendingBotFactories.end()) return;
	const BotFactorySpec& s = it->second.Spec;

	f->s_nTaskForce      = s.nTaskForce;
	f->s_nTeamNumber     = s.nTeamNumber;
	f->nSpawnTableId     = s.nSpawnTableId;
	f->bSpawnOnAlarm     = s.bSpawnOnAlarm;
	f->nPriority         = s.nPriority;
	f->nGlobalAlarmId    = s.nGlobalAlarmId;
	f->bAutoSpawn        = s.bAutoSpawn;
	f->bRespawn          = s.bRespawn;
	f->bBulkSpawn        = s.bBulkSpawn;
	f->bPatrolLoop       = s.bPatrolLoop;
	f->bAlwaysPatrol     = s.bAlwaysPatrol;
	f->nActiveCount      = s.nActiveCount;
	f->fSpawnDelay       = s.fSpawnDelay;
	f->fRespawnDelay     = s.fRespawnDelay;
	f->LocationSelection = s.LocationSelection;
	f->TypeSelection     = s.TypeSelection;

	for (ANavigationPoint* n : it->second.SpawnNodes)  f->LocationList.Add(n);
	for (ANavigationPoint* n : it->second.PatrolNodes) f->PatrolPath.Add(n);
}

void MapAdditions::Tick() {
	if (s_BossAlarms.empty()) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game || Game != s_AppliedGame || !Game->WorldInfo) return;
	if (Game->bGameEnded) return;

	const float now = Game->WorldInfo->TimeSeconds;
	if (s_LastTick >= 0.0f && now >= s_LastTick && now - s_LastTick < 1.0f) return;
	s_LastTick = now;

	const int difficulty = Config::GetDifficultyValueId();
	for (BossAlarmState& a : s_BossAlarms) {
		const auto& d = a.Spec.Difficulties;
		if (!d.empty() && std::find(d.begin(), d.end(), difficulty) == d.end()) continue;
		TickBossAlarm(Game, a, now);
	}
}

void MapAdditions::Apply(ATgGame* Game) {
	if (!Game || Game == s_AppliedGame) return;
	s_AppliedGame = Game;
	s_BossAlarms.clear();
	s_LastTick = -1.0f;

	// Exact, case-sensitive map name (note 1p_SDColony04_P is lower-case "p").
	const std::string map = Config::GetMapNameChar();

	// Alarm responder: bSpawnOnAlarm + nPriority -1. ActivateAlarm drives the
	// closest eligible factory to the alarm source.
	//
	// if (map == "1P_SDColony01_P") {
	// 	BotFactorySpec f;
	// 	f.nMapObjectId  = 900001;
	// 	f.nSpawnTableId = 100;
	// 	f.bSpawnOnAlarm = true;
	// 	f.nPriority     = -1;
	// 	f.SpawnPoints   = { { FVector(0, 0, 0), 0 }, { FVector(0, 0, 0), 16384 } };
	// 	AddBotFactory(Game, f);
	// }
	//
	// Boss alarm (difficulty ids: 1259 Max, 1471 Ultra-Max, 5000 Hardcore, 10000 Super Agent):
	//
	// 	BossAlarmSpec b;
	// 	b.nSpawnTableId         = 59;   // nObjectiveMapObjectId left 0 = auto-pick
	// 	b.FactoryIds            = { 900005 };
	// 	b.Difficulties          = { 1471, 5000 };
	// 	b.nMinPlayers           = 2;
	// 	AddBossAlarm(b);

	auto AddAlarmFactory = [Game](int mid, int table, std::vector<NavSpot> spots) {
		BotFactorySpec f;
		f.nMapObjectId  = mid;
		f.nSpawnTableId = table;
		f.bSpawnOnAlarm = true;
		f.nPriority     = -1;
		f.SpawnPoints   = std::move(spots);
		AddBotFactory(Game, f);
	};

	if (map == "1P_SDColony01_P") {
		AddAlarmFactory(900001, 85, {
			{ FVector(2812, -78, -31), 0 },
			{ FVector(7715, -933, -29), 0 } });
		AddAlarmFactory(900002, 85, {
			{ FVector(3328, -6790, -974), 0 },
			{ FVector(5917, -6423, -974), 0 } });
		AddAlarmFactory(900003, 85, {
			{ FVector(8409, -10224, -2510), 0 },
			{ FVector(5914, -9273, -2514), 0 },
			{ FVector(4374, -9230, -2514), 0 } });
		AddAlarmFactory(900004, 85, {
			{ FVector(-1454, -6615, -3026), 0 },
			{ FVector(-2625, -6644, -3027), 0 } });
		AddAlarmFactory(900005, 85, {
			{ FVector(-1522, -11373, -3070), 0 },
			{ FVector(-2730, -11395, -3070), 0 } });

		BossAlarmSpec b;
		b.nSpawnTableId         = 153;   // nObjectiveMapObjectId left 0 = auto-pick
		b.FactoryIds            = { 900005 };
		b.Difficulties          = { 1471, 5000 };
		b.nMinPlayers           = 2;
		AddBossAlarm(b);
	}

	if (map == "1P_SDColony03_P") {
		AddAlarmFactory(900001, 85, {
			{ FVector(12162, -2293, -2498), 0 },
			{ FVector(13319, 1091, -2982), 0 } });
		AddAlarmFactory(900002, 85, {
			{ FVector(16043, -1414, -4321), 0 },
			{ FVector(16328, 714, -4330), 0 } });
		AddAlarmFactory(900003, 85, {
			{ FVector(20173, -1194, -4335), 0 },
			{ FVector(20198, 168, -4335), 0 } });
		AddAlarmFactory(900004, 85, {
			{ FVector(26244, -2398, -4335), 0 },
			{ FVector(26326, 841, -4335), 0 } });
		AddAlarmFactory(900005, 85, {
			{ FVector(29499, -2176, -4270), 0 } });
		AddAlarmFactory(900006, 85, {
			{ FVector(30619, -474, -663), 0 },
			{ FVector(34927, 517, -662), 0 } });
		AddAlarmFactory(900007, 85, {
			{ FVector(33676, 418, 296), 0 } });
		AddAlarmFactory(900008, 85, {
			{ FVector(33754, 1028, 1240), 0 } });
		AddAlarmFactory(900009, 85, {
			{ FVector(32475, -2732, 1534), 0 },
			{ FVector(33459, -2538, 1534), 0 } });

		BossAlarmSpec b;
		b.nSpawnTableId         = 153;   // nObjectiveMapObjectId left 0 = auto-pick
		b.FactoryIds            = { 900009 };
		b.Difficulties          = { 1471, 5000 };
		b.nMinPlayers           = 2;
		AddBossAlarm(b);
	}

	Logger::Log(CH, "Apply: map=%s bossAlarms=%zu\n", map.c_str(), s_BossAlarms.size());
}
