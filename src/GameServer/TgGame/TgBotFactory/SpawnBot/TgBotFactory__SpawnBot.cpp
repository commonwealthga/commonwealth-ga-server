#include "src/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__SpawnBot::Call(ATgBotFactory* BotFactory, void* edx) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;

	ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
		GA_G::BOT_ID_SUPPORT_GUARDIAN,
		BotFactory->Location,
		BotFactory->Rotation,
		false,
		BotFactory,
		true,
		nullptr,
		false,
		nullptr,
		0.0f
	);

	// guardian main gun
	// ATgDevice* Device = (ATgDevice*)Game->Spawn(
	// 	ClassPreloader::GetTgDeviceClass(),
	// 	(AWorldInfo*)Globals::Get().GWorldInfo,
	// 	FName(),
	// 	BotFactory->Location,
	// 	BotFactory->Rotation,
	// 	Bot,
	// 	1
	// );
	// Device->r_nDeviceId = 4478;
	// Device->m_nDeviceType = 217;
	// Device->r_nQualityValueId = 1165;
	// Device->r_eEquippedAt; // TG_EQUIP_POINT
	// Bot->r_EquipDeviceInfo[0].nDeviceId = 4478;
	// Bot->r_EquipDeviceInfo[0].nDeviceInstanceId = 1;
	// Bot->r_EquipDeviceInfo[0].nQualityValueId = 1165;
	ATgDevice* MainGun = Bot->CreateEquipDevice(0, 4478, Bot->GetEquipPointByType(217));
	if (MainGun != nullptr) {
		Logger::Log("botfactory", "MainGun = 0x%p\n", MainGun);
	} else {
		Logger::Log("botfactory", "MainGun = nullptr\n");
	}

	ATgRepInfo_Player* BotRepInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	Bot->m_CurrentInHandDevice = MainGun;
	for (int i = 0; i < 25; i++) {
		Bot->m_EquippedDevices[i] = MainGun;
		Bot->r_EquipDeviceInfo[i].nDeviceId = 4478;
		Bot->r_EquipDeviceInfo[i].nDeviceInstanceId = 1;
		Bot->r_EquipDeviceInfo[i].nQualityValueId = 1165;

		BotRepInfo->r_EquipDeviceInfo[i].nDeviceId = 4478;
		BotRepInfo->r_EquipDeviceInfo[i].nDeviceInstanceId = 1;
		BotRepInfo->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	}

	Bot->r_eDesiredInHand = 1;
	Bot->m_eEquippedInHand = 1;
	Bot->r_eEquippedInHandMode = 4205;

	// [byte] 0x0206_DEVICE_ID: 4478
	// [byte] 0x0488_SLOT_USED_VALUE_ID: 198
	// [byte] 0x0208_DEVICE_MODE_ID: 4205

	// [byte] 0x0206_DEVICE_ID: 4477
	// [byte] 0x0488_SLOT_USED_VALUE_ID: 200
	// [byte] 0x0206_DEVICE_ID: 3855
	// [byte] 0x0488_SLOT_USED_VALUE_ID: 202
	// [byte] 0x0206_DEVICE_ID: 4344
	// [byte] 0x0488_SLOT_USED_VALUE_ID: 203
	// [byte] 0x0206_DEVICE_ID: 6138
	// [byte] 0x0488_SLOT_USED_VALUE_ID: 385
		

	

	// Logger::Log("botfactory", "\n\nBot Factory 0x%p:\n", BotFactory);
	// Logger::Log("botfactory", "->LocationSelection = %d\n", BotFactory->LocationSelection);
	// Logger::Log("botfactory", "->TypeSelection = %d\n", BotFactory->TypeSelection);
	// if (BotFactory->LocationList.Data == nullptr) {
	// 	Logger::Log("botfactory", "->LocationList = nullptr\n");
	// } else {
	// 	Logger::Log("botfactory", "->LocationList = 0x%p\n", BotFactory->LocationList);
	// 	Logger::Log("botfactory", "->LocationList.Num() = %d\n", BotFactory->LocationList.Num());
	// }
	// Logger::Log("botfactory", "->bPatrolLoop = %d\n", BotFactory->bPatrolLoop);
	// Logger::Log("botfactory", "->bAlwaysPatrol = %d\n", BotFactory->bAlwaysPatrol);
	// Logger::Log("botfactory", "->bSpawnOnAlarm = %d\n", BotFactory->bSpawnOnAlarm);
	// Logger::Log("botfactory", "->bAutoSpawn = %d\n", BotFactory->bAutoSpawn);
	// Logger::Log("botfactory", "->m_bFirstSpawn = %d\n", BotFactory->m_bFirstSpawn);
	// Logger::Log("botfactory", "->bBulkSpawn = %d\n", BotFactory->bBulkSpawn);
	// Logger::Log("botfactory", "->bRespawn = %d\n", BotFactory->bRespawn);
	// Logger::Log("botfactory", "->m_bIgnoreCollisionOnSpawn = %d\n", BotFactory->m_bIgnoreCollisionOnSpawn);
	// if (BotFactory->PatrolPath.Data == nullptr) {
	// 	Logger::Log("botfactory", "->PatrolPath = nullptr\n");
	// } else {
	// 	Logger::Log("botfactory", "->PatrolPath = 0x%p\n", BotFactory->PatrolPath);
	// 	Logger::Log("botfactory", "->PatrolPath.Num() = %d\n", BotFactory->PatrolPath.Num());
	// }
	// Logger::Log("botfactory", "->nGlobalAlarmId = %d\n", BotFactory->nGlobalAlarmId);
	// Logger::Log("botfactory", "->nBotCount = %d\n", BotFactory->nBotCount);
	// Logger::Log("botfactory", "->nCurrentCount = %d\n", BotFactory->nCurrentCount);
	// Logger::Log("botfactory", "->nActiveCount = %d\n", BotFactory->nActiveCount);
	// Logger::Log("botfactory", "->nTotalSpawns = %d\n", BotFactory->nTotalSpawns);
	// Logger::Log("botfactory", "->nSpawnTableId = %d\n", BotFactory->nSpawnTableId);
	// Logger::Log("botfactory", "->nDefaultSpawnTableId = %d\n", BotFactory->nDefaultSpawnTableId);
	// Logger::Log("botfactory", "->fSpawnDelay = %f\n", BotFactory->fSpawnDelay);
	// if (BotFactory->m_SpawnQueue.Data == nullptr) {
	// 	Logger::Log("botfactory", "->m_SpawnQueue = nullptr\n");
	// } else {
	// 	Logger::Log("botfactory", "->m_SpawnQueue = %d\n", BotFactory->m_SpawnQueue);
	// }
	// if (BotFactory->m_SpawnGroups.Data == nullptr) {
	// 	Logger::Log("botfactory", "->m_SpawnGroups = nullptr\n");
	// } else {
	// 	Logger::Log("botfactory", "->m_SpawnGroups = %d\n", BotFactory->m_SpawnGroups);
	// }
	// Logger::Log("botfactory", "->m_nLastGroup = %d\n", BotFactory->m_nLastGroup);
	// Logger::Log("botfactory", "->nPriority = %d\n", BotFactory->nPriority);
	// Logger::Log("botfactory", "->nPrevPriority = %d\n", BotFactory->nPrevPriority);
	// Logger::Log("botfactory", "->fLastKillTime = %f\n", BotFactory->fLastKillTime);
	// Logger::Log("botfactory", "->fBalance = %f\n", BotFactory->fBalance);
	// if (BotFactory->TypeList.Data == nullptr) {
	// 	Logger::Log("botfactory", "->TypeList = nullptr\n");
	// } else {
	// 	Logger::Log("botfactory", "->TypeList = %d\n", BotFactory->TypeList);
	// }
	// Logger::Log("botfactory", "->fRespawnDelay = %f\n", BotFactory->fRespawnDelay);
	// Logger::Log("botfactory", "->m_nSpawnOrder = %d\n", BotFactory->m_nSpawnOrder);

}

