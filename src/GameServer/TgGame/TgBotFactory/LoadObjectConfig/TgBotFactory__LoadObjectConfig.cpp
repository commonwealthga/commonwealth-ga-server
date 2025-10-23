#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

std::map<int, int> TgBotFactory__LoadObjectConfig::m_mapObjectIdToSpawnTableId;
std::map<int, int> TgBotFactory__LoadObjectConfig::m_mapObjectIdToTaskForceNumber;
std::map<int, std::map<int, std::vector<SpawnTableEntry>>> TgBotFactory__LoadObjectConfig::m_spawnTables;
bool TgBotFactory__LoadObjectConfig::bConfigLoaded = false;

void __fastcall TgBotFactory__LoadObjectConfig::Call(ATgBotFactory *BotFactory, void *edx) {
	Logger::Log("tgbotfactory", "[%s] %s LoadObjectConfig mapObjectId=%d\n", Logger::GetTime(), BotFactory->GetName(), BotFactory->m_nMapObjectId);

	// BotFactory->bAutoSpawn = 1;
	BotFactory->nPriority = 0;

	if (!bConfigLoaded) {
		sqlite3* db = Database::GetConnection();

		char* err = nullptr;
		int result = 0;

		std::vector<std::map<std::string, std::string>> factories_data;
		result = sqlite3_exec(db, "SELECT * FROM obj_bot_factories", Database::Callback, &factories_data, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to select obj_bot_factories: %s\n", err);
			return;
		}

		for (auto row : factories_data) {
			int mapObjectId = std::stoi(row["map_object_id"]);
			int botSpawnTableId = std::stoi(row["bot_spawn_table_id"]);
			int taskForceNumber = std::stoi(row["task_force_number"]);

			m_mapObjectIdToSpawnTableId[mapObjectId] = botSpawnTableId;
			m_mapObjectIdToTaskForceNumber[mapObjectId] = taskForceNumber;
		}

		std::vector<std::map<std::string, std::string>> spawn_tables_data;
		result = sqlite3_exec(db, " \
			SELECT \
				bot_spawn_table_id, \
				spawn_group, \
				enemy_bot_id, \
				bot_count, \
				spawn_chance, \
				reference_name \
			FROM asm_data_set_bot_spawn_tables \
			LEFT JOIN asm_data_set_bots ON asm_data_set_bot_spawn_tables.enemy_bot_id = asm_data_set_bots.bot_id \
			WHERE difficulty_value_id = 1471; \
			", Database::Callback, &spawn_tables_data, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to select asm_data_set_bot_spawn_tables: %s\n", err);
			return;
		}

		std::map<int, std::map<int, std::vector<SpawnTableEntry>>> spawnTables;

		for (auto row : spawn_tables_data) {
			int botSpawnTableId = std::stoi(row["bot_spawn_table_id"]);
			int spawnGroup = std::stoi(row["spawn_group"]);
			int enemyBotId = std::stoi(row["enemy_bot_id"]);
			int botCount = std::stoi(row["bot_count"]);
			float spawnChance = std::stof(row["spawn_chance"]);
			std::string referenceName = row["reference_name"];

			// SpawnGroups[spawnGroup].push_back(SpawnTableEntry{
			// 	botSpawnTableId,
			// 	spawnGroup,
			// 	enemyBotId,
			// 	botCount,
			// 	spawnChance,
			// 	referenceName
			// });

			spawnTables[botSpawnTableId][spawnGroup].push_back(SpawnTableEntry{
				botSpawnTableId,
				spawnGroup,
				enemyBotId,
				botCount,
				spawnChance,
				referenceName
			});
		}

		for (auto& spawnTable : spawnTables) {
			for (auto& spawnGroup : spawnTable.second) {
				std::vector<SpawnTableEntry> spawnTableWeighted;
				for (auto& spawnEntry : spawnGroup.second) {
					for (int i = 0; i < spawnEntry.SpawnChance * 100; i++) {
						Logger::Log("tgbotfactory", " Adding spawn entry %d to spawn table %d, group %d\n", spawnEntry.EnemyBotId, spawnEntry.SpawnTableId, spawnEntry.SpawnGroup);
						spawnTableWeighted.push_back(spawnEntry);
					}
				}

				srand(time(NULL));

				if (spawnTableWeighted.size() == 0) {
					Logger::Log("tgbotfactory", " Weighted spawn table is empty\n");
					continue;
				}

				SpawnTableEntry randomSpawnEntry = spawnTableWeighted[rand() % spawnTableWeighted.size()];

				Logger::Log("tgbotfactory", " Selected spawn entry %d to spawn table %d\n", randomSpawnEntry.EnemyBotId, randomSpawnEntry.SpawnTableId);

				m_spawnTables[randomSpawnEntry.SpawnTableId][spawnGroup.first].push_back(SpawnTableEntry{
					randomSpawnEntry.SpawnTableId,
					randomSpawnEntry.SpawnGroup,
					randomSpawnEntry.EnemyBotId,
					randomSpawnEntry.BotCount,
					randomSpawnEntry.SpawnChance,
					randomSpawnEntry.ReferenceName
				});
			}
		}

		bConfigLoaded = true;
	}

	if (!m_mapObjectIdToSpawnTableId[BotFactory->m_nMapObjectId]) {
		Logger::Log("tgbotfactory", "No spawn table found for map object id %d\n", BotFactory->m_nMapObjectId);
		return;
	}

	Logger::Log("tgbotfactory", "Loaded spawn table %d for map object id %d\n", m_mapObjectIdToSpawnTableId[BotFactory->m_nMapObjectId], BotFactory->m_nMapObjectId);

	BotFactory->nSpawnTableId = m_mapObjectIdToSpawnTableId[BotFactory->m_nMapObjectId];
	BotFactory->s_nTaskForce = m_mapObjectIdToTaskForceNumber[BotFactory->m_nMapObjectId];

	TARRAY_INIT(BotFactory, m_SpawnQueue, FSpawnQueueEntry, 0x240, 128);
	TARRAY_INIT(BotFactory, m_SpawnGroups, FSpawnGroupDetail, 0x250, 128);

	for (auto& spawnGroup : m_spawnTables[BotFactory->nSpawnTableId]) {
		int spawnGroupNumber = spawnGroup.first;
		for (auto& spawnEntry : spawnGroup.second) {
			FSpawnQueueEntry entry;
			entry.nGroupNumber = spawnGroupNumber;
			entry.nSpawnTableId = BotFactory->nSpawnTableId;
			entry.bRespawn = 0;
			entry.fSpawnTime = 0.0f;

			TARRAY_ADD(m_SpawnQueue, entry);
		}
	}

	// if (
	// 	BotFactory->m_nMapObjectId == 13639
	// 	|| BotFactory->m_nMapObjectId == 13632
	// 	|| BotFactory->m_nMapObjectId == 13629
	// 	|| BotFactory->m_nMapObjectId == 13638
	// 	|| BotFactory->m_nMapObjectId == 13647
	// 	|| BotFactory->m_nMapObjectId == 13630
	// 	|| BotFactory->m_nMapObjectId == 13634
	// 	|| BotFactory->m_nMapObjectId == 13633
	// 	|| BotFactory->m_nMapObjectId == 13637
	// 	|| BotFactory->m_nMapObjectId == 13635
	// 	|| BotFactory->m_nMapObjectId == 13636
	// 	|| BotFactory->m_nMapObjectId == 13640
	// ) {
	// 	BotFactory->bAutoSpawn = 1;
	// }
	//
// insert into obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number) values
//  (12712, 29, 2),
// (12708, 29, 2),
// (12710, 58, 2),
// (12711, 29, 2),
// (12714, 34, 2),
// (12713, 34, 2),
// (12709, 28, 2);
//
// 12712, 29, 2 standard
// 12708, 29, 2 initial T1 
// 12710, 58, 2 standard T3
// 12711, 29, 2 standard
// 12714, 34, 2 support
// 12713, 34, 2 support T2
// 12709, 28, 2 standard T2
//
// 12728 alarm bots
// 12729 alarm bots
// 12730 alarm bots
// 12724 alarm bots
// 12732 alarm bots
// 12733 alarm bots
// 12734 alarm bots
// 12735 alarm bots
// 12744 alarm bots bossroom
// 12738 alarm bots bossroom
// 12736 alarm bots
// 12740 alarm bots bossroom
// 12741 alarm bots bossroom
// 12739 alarm bots bossroom
// 12698 ?
// 12737 alarm bots
// 12731 alarm bots
// 12727 alarm bots


// TgBotFactory   13639 = wasps wave 1
// TgBotFactory_1 13632 = Spider - wave1 - fac11
// TgBotFactory_2 13629 = minion wave 1
// TgBotFactory_3 13638 = ticks wave 1
// TgBotFactory_4 13647 = guardian = bot_id 1461
// TgBotFactory_5 13630 = ticks wave 1
// TgBotFactory_6 13634 = ticks wave 1
// TgBotFactory_7 13633 = minion wave 1
// TgBotFactory_8 13637 = ticks wave 1
// TgBotFactory_9 13635 ?
// TgBotFactory_10 13636 = minion wave 1
// TgBotFactory_11 13640 = ticks wave 1

	// TARRAY_INIT(NetDriver, ClientConnections, UNetConnection*, 0x44, 128); // todo: move this elsewhere for performance



	
	// 147 = wasps
	// 86 = spider
	// 99 = minion
	// 148 = ticks
	// 87 = guardian
	// 102 = unknown
	// 149 = juggernaut
	// 166 = dome guards
	

// TgBotFactory_51 13849 = DOME GUARDS
// TgBotFactory_52 13846 = DOME GUARDS
// TgBotFactory_53 13847 = DOME GUARDS
// TgBotFactory_54 13848 = DOME GUARDS
	
	// 87 = guardian

	
// TgBotFactory   13639 = wasps wave 1
// TgBotFactory_1 13632 = Spider - wave1 - fac11
// TgBotFactory_2 13629 = minion wave 1
// TgBotFactory_3 13638 = ticks wave 1
// TgBotFactory_4 13647 = guardian = bot_id 1461
// TgBotFactory_5 13630 = ticks wave 1
// TgBotFactory_6 13634 = ticks wave 1
// TgBotFactory_7 13633 = minion wave 1
// TgBotFactory_8 13637 = ticks wave 1
// TgBotFactory_9 13635 ?
// TgBotFactory_10 13636 = minion wave 1
// TgBotFactory_11 13640 = ticks wave 1
// TgBotFactory_12 13642 = spider
// TgBotFactory_13 13641 = minions
// TgBotFactory_14 13810 ?
// TgBotFactory_15 13645 = minions
// TgBotFactory_16 13644 = spider
// TgBotFactory_17 13646 = minions
// TgBotFactory_18 13664 = minions
// TgBotFactory_19 13648 = minions
// TgBotFactory_20 13650 = ticks
// TgBotFactory_21 13704 = spider
// TgBotFactory_22 13652 = ticks
// TgBotFactory_23 13651 = minion
// TgBotFactory_24 13694 ?
// TgBotFactory_25 13659 ?
// TgBotFactory_26 13661 = spider
// TgBotFactory_27 13655 = minion
// TgBotFactory_28 13656 = minion
// TgBotFactory_29 13657 = guardian
// TgBotFactory_30 13660 = minion
// TgBotFactory_31 13809 = dummy bot factory
// TgBotFactory_32 13654 = minion
// TgBotFactory_33 13692 = wasps
// TgBotFactory_34 13708 = spider
// TgBotFactory_35 13643 = ticks wave 1
// TgBotFactory_36 13709 ?
// TgBotFactory_37 13673 = juggernaut
// TgBotFactory_38 13805 ?
// TgBotFactory_39 13691 = guardian
// TgBotFactory_40 13658 = guardian
// TgBotFactory_41 13703 = juggernaut
// TgBotFactory_42 13662 = juggernaut
// TgBotFactory_43 13649 = minion
// TgBotFactory_44 13665 ?
// TgBotFactory_45 13802 ?
// TgBotFactory_46 13804 ?
// TgBotFactory_47 13803 ?
// TgBotFactory_48 13700 ?
// TgBotFactory_49 13806 ?
// TgBotFactory_50 13653 = juggernaut
// TgBotFactory_51 13849 = DOME GUARDS
// TgBotFactory_52 13846 = DOME GUARDS
// TgBotFactory_53 13847 = DOME GUARDS
// TgBotFactory_54 13848 = DOME GUARDS
}

