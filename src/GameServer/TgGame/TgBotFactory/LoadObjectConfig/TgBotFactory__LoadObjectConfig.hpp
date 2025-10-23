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

class TgBotFactory__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2d0,
	TgBotFactory__LoadObjectConfig> {
public:
	static std::map<int, int> m_mapObjectIdToSpawnTableId;
	static std::map<int, int> m_mapObjectIdToTaskForceNumber;
	static std::map<int, std::map<int, std::vector<SpawnTableEntry>>> m_spawnTables;
	static bool bConfigLoaded;
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

