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

struct BotDeviceEntry {
	int DeviceId;
	int SlotUsedValueId;
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
	// Primary: objective map_object_id -> bot_factory_id (legacy resolution).
	static std::map<int, int> m_missionObjectiveBotToBotFactoryId;
	// Fallbacks consulted by SpawnObjectiveBot in this order when no factory
	// is wired: spawn this exact bot id, or pick one weighted entry from
	// asm_data_set_bot_spawn_tables (lowest spawn_group at our difficulty).
	static std::map<int, int> m_missionObjectiveBotToBotId;
	static std::map<int, int> m_missionObjectiveBotToSpawnTableId;
	// Task force number for objective-spawned bots (1=Attackers, 2=Defenders).
	// Only consulted by tiers 2/3 (bot_id / spawn_table_id fallbacks). The
	// factory path inherits task force from the factory's own s_nTaskForce.
	static std::map<int, int> m_missionObjectiveBotToTaskForce;
	static std::map<int, std::vector<BotDeviceEntry>> m_botDevices;  // bot_id -> devices
	static std::map<int, int> m_botDefaultSlots;                     // bot_id -> default_slot_value_id
	static bool bConfigLoaded;

	static inline int GetEquipPointBySlot(int slotUsedValueId) {
		switch (slotUsedValueId) {
			case 221: return 1;
			case 198: return 2;
			case 200: return 3;
			case 199: return 4;
			case 201: return 5;
			case 202: return 6;
			case 203: return 7;
			case 204: return 8;
			case 385: return 9;
			case 386: return 10;
			case 499: return 11;
			case 500: return 12;
			case 501: return 13;
			case 502: return 14;
			case 823: return 15;
			case 996: return 16;
			case 997: return 17;
			case 998: return 18;
			case 999: return 19;
			case 1000: return 20;
			case 1001: return 21;
			case 1002: return 22;
			case 1003: return 23;
			case 1004: return 24;
			default:   return 0;
		}
	};
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

