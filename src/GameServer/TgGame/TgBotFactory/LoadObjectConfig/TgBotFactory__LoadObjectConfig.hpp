#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

struct SpawnTableEntry {
	int SpawnTableId;
	int SpawnGroup;
	int EnemyBotId;
	int BotCount;
	float SpawnChance;
	std::string ReferenceName;
};

struct BotFactoryConfig {
	int MapObjectId;
	int BotSpawnTableId;
	int TaskForceNumber;
	int ObjectiveBotMapObjectId;
	std::map<int, std::vector<SpawnTableEntry>> SpawnTables;
};

class TgBotFactory__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2d0,
	TgBotFactory__LoadObjectConfig> {
public:
	static std::map<int, ATgBotFactory*> m_loadedBotFactories;
	static std::map<int, BotFactoryConfig> m_botFactoryConfigs;
	static std::map<int, int> m_missionObjectiveBotToBotFactoryId;
	static bool bConfigLoaded;
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

