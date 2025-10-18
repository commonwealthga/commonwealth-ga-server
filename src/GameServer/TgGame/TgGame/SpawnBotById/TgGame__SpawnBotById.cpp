#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgPawn* __fastcall TgGame__SpawnBotById::Call(
	ATgGame* Game,
	void* edx,
	int nBotId,
	FVector vLocation,
	FRotator rRotation,
	bool bKillController,
	ATgBotFactory* pFactory,
	bool bIgnoreCollision,
	ATgPawn* pOwnerPawn,
	bool bIsDecoy,
	UTgDeviceFire* deviceFire,
	float fDeployAnimLength
) {
	Logger::Log("debug", "Spawning bot with id %d\n", nBotId);

	ATgAIController* AIController = (ATgAIController*)Game->Spawn(
		ClassPreloader::GetTgAIControllerClass(),
		(AWorldInfo*)Globals::Get().GWorldInfo,
		FName(),
		vLocation,
		rRotation,
		nullptr,
		bIgnoreCollision ? 1 : 0
	);
	ATgPawn_Character* Bot = (ATgPawn_Character*)Game->Spawn(
		ClassPreloader::GetTgPawnCharacterClass(),
		AIController->PlayerReplicationInfo,
		FName(),
		vLocation,
		rRotation,
		nullptr,
		bIgnoreCollision ? 1 : 0
	);

	Bot->r_bIsBot = 1;
	AIController->Pawn = Bot;
	Bot->Controller = AIController;
	Bot->Owner = AIController->PlayerReplicationInfo;
	Bot->PlayerReplicationInfo = AIController->PlayerReplicationInfo;

	Bot->r_EffectManager->Owner = Bot;
	Bot->r_EffectManager->r_Owner = Bot;
	Bot->r_EffectManager->Base = Bot;
	Bot->r_EffectManager->Role = 3;

	ATgRepInfo_Player* BotRepInfo = reinterpret_cast<ATgRepInfo_Player*>(AIController->PlayerReplicationInfo);
	BotRepInfo->bBot = 1;
	BotRepInfo->r_PawnOwner = Bot;
	BotRepInfo->r_ApproxLocation = Bot->Location;

	// todo: maybe pull team from pawn/factory?
	BotRepInfo->r_TaskForce = GTeamsData.Defenders;
	BotRepInfo->SetTeam(GTeamsData.Defenders);

	AIController->Possess(Bot, 0, 1);
	Bot->ApplyPawnSetup();
	Bot->WaitForInventoryThenDoPostPawnSetup();

	if (!CAmBot__LoadBotMarshal::m_BotPointers[nBotId]) {
		Logger::Log("debug", "Bot with id %d not found\n", nBotId);
		return nullptr;
	}

	void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[nBotId];

	// offsets pulled from 0x1094c730
	Bot->r_nPhysicalType = *(int*)((char*)BotConfig + 0x64); // PHYSICAL_TYPE_VALUE_ID
	Bot->r_nBodyMeshAsmId = *(int*)((char*)BotConfig + 0x54); // BODY_ASM_ID
	Bot->s_nCharacterId = *(int*)((char*)BotConfig + 0x5C); // BOT_TYPE_VALUE_ID
	Bot->r_nHealthMaximum = *(int*)((char*)BotConfig + 0x74);
	Bot->Health = *(int*)((char*)BotConfig + 0x74);
	Bot->HealthMax = *(int*)((char*)BotConfig + 0x74);
	BotRepInfo->r_nHealthCurrent = *(int*)((char*)BotConfig + 0x74);
	BotRepInfo->r_nHealthMaximum = *(int*)((char*)BotConfig + 0x74);
	BotRepInfo->r_nCharacterId = *(int*)((char*)BotConfig + 0x5C);

	TgAIController__InitBehavior::CallOriginal(AIController, edx, BotConfig, pFactory);

	BotRepInfo->bNetDirty = 1;
	BotRepInfo->bSkipActorPropertyReplication = 0;
	BotRepInfo->bOnlyDirtyReplication = 0;
	BotRepInfo->bNetInitial = 1;
	
	Bot->r_EffectManager->RemoteRole = 1;
	Bot->r_EffectManager->bNetInitial = 1;
	Bot->r_EffectManager->bNetDirty = 1;

	Bot->bNetInitial = 1;
	Bot->bNetDirty = 1;
	Bot->bReplicateMovement = 1;

	Bot->NetUpdateFrequency = 15.0f;
	Bot->NetPriority = 1.2f;

	Bot->r_EffectManager->NetUpdateFrequency = 1.0f;
	Bot->r_EffectManager->NetPriority = 1.0f;

	Bot->PlayerReplicationInfo->NetUpdateFrequency = 1.0f;
	Bot->PlayerReplicationInfo->NetPriority = 1.0f;

	return Bot;
}

