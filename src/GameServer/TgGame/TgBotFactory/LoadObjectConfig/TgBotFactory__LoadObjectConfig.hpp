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
	static std::map<int, int> m_missionObjectiveBotToBotFactoryId;
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

