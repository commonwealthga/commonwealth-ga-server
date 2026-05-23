#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <climits>
#include <cstdlib>
#include <ctime>
#include <vector>

namespace {

// Spawn the bot at ObjectiveBot.Location with the cylinder-Z lift that
// SpawnNextBot uses for factory spawns (APawn.Location is at cylinder centre;
// editor markers are at ground level, so without the lift the mesh's lower
// half ends up below terrain).
ATgPawn* SpawnBotAtLocation(ATgGame* Game, int botId, FVector loc, FRotator rot) {
	float radius = 0.f, halfHeight = 0.f;
	TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
	if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
	return (ATgPawn*)Game->SpawnBotById(
		botId, loc, rot,
		/*bKillController=*/   false,
		/*pFactory=*/          nullptr,
		/*bIgnoreCollision=*/  true,
		/*pOwnerPawn=*/        nullptr,
		/*bIsDecoy=*/          false,
		/*deviceFire=*/        nullptr,
		/*fDeployAnimLength=*/ 0.0f);
}

// Pick one bot from a spawn table at the current difficulty, using the same
// weighted-by-spawn_chance scheme as LoadObjectConfig builds for factories.
// Only entries in the lowest spawn_group are considered — boss tables (e.g.
// table 36) have one group; multi-group tables would otherwise risk picking
// a non-boss wave-member row as the objective.
// Returns 0 if the table has no entries at this difficulty.
int PickBotFromSpawnTable(int spawnTableId) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT enemy_bot_id, spawn_chance, spawn_group "
		"FROM asm_data_set_bot_spawn_tables "
		"WHERE bot_spawn_table_id = ? AND difficulty_value_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) return 0;
	sqlite3_bind_int(stmt, 1, spawnTableId);
	sqlite3_bind_int(stmt, 2, Config::GetDifficultyValueId());

	struct Entry { int bot; float chance; int group; };
	std::vector<Entry> all;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		all.push_back({
			sqlite3_column_int(stmt, 0),
			static_cast<float>(sqlite3_column_double(stmt, 1)),
			sqlite3_column_int(stmt, 2),
		});
	}
	sqlite3_finalize(stmt);
	if (all.empty()) return 0;

	int minGroup = INT_MAX;
	for (const auto& e : all) if (e.group < minGroup) minGroup = e.group;

	std::vector<int> weighted;
	for (const auto& e : all) {
		if (e.group != minGroup) continue;
		int reps = static_cast<int>(e.chance * 100);
		for (int i = 0; i < reps; ++i) weighted.push_back(e.bot);
	}
	if (weighted.empty()) return 0;
	srand(static_cast<unsigned>(time(nullptr)));
	return weighted[rand() % weighted.size()];
}

// Mirror the team-assignment logic SpawnNextBot applies after each factory
// spawn (TgBotFactory__SpawnNextBot.cpp). Used only by the tier-2 / tier-3
// fallback paths in SpawnObjectiveBot — the factory tier already runs this
// inside SpawnNextBot via BotFactory->s_nTaskForce. tf==1 → Attackers,
// anything else → Defenders (typical PvE: bots defend, players attack).
void ApplyTaskForceToBot(ATgPawn* Bot, int taskForce) {
	if (!Bot) return;
	ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	if (!BotRep) return;
	int team = (taskForce == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
	BotRep->r_TaskForce = team;
	BotRep->Team        = team;
	BotRep->SetTeam(team);
	Bot->NotifyTeamChanged();
}

void StampObjectiveAndReplicate(ATgMissionObjective_Bot* ObjectiveBot, ATgPawn* Bot) {
	if (!Bot) return;
	ObjectiveBot->Role               = 3;
	// ObjectiveBot->RemoteRole         = 1;
	ObjectiveBot->r_ObjectiveBot     = Bot;
	ObjectiveBot->r_ObjectiveBotInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	ObjectiveBot->r_bIsActive        = 1;
	ObjectiveBot->r_eStatus          = 3;
	// ObjectiveBot->bNetDirty          = 1;
	// ObjectiveBot->bNetInitial        = 1;
	// ObjectiveBot->bForceNetUpdate    = 1;
	// ObjectiveBot->bSkipActorPropertyReplication = 0;

	// Bot->Role            = 3;
	// Bot->RemoteRole      = 1;
	// Bot->bNetDirty       = 1;
	// Bot->bNetInitial     = 1;
	// Bot->bForceNetUpdate = 1;
	// Bot->bSkipActorPropertyReplication = 0;

	// ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	// if (BotRep) {
	// 	BotRep->bNetDirty       = 1;
	// 	BotRep->bNetInitial     = 1;
	// 	BotRep->bForceNetUpdate = 1;
	// 	BotRep->bSkipActorPropertyReplication = 0;
	// }
}

} // namespace

void __fastcall TgMissionObjective_Bot__SpawnObjectiveBot::Call(ATgMissionObjective_Bot *ObjectiveBot, void *edx) {
	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot::Call\n");

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	UWorld* World = (UWorld*)Globals::Get().GWorld;
	AWorldInfo* WorldInfo = (AWorldInfo*)World__GetWorldInfo::CallOriginal(World, nullptr, 0);

	const int objId = ObjectiveBot->m_nMapObjectId;
	auto& facMap   = TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToBotFactoryId;
	auto& botMap   = TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToBotId;
	auto& tableMap = TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToSpawnTableId;
	auto& tfMap    = TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToTaskForce;

	auto facIt   = facMap.find(objId);
	auto botIt   = botMap.find(objId);
	auto tableIt = tableMap.find(objId);
	auto tfIt    = tfMap.find(objId);
	int factoryId = (facIt   != facMap.end())   ? facIt->second   : 0;
	int directBot = (botIt   != botMap.end())   ? botIt->second   : 0;
	int tableId   = (tableIt != tableMap.end()) ? tableIt->second : 0;
	// Default to 2 (Defenders) when the column is unset — matches the
	// historical PvE convention for boss objectives. If the user has wired
	// a specific tf, that wins.
	int taskForce = (tfIt    != tfMap.end())    ? tfIt->second    : 2;

	ATgPawn* Bot = nullptr;

	// Tier 1: factory spawn — existing path, recurses through the factory's
	// SpawnQueue and uses the LAST spawned bot. Kept verbatim so already-
	// working maps (e.g. CPLab05's bossroom after v27) don't change behavior.
	if (factoryId > 0) {
		while (TgBotFactory__LoadObjectConfig::m_loadedBotFactories.find(factoryId)
		       == TgBotFactory__LoadObjectConfig::m_loadedBotFactories.end()) {
			Sleep(100);
		}
		ATgBotFactory* BotFactory = TgBotFactory__LoadObjectConfig::m_loadedBotFactories[factoryId];
		if (BotFactory) {
			for (int i = 0; i < BotFactory->LocationList.Num(); i++) {
				BotFactory->LocationList.Data[i]->Location = ObjectiveBot->Location;
			}
			BotFactory->SpawnNextBot();
			Bot = TgBotFactory__SpawnNextBot::m_lastSpawnedBot[factoryId];
		}
	}
	// Tier 2: spawn a specific bot at the objective marker. For maps whose
	// boss isn't wired through a TgBotFactory (Kismet-spawned in the
	// original game) — sidesteps the factory queue entirely.
	else if (directBot > 0) {
		Bot = SpawnBotAtLocation(Game, directBot, ObjectiveBot->Location, ObjectiveBot->Rotation);
		ApplyTaskForceToBot(Bot, taskForce);
		Logger::Log("debug",
			"[SpawnObjectiveBot] objective=%d via bot_id fallback: bot=%d tf=%d (Bot=%p)\n",
			objId, directBot, taskForce, Bot);
	}
	// Tier 3: weighted pick from a spawn table. For maps where the boss
	// roster varies (e.g. CPLab03 → table 36: Boss Think-Tank/Vanguard).
	else if (tableId > 0) {
		int pickedBotId = PickBotFromSpawnTable(tableId);
		if (pickedBotId > 0) {
			Bot = SpawnBotAtLocation(Game, pickedBotId, ObjectiveBot->Location, ObjectiveBot->Rotation);
			ApplyTaskForceToBot(Bot, taskForce);
		}
		Logger::Log("debug",
			"[SpawnObjectiveBot] objective=%d via spawn_table fallback: table=%d picked bot=%d tf=%d (Bot=%p)\n",
			objId, tableId, pickedBotId, taskForce, Bot);
	}
	else {
		Logger::Log("debug",
			"[SpawnObjectiveBot] objective=%d: no resolution config (factory/bot_id/spawn_table all empty)\n",
			objId);
	}

	if (Bot == nullptr) {
		Logger::Log("debug", "Bot == nullptr\n");
		return;
	}
	Logger::Log("debug", "Bot != nullptr\n");

	StampObjectiveAndReplicate(ObjectiveBot, Bot);

	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot END\n");
}
