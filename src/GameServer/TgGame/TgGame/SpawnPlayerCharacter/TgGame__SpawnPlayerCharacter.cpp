#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Constants/DeviceIds.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/SyncPawnHealth/SyncPawnHealth.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool TgGame__SpawnPlayerCharacter::bEnemyGearSpawned = true;  // now handled in SpawnBotById

// Single-line dump of all 17 FCustomCharacterAssembly ints + bitflags.
// Diagnostic-only — caller passes a `where` tag so a sequence of snapshots
// shows the timeline of writes across the spawn pipeline. Lives in this
// translation unit (rather than a header) because it's not used by anyone
// else and we want to keep the snapshot format consistent here.
static void LogAssemblySnapshot(const char* where, void* obj, const FCustomCharacterAssembly& a) {
	Logger::Log("spawn-asm",
		"%s obj=%p Suit=%d Head=%d Hair=%d Helmet=%d "
		"Skin=%d SkinRace=%d Eye=%d "
		"bBald=%d bHideHelmet=%d bValid=%d bHalfHelmet=%d Gender=%d "
		"HeadFlair=%d SuitFlair=%d Trail=%d "
		"Dye=[%d,%d,%d,%d,%d]\n",
		where, obj,
		a.SuitMeshId, a.HeadMeshId, a.HairMeshId, a.HelmetMeshId,
		a.SkinToneParameterId, a.SkinRaceParameterId, a.EyeColorParameterId,
		(int)a.bBald, (int)a.bHideHelmet, (int)a.bValidCustomAssembly, (int)a.bHalfHelmet,
		a.nGenderTypeId,
		a.HeadFlairId, a.SuitFlairId, a.JetpackTrailId,
		a.DyeList[0], a.DyeList[1], a.DyeList[2], a.DyeList[3], a.DyeList[4]);
}

static bool AddTouchingIfMissing(AActor* Actor, AActor* Other) {
	if (!Actor || !Other) return false;
	for (int i = 0; i < Actor->Touching.Count; ++i) {
		if (Actor->Touching.Data[i] == Other) return false;
	}
	Actor->Touching.Add(Other);
	return true;
}

static bool RepairSpawnVolumeTouch(ATgPawn_Character* Pawn, AVolume* Volume, const char* Kind, int MapObjectId) {
	if (!Pawn || !Volume) return false;
	if (!Volume->Encompasses((AActor*)Pawn)) return false;

	bool addedPawnSide = AddTouchingIfMissing((AActor*)Pawn, (AActor*)Volume);
	bool addedVolumeSide = AddTouchingIfMissing((AActor*)Volume, (AActor*)Pawn);
	if (addedPawnSide || addedVolumeSide) {
		Logger::Log("spawn",
			"SpawnPlayerCharacter: repaired %s overlap pawn=%p volume=%p mapObjectId=%d pawnTouching=%d volumeTouching=%d\n",
			Kind, Pawn, Volume, MapObjectId,
			(int)addedPawnSide, (int)addedVolumeSide);
	}
	return addedPawnSide || addedVolumeSide;
}

static void RepairSpawnVolumeState(ATgPawn_Character* Pawn) {
	if (!Pawn) return;

	ActorCache::CacheMapActors();

	bool repairedModify = false;
	bool repairedOmega = false;
	bool recalcModify = false;
	bool recalcOmega = false;
	int modifyOverlaps = 0;
	int omegaOverlaps = 0;
	const int modifyVolumes = (int)ActorCache::ModifyPawnPropertiesVolumes.size();
	const int omegaVolumes  = (int)ActorCache::OmegaVolumes.size();

	for (ATgModifyPawnPropertiesVolume* volume : ActorCache::ModifyPawnPropertiesVolumes) {
		if (volume && ((AVolume*)volume)->Encompasses((AActor*)Pawn)) {
			++modifyOverlaps;
			recalcModify = true;
			repairedModify |= RepairSpawnVolumeTouch(
				Pawn, (AVolume*)volume,
				"TgModifyPawnPropertiesVolume", volume->m_nMapObjectId);
		}
	}
	for (ATgOmegaVolume* volume : ActorCache::OmegaVolumes) {
		if (volume && ((AVolume*)volume)->Encompasses((AActor*)Pawn)) {
			++omegaOverlaps;
			recalcOmega = true;
			repairedOmega |= RepairSpawnVolumeTouch(
				Pawn, (AVolume*)volume,
				"TgOmegaVolume", volume->m_nMapObjectId);
		}
	}

	if (recalcModify) {
		Pawn->eventModifyPawnPropertiesVolumeChanged();
	}
	if (recalcOmega) {
		Pawn->eventOmegaVolumePropertiesChanged();
	}
	if (recalcModify || recalcOmega) {
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
		Logger::Log("spawn",
			"SpawnPlayerCharacter: volume state refreshed pawn=%p disableAllDevices=%d enableEquip=%d enableSkills=%d currentOmega=%p\n",
			Pawn, (int)Pawn->r_bDisableAllDevices, (int)Pawn->r_bEnableEquip,
			(int)Pawn->r_bEnableSkills, Pawn->r_CurrentOmegaVolume);
	}
	Logger::Log("spawn",
		"SpawnPlayerCharacter: volume scan pawn=%p loc=(%.1f,%.1f,%.1f) modify=%d/%d omega=%d/%d recalcModify=%d recalcOmega=%d repairedModify=%d repairedOmega=%d disableAllDevices=%d enableSkills=%d\n",
		Pawn, Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z,
		modifyOverlaps, modifyVolumes, omegaOverlaps, omegaVolumes,
		(int)recalcModify, (int)recalcOmega,
		(int)repairedModify, (int)repairedOmega,
		(int)Pawn->r_bDisableAllDevices, (int)Pawn->r_bEnableSkills);
}

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	LogCallBegin();

	int32_t ConnectionIndex = (int32_t)((UNetConnection*)PlayerController->Player);

	Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: connection=%d\n", ConnectionIndex);
	Logger::Log("spawn-asm",
		"=== SpawnPlayerCharacter ENTRY conn=%d charId=%lld profileId=%u ===\n",
		ConnectionIndex,
		(long long)GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_character_id,
		GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_profile_id);

	// Read the player's selected class from connection data.
	uint32_t profileId = GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_profile_id;

	// Validate: fall back to Assault (680) for unknown or unset profile IDs.
	if (profileId != 567 && profileId != 679 && profileId != 680 && profileId != 681) {
		Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: invalid profile_id=%d for connection=%d, falling back to Assault\n", profileId, ConnectionIndex);
		profileId = 681;
		// profileId = 680;
	}

	const ClassConfig& classConfig = GetClassConfig(profileId);

	// nPendingBotId = profileId so InitializeDefaultProps loads the correct class stats from DB.
	// For player classes, bot_id == profile_id in asm_data_set_bots.
	TgPawn__InitializeDefaultProps::nPendingBotId = profileId;
	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);
	// nPendingBotId cleared inside InitializeDefaultProps::Call

	// r_nPawnId is assigned by UC TgPawn.PostBeginPlay via TgGame.GetNextPawnId()
	// during Spawn() — per-TgGame-instance monotonic counter. Don't override.
	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);

	// SpawnPlayerCharacter no longer pre-fills r_CustomCharacterAssembly or
	// s_OrigCustomCharacterAssembly. CosmeticEquip::LoadFromDB owns the
	// baseline write (via WriteBaselineAssembly) and already addresses the
	// hair=0 → crash bug by hardcoding hairMesh=1757 for both genders. An
	// earlier pre-Fill block in this slot wrote to s_Orig — a server-only
	// field whose CDO default has bValidCustomAssembly=false — and overlapped
	// the post-spawn inventory pipeline in a way that regressed device-bar
	// display. Snapshots stay so the spawn-asm channel still narrates state.
	const int64_t characterId = GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_character_id;
	Logger::Log("spawn-asm", "post-Spawn newpawn=%p newrepplayer=%p charId=%lld\n",
		newpawn, newrepplayer, (long long)characterId);
	LogAssemblySnapshot("[post-Spawn pawn]", newpawn, newpawn->r_CustomCharacterAssembly);
	LogAssemblySnapshot("[post-Spawn Orig]", newpawn, newpawn->s_OrigCustomCharacterAssembly);
	LogAssemblySnapshot("[post-Spawn PRI ]", newrepplayer, newrepplayer->r_CustomCharacterAssembly);

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;
	// RestartPlayer owns Possess after this returns; doing it here runs before
	// cosmetics, inventory, PRI/team state, and effect ownership are ready.

	// NOTE: don't call ClientSetCinematicMode here — we're still inside
	// PostLogin → RestartPlayer → SpawnDefaultPawnFor, i.e. BEFORE
	// GenericPlayerInitialization → ClientSetHUD spawns the real TgHUD_Game
	// on the client. Firing the RPC now would target the stale TgLoginHUD.
	// It's invoked from MarshalChannel__NotifyControlMessage::HandlePlayerConnected
	// after eventPostLogin completes.






	// undefined4 __fastcall TgDevice__IsOffhandJetpack(int param_1)
	// {
	//   if (((*(uint *)(param_1 + 0x22c) & 0x8000) != 0) && (*(int *)(param_1 + 0x298) == 0x326)) {
	// 	return 1;
	//   }
	//   return 0;
	// }
	// if (device->m_bIsOffHand && device->m_nDeviceType == 806) {

	// // Logger::DumpMemory("newpawn", newpawn, 0x800, 0);
	// int AgonizerSlot = newpawn->GetEquipPointByType(217);
	// ATgDevice* Agonizer = newpawn->CreateEquipDevice(0, GA_G::DEVICE_ID_AGONIZER, AgonizerSlot);
	// Agonizer->bNetInitial = 1;
	// Agonizer->bNetDirty = 1;
	// Agonizer->bForceNetUpdate = 1;
	//
	//
	// // try equip jetpack
	// ATgDevice* Jetpack = newpawn->CreateEquipDevice(0, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
	// if (Jetpack != nullptr) {
	// 	Jetpack->m_bIsOffHand = 1;
	// 	Jetpack->m_nDeviceType = 806;
	// 	Jetpack->bNetInitial = 1;
	// 	Jetpack->bNetDirty = 1;
	// 	Jetpack->bForceNetUpdate = 1;
	// 	Logger::Log("debug", "Jetpack = 0x%p, %s\n", Jetpack, Jetpack->GetFullName());
	// 	Logger::Log("debug", "Jetpack->r_nInventoryId = %d", Jetpack->r_nInventoryId);
	//
	// 	if (Jetpack->s_InventoryObject != nullptr) {
	// 		Logger::Log("debug", "Jetpack->s_InventoryObject = 0x%p, %s\n", Jetpack->s_InventoryObject, Jetpack->s_InventoryObject->GetFullName());
	// 	} else {
	// 		Logger::Log("debug", "Jetpack->s_InventoryObject = nullptr\n");
	// 	}
	//
	//
	// 	for (int i = 0; i < 25; i++) {
	// 		if (i == AgonizerSlot) {
	// 			newpawn->m_EquippedDevices[i] = Agonizer;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_AGONIZER;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 1;
	// 			newpawn->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	//
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_AGONIZER;
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 1;
	// 			newrepplayer->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	// 		} else {
	// 			newpawn->m_EquippedDevices[i] = Jetpack;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 2;
	// 			newpawn->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	//
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 2;
	// 			newrepplayer->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	// 		}
	// 	}
	//
	// 	Jetpack->r_eEquippedAt = GA_G::EQUIP_POINT_JETPACK_ID;
	// 	Agonizer->r_eEquippedAt = AgonizerSlot;
	//
	// 	// for (int i = 0; i < Jetpack->m_FireMode.Num(); i++) {
	// 	// 	UTgDeviceFire* FireMode = Jetpack->m_FireMode.Data[i];
	// 	// 	Jetpack->CurrentFireMode = FireMode->m_nFireType;
	// 	// }
	// 	// Jetpack->CurrentFireMode = 2;
	//
	// 	Jetpack->r_nInventoryId = nMaxInventoryId + 2;
	// 	Jetpack->s_InventoryObject->m_InventoryData.nInvId = nMaxInventoryId + 2;
	//
	// 	if ((char*)newpawn->InvManager + 0x1f0 == nullptr) {
	// 		TMap__Allocate::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0));
	// 	}
	// 	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0), edx, nMaxInventoryId + 2, Jetpack->s_InventoryObject);
	// 	ATgDevice* JetpackFixed = newpawn->CreateEquipDevice(nMaxInventoryId + 2, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
	// 	if (JetpackFixed != nullptr) {
	// 		Logger::Log("debug", "JetpackFixed = 0x%p, %s\n", JetpackFixed, JetpackFixed->GetFullName());
	// 	} else {
	// 		Logger::Log("debug", "JetpackFixed = nullptr\n");
	// 	}
	//
	// 	// Logger::DumpMemory("jetpack", JetpackFixed, 0x2D8, 0);
	//
	// 	Agonizer->r_nInventoryId = nMaxInventoryId + 1;
	// 	Agonizer->s_InventoryObject->m_InventoryData.nInvId =nMaxInventoryId + 1;
	// 	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0), edx, nMaxInventoryId + 1, Agonizer->s_InventoryObject);
	//
	// 	// newpawn->ForceUpdateEquippedDevices();
	// 	// newpawn->UpdateClientDevices();
	// } else {
	// 	Logger::Log("debug", "Jetpack = nullptr\n");
	// }

	newpawn->r_nPhysicalType = 860;
	newpawn->ReplicatedCollisionType = newpawn->CollisionType;
	// HP and power pool are initialized by InitializeDefaultProps from
	// asm_data_set_bots. r_nHealthMaximum is the post-init source of truth
	// (Engine APawn::HealthMax isn't written by SetProperty(304)).
	int hp = newpawn->r_nHealthMaximum;
	newpawn->r_nProfileId = classConfig.profileId;
	newpawn->r_nProfileTypeValueId = classConfig.classTypeValueId;
	newpawn->r_bDisableAllDevices = 0;
	newpawn->r_bEnableEquip = 1;
	newpawn->r_bEnableSkills = 1;
	newpawn->r_bEnableCrafting = 1;
	newpawn->r_bIsStealthed = 0;
	newpawn->r_bIsBot = 0;
	newpawn->r_bIsDecoy = 0;
	newpawn->r_bIsHacking = 0;
	newpawn->r_fCurrentPowerPool = newpawn->r_fMaxPowerPool;  // set by SetProperty(TGPID_POWERPOOL_MAX)
	newpawn->r_nXp = 999999;

	newpawn->r_EffectManager->r_Owner = newpawn;
	newpawn->r_EffectManager->SetOwner(newpawn);
	newpawn->r_EffectManager->Base = newpawn;
	newpawn->r_EffectManager->Role = 3;

	// Force ROLE_Authority on the server-spawned player pawn, mirroring every
	// other server-spawn path (SpawnBotById/SpawnNextBot/SpawnDeployable/
	// SpawnPet all set Role=3). UWorld::SpawnActor swaps Role<->RemoteRole when
	// bRemoteOwned=true (PlayerController-owned spawns), which would land us
	// at Role=ROLE_AutonomousProxy(2). UC gates ProcessInstantHit on
	// `Instigator.Role == ROLE_Authority` (TgDevice.uc:1593), so without this
	// every player-fired hitscan/repair impact gets dropped before ApplyHit.
	newpawn->Role       = 3;  // ROLE_Authority
	newpawn->RemoteRole = 1;  // ROLE_SimulatedProxy

	// r_nBodyMeshAsmId + r_CustomCharacterAssembly are owned by
	// CosmeticEquip::LoadFromDB (called below, before the device equip loop).
	// It writes the baseline assembly, overlays the character's saved
	// cosmetics, and aligns r_nBodyMeshAsmId with the resolved SuitMeshId so
	// downstream readers (ReplicatedEvent for r_nBodyMeshAsmId,
	// SetCollisionFromMesh, GetBodyMeshId) see consistent state from the
	// initial replication bunch onward.
	newpawn->r_nSkillGroupSetId = classConfig.skillGroupSetId;
	newpawn->s_nCharacterId = (int)GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_character_id;



	// PlayerController->Role = 3;
	// PlayerController->RemoteRole = 2;
	// PlayerController->bNetInitial = 1;
	// PlayerController->bNetDirty = 1;
	// PlayerController->bForceNetUpdate = 1;
	// PlayerController->bReplicateMovement = 1;
	// PlayerController->NetPriority        = 2.0f;
	// PlayerController->NetUpdateFrequency = 10.0f;

	newpawn->PlayerReplicationInfo = PlayerController->PlayerReplicationInfo;

	// newpawn->NetPriority        = 10.0f;
	// newpawn->NetUpdateFrequency = 10.0f;
	// newpawn->bNetInitial = 1;
	// newpawn->bNetDirty = 1;
	// newpawn->bForceNetUpdate = 1;
	// newpawn->bReplicateMovement = 1;


	// newrepplayer->Team = defenders;
	newrepplayer->bAdmin = 1;
	// PRI's r_CustomCharacterAssembly is also written by CosmeticEquip::LoadFromDB
	// (it mirrors every assembly field from the pawn so the client's
	// UpdateCharacterAssetRefs preload sees the final cosmetic state).
	newrepplayer->r_nProfileId = classConfig.profileId;
	// PRI is now wired to newpawn — fan HP across all 7 storage locations
	// (engine fields + property descriptors + PRI replicated fields).
	SyncPawnHealth::Apply(newpawn, hp, hp);
	newrepplayer->r_nCharacterId = newpawn->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawn;
	newrepplayer->r_ApproxLocation = newpawn->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");


	// eventSetPlayerName passes FString to ProcessEvent via memcpy (shallow copy).
	// ProcessEvent's cleanup calls appFree on the FString::Data (CPF_NeedCtorLink).
	// If Data was allocated with C++ new[], appFree corrupts the heap.
	// Null out Data after the memcpy inside eventSetPlayerName copies it,
	// so the C++ destructor doesn't also delete[] the same pointer.
	{
		FString playerNameStr(GClientConnectionsData[(int)PlayerController->Player].PlayerInfo.player_name_w.data());
		newrepplayer->eventSetPlayerName(playerNameStr);
		// eventSetPlayerName memcpy'd our FString into its Parms struct.
		// ProcessEvent already consumed (and potentially freed) the Data.
		// Null out so ~FString doesn't double-free.
		playerNameStr.Data = nullptr;
		playerNameStr.Count = playerNameStr.Max = 0;
	}
	// newrepplayer->SetTeam(GTeamsData.Attackers);

	// newrepplayer->r_TaskForce = GTeamsData.Defenders;
	// newrepplayer->Team = GTeamsData.Defenders;
	// newrepplayer->SetTeam(GTeamsData.Defenders);

	{
		int tf = GClientConnectionsData[ConnectionIndex].PlayerInfo.task_force;
		ATgRepInfo_TaskForce* taskforce = (tf == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
		if (newrepplayer->Team == nullptr) {
			newrepplayer->Team = (tf == 1) ? GTeamsData.Defenders : GTeamsData.Attackers;
		}
		newrepplayer->SetTeam(taskforce);
		newrepplayer->r_TaskForce = taskforce;
		newpawn->NotifyTeamChanged();
		Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: assigned to task_force=%d\n", tf);
	}

	// newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// newrepplayer->SetTeam(GTeamsData.Attackers);

	// newpawn->NotifyTeamChanged();

	// newrepplayer->SetTeam(GTeamsData.Defenders);
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	// newrepplayer->bNetDirty = 1;
	// newrepplayer->bForceNetUpdate = 1;
	// newrepplayer->bSkipActorPropertyReplication = 0;
	// newrepplayer->bOnlyDirtyReplication = 0;
	// newrepplayer->bNetInitial = 1;
	// newrepplayer->NetPriority        = 5.0f;
	// newrepplayer->NetUpdateFrequency = 5.0f;

	// if (PlayerController->myHUD != nullptr) {
	// 	PlayerController->myHUD->Destroy();
	// 	PlayerController->myHUD = nullptr;
	// }
	// APlayerController* AController = reinterpret_cast<APlayerController*>(PlayerController);
	// AController->ClientSetHUD(ClassPreloader::GetTgHudTeamGameClass(), Game->ScoreBoardType);
	// Logger::Log("debug", "Client HUD changed");

	// PlayerController->ClientShowHUDElement(5); // MissionInfo
	// PlayerController->ClientShowHUDElement(7); // TeamInfo
	// PlayerController->ClientHideHUDElement(10); // AgentInfo
	// PlayerController->ClientHideHUDElement(11); // QuestTracking 

	newpawn->r_UIClockTime = 15 * 60;
	newpawn->r_UIClockState = 0x00;

	// PlayerController->ViewTarget = newpawn;
	// if (PlayerController->PlayerCamera != nullptr) {
	// 	PlayerController->PlayerCamera->ViewTarget.Controller = PlayerController;
	// 	PlayerController->PlayerCamera->ViewTarget.PRI = newrepplayer;
	// 	PlayerController->PlayerCamera->ViewTarget.Target = newpawn;
	// }
	// PlayerController->RealViewTarget = newrepplayer;

	ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
	ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;

	// Allocate the FString::Data fields via GAllocator (not via the SDK
	// FString constructor, which would use `new wchar_t[]` from the MinGW
	// C++ runtime heap). The engine's `SetTeam` → `RemovePRI` walks
	// m_TeamPlayers when teams shuffle (e.g. on `-changeteam`) and
	// appFrees these Data pointers via FMallocWindows::Free; a `new[]`
	// pointer there hits a null pool-bucket and crashes
	// (write-at-0 at 0x1090b4dc, EAX=0).
	//
	// We bypass the FString class instead of changing it because making
	// ~FString / operator= GAllocator-aware caused immediate double-frees
	// across the broader SDK (auto-generated parm structs memcpy FString
	// fields, producing shared-Data shallow copies that all destruct).
	// Touching just this one handoff site keeps the blast radius minimal.
	auto allocFStringData = [](FString& fs, const wchar_t* src) {
		if (src == nullptr) {
			fs.Data = nullptr;
			fs.Count = fs.Max = 0;
			return;
		}
		const int len = (int)wcslen(src) + 1;
		fs.Data = (wchar_t*)GAllocator::Malloc(len * sizeof(wchar_t));
		fs.Count = fs.Max = len;
		wcscpy(fs.Data, src);
	};

	FTGTEAM_ENTRY newplayerteamentry;
	allocFStringData(newplayerteamentry.fsName,
		GClientConnectionsData[(int32_t)((UNetConnection*)PlayerController->Player)].PlayerInfo.player_name_w.data());
	allocFStringData(newplayerteamentry.fsMapName, L"Tetra Pier");
	newplayerteamentry.nHealth = hp;
	newplayerteamentry.nMaxHealth = hp;
	newplayerteamentry.bLeader = 0;
	newplayerteamentry.pPrep = newrepplayer;

	{
		int tf = GClientConnectionsData[ConnectionIndex].PlayerInfo.task_force;
		if (tf == 1) {
			attackers->m_TeamPlayers.Add(newplayerteamentry);
		} else {
			defenders->m_TeamPlayers.Add(newplayerteamentry);
		}
	}

	// TArray::Add shallow-copies the struct (memcpy-style). The TArray
	// copy now owns the FString Data pointers. Null the local so
	// ~FString (which uses delete[]) doesn't try to free a GAllocator
	// buffer at end of scope. The engine appFrees the entry's copy via
	// its own m_TeamPlayers teardown.
	newplayerteamentry.fsName.Data = nullptr;
	newplayerteamentry.fsName.Count = newplayerteamentry.fsName.Max = 0;
	newplayerteamentry.fsMapName.Data = nullptr;
	newplayerteamentry.fsMapName.Count = newplayerteamentry.fsMapName.Max = 0;

	// GIVE COLONY EYE PET

	// FVector PetSpawnLocation = newpawn->Location;
	// PetSpawnLocation.X += 100;
	// ATgPawn_VanityPet* Pet = (ATgPawn_VanityPet*)Game->SpawnBotById(1657, PetSpawnLocation, newpawn->Rotation, false, nullptr, true, newpawn, false, nullptr, 0.0f);
	// if (Pet != nullptr) {
	// 	newpawn->r_CurrentVanityPet = Pet;
	// }

	// OLD: TgGame__SpawnBotById::GiveDevicesFromBotConfig(newpawn, newrepplayer, PLAYER_BOT_ID);

	Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: profileId=%d, profileType=%d, skillGroupSetId=%d, botId=%d, characterId=%d, hp=%d\n",
	    classConfig.profileId, classConfig.classTypeValueId, classConfig.skillGroupSetId, profileId, newpawn->s_nCharacterId, hp);

	// Read equipped devices from DB.
	//
	// Schema since v53: ga_character_devices is a thin (character_id,
	// inventory_id, equipped_slot) join row; the actual device descriptor
	// (device_id / quality / mods / oc) lives in the account-scoped
	// ga_players_inventory pool, keyed by inventory_id. JOIN here so the
	// in-engine equip path keeps getting the same (device, slot, mods)
	// tuples it did before, just sourced from two tables instead of one.
	//
	// Also auto-attach the profile's class device (slot 14, the rest /
	// HUMAN BASE ATTRIBUTES) from asm_data_set_bots_data_set_bot_devices
	// — that one is not player-equippable so it lives outside the
	// inventory pool and gets re-applied unconditionally on every spawn.
	{
		int64_t charId = (int64_t)newpawn->s_nCharacterId;
		sqlite3* db = Database::GetConnection();

		// Active loadout slot from PlayerInfo (populated by PLAYER_REGISTER
		// IPC). Drives both the ga_character_devices query below and the
		// pawn's r_nItemProfileId so RCST's PHASE 2 ApplyDefaultArmor reads
		// the right armor rows.
		int item_profile_id = 1;
		{
			const std::string& guid = GClientConnectionsData[ConnectionIndex].SessionGuid;
			if (!guid.empty()) {
				PlayerInfo* info = PlayerRegistry::GetByGuidPtr(guid);
				if (info && info->current_item_profile_id >= 1 && info->current_item_profile_id <= 5) {
					item_profile_id = info->current_item_profile_id;
				}
			}
		}
		((ATgPawn_Character*)newpawn)->r_nItemProfileId = item_profile_id;
		// Owned-profile count — CDO default is 1 (retail gated 2..5 behind a
		// vendor purchase). Private-server policy: every character has all 5
		// unlocked from the start. Field is CPF_Net so the client's
		// TgUIAgentProfile button bar (m_ProfileButtonArray[5]) lights up
		// without any further plumbing; the engine native's nId<=Nbr gate in
		// ServerLoadItemProfile also passes.
		((ATgPawn_Character*)newpawn)->r_nItemProfileNbr = 5;
		Logger::Log(GetLogChannel(),
			"SpawnPlayerCharacter: itemProfileId=%d nbr=5 for charId=%lld\n",
			item_profile_id, charId);
		int equippedDeviceActors = 0;

		// (0) Cosmetic assembly first — BEFORE the device equip loop and
		// BEFORE Inventory::Finalize. Why ordering matters:
		//
		// Inventory::Finalize sets bForceNetUpdate=1 + calls UpdateClientDevices
		// which, in this engine, drives the pawn's initial-replication bunch
		// out to the client. Any r_CustomCharacterAssembly writes done AFTER
		// that arrive as a later diff — the client first builds the body mesh
		// from the pre-cosmetic SuitMeshId, attaches Mace and Shield (and any
		// other left-hand-anchored device visuals) to THAT skeleton's bones,
		// THEN OnCustomAssemblyChanged fires and rebuilds the mesh with the
		// cosmetic flair, leaving the attached shield bound to stale bone
		// transforms. Symptom: Mace and Shield's right-click shield rendered
		// rotated ~90° CCW on first spawn, "fixed" only by swapping suit +
		// melee (which forces a fresh attachment against the final mesh).
		//
		// Writing the assembly BEFORE the device loop ensures the initial
		// bunch carries the FINAL flair state. Client builds the body mesh
		// once with the cosmetic, then Mace and Shield attaches to the right
		// skeleton from the start.
		//
		// LoadFromDB filters by i.item_id > 0 so it only touches cosmetic
		// rows (suit flair / helmet flair / dyes / jetpack trail). The
		// gameplay device equip loop below filters by i.device_id > 0 so
		// the two passes never overlap.
		Logger::Log("spawn-asm", "pre-LoadFromDB snapshots (charId=%lld):\n", (long long)charId);
		LogAssemblySnapshot("[pre-LoadFromDB pawn]", newpawn, newpawn->r_CustomCharacterAssembly);
		CosmeticEquip::LoadFromDB(newpawn, charId);
		Logger::Log("spawn-asm", "post-LoadFromDB snapshots:\n");
		LogAssemblySnapshot("[post-LoadFromDB pawn]", newpawn, newpawn->r_CustomCharacterAssembly);
		{
			ATgRepInfo_Player* pri = (ATgRepInfo_Player*)newpawn->PlayerReplicationInfo;
			if (pri) LogAssemblySnapshot("[post-LoadFromDB PRI ]", pri, pri->r_CustomCharacterAssembly);
		}

		// (1) Player-chosen equipped gear.
		// v76: filter to device rows only (i.device_id > 0). Cosmetic rows
		// (item_id > 0, device_id = 0) were already replayed by the
		// LoadFromDB call above; they don't go through Inventory::Equip.
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT i.device_id, d.equipped_slot, i.quality, i.id, i.mod_effect_group_ids "
			"FROM ga_character_devices d "
			"JOIN ga_players_inventory i ON i.id = d.inventory_id "
			"WHERE d.character_id = ? AND d.item_profile_id = ? AND i.device_id > 0 "
			"ORDER BY d.equipped_slot",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, charId);
			sqlite3_bind_int  (stmt, 2, item_profile_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				int deviceId    = sqlite3_column_int(stmt, 0);
				int slot        = sqlite3_column_int(stmt, 1);
				int quality     = sqlite3_column_int(stmt, 2);
				int inventoryId = sqlite3_column_int(stmt, 3);

				// Parse mod_effect_group_ids CSV ("24208,24211,24212,...") into ints.
				std::vector<int> mods;
				const char* csv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
				if (csv && *csv) {
					const char* p = csv;
					while (*p) {
						while (*p == ',' || *p == ' ') ++p;
						if (!*p) break;
						char* end = nullptr;
						long v = std::strtol(p, &end, 10);
						if (end == p) break;
						mods.push_back((int)v);
						p = end;
					}
				}

				Logger::Log("debug",
					"SpawnPlayerCharacter: equip row char=%lld profile=%d slot=%d device=%d inv=%d quality=%d mods=%zu\n",
					charId, item_profile_id, slot, deviceId, inventoryId, quality, mods.size());
				const bool hiddenRestDevice =
					(slot == 14 && deviceId == GA::DeviceId::RestDevice);
				if (hiddenRestDevice) {
					// R self-heal uses this hidden slot; bind the actor but
					// skip startup UpdateClientDevices, which crashes here.
					ATgDevice* restDevice = Inventory::Equip(newpawn, deviceId, slot, quality, inventoryId, mods);
					if (restDevice != nullptr) {
						if (newrepplayer != nullptr) {
							newpawn->r_EquipDeviceInfo[slot] = newrepplayer->r_EquipDeviceInfo[slot];
						}
						restDevice->bNetDirty = 1;
						restDevice->bForceNetUpdate = 1;
						newpawn->bNetDirty = 1;
						newpawn->bForceNetUpdate = 1;
						if (newrepplayer != nullptr) {
							newrepplayer->bNetDirty = 1;
							newrepplayer->bForceNetUpdate = 1;
						}
						Logger::Log("debug",
							"SpawnPlayerCharacter: bound hidden rest device slot=14 char=%lld inv=%d without startup refresh\n",
							charId, inventoryId);
					} else {
						Logger::Log("debug",
							"SpawnPlayerCharacter: failed to bind hidden rest device slot=14 char=%lld inv=%d\n",
							charId, inventoryId);
					}
					continue;
				}
				ATgDevice* equipped = Inventory::Equip(newpawn, deviceId, slot, quality, inventoryId, mods);
				if (equipped != nullptr) {
					++equippedDeviceActors;
					Logger::Log("debug",
						"SpawnPlayerCharacter: equipped slot=%d device=%d inv=%d\n",
						slot, deviceId, inventoryId);
				}
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log(GetLogChannel(),
				"SpawnPlayerCharacter: failed to query ga_character_devices for charId=%lld: %s\n",
				charId, sqlite3_errmsg(db));
		}

		// Class-device slot-14 fallback REMOVED. ga_character_devices is now
		// the single source of truth for equipped state — if the player
		// hasn't equipped slot 14, nothing is equipped there.

		if (equippedDeviceActors > 0) {
			Logger::Log("debug",
				"SpawnPlayerCharacter: deferring UpdateClientDevices for %d startup device actor(s)\n",
				equippedDeviceActors);
			newpawn->bNetDirty = 1;
			newpawn->bForceNetUpdate = 1;
			if (newrepplayer != nullptr) {
				newrepplayer->bNetDirty = 1;
				newrepplayer->bForceNetUpdate = 1;
			}
		} else {
			Logger::Log("debug",
				"SpawnPlayerCharacter: no equipped device actors; skipping UpdateClientDevices finalize\n");
		}

		// Cosmetic assembly was applied BEFORE the device loop above; see the
		// (0) comment up there for why the order matters.

		// r_ItemCount (ATgInventoryManager+0x1e8, CPF_Net) MUST equal the count
		// of non-null entries in m_InventoryMap on the client, otherwise
		// ATgInventoryManager::IsValid() @ 0x10a163f0 returns false. That gates
		// UTgUIAgentProfile_Equip::FixupWidgets's call to FUN_11416a20 — the
		// function that binds invObj → slot widget. Result of mismatch: equip
		// screen renders all slot widgets blank even though m_EquippedDevices
		// is populated and the device bar works.
		//
		// Inventory::Equip increments r_ItemCount by 1 per equipped device,
		// but send_inventory_response on the control server also ships the
		// account-scoped bag pool. The client adds every record to its
		// m_InventoryMap, so its count = equipped + bag. We need r_ItemCount
		// to match the same total. Query the DB for the full pool size.
		{
			sqlite3_stmt* who = nullptr;
			int rc = sqlite3_prepare_v2(db,
				"SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
				-1, &who, nullptr);
			if (rc == SQLITE_OK && who) {
				sqlite3_bind_int64(who, 1, charId);
				if (sqlite3_step(who) == SQLITE_ROW) {
					int64_t  user_id    = sqlite3_column_int64(who, 0);
					int      profile_id = sqlite3_column_int  (who, 1);

					sqlite3_stmt* cnt = nullptr;
					rc = sqlite3_prepare_v2(db,
						"SELECT COUNT(*) FROM ga_players_inventory "
						"WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?)",
						-1, &cnt, nullptr);
					if (rc == SQLITE_OK && cnt) {
						sqlite3_bind_int64(cnt, 1, user_id);
						sqlite3_bind_int  (cnt, 2, profile_id);
						if (sqlite3_step(cnt) == SQLITE_ROW) {
							const int total = sqlite3_column_int(cnt, 0);
							((ATgInventoryManager*)newpawn->InvManager)->r_ItemCount = total;
							Logger::Log(GetLogChannel(),
								"SpawnPlayerCharacter: InvManager->r_ItemCount=%d (pool total, IsValid gate)\n",
								total);
						}
						sqlite3_finalize(cnt);
					}
				}
				sqlite3_finalize(who);
			}
		}
	}

	// Force a post-spawn transform refresh. TgGame.RestartPlayer's bHadPawn
	// branch (TgGame.uc:558-559) does this on every respawn but NOT on the
	// first spawn — and without it, server-side hitscan traces fire from a
	// position that tracks the player but is angularly displaced, making
	// aiming nearly impossible until the player dies and respawns.
	// SetLocation/SetRotation force ConditionalUpdateComponents on the pawn,
	// which re-resolves component-relative transforms (collision cylinder,
	// skeletal mesh, attached weapon meshes) against the now-correct
	// Location/Rotation. Mirrors the respawn fix-up so first spawn matches.
	newpawn->SetLocation(SpawnLocation);
	newpawn->SetRotation(PlayerController->Rotation);
	RepairSpawnVolumeState(newpawn);

	// scope-zoom investigation baseline — snapshot the aim-mode fields right
	// after spawn so the log captures what the FIRST initial-replication bunch
	// will ship to the client. If r_bAimingMode is TRUE here, the client's
	// repnotify fires OnAimingModeChange immediately, the next-tick
	// ManageZoomingClientSide kicks ClientZoomIn before the user has even
	// right-clicked, and the lerp completes invisibly — which would
	// EXACTLY match the symptom ("scope snaps instant, even first time").
	Logger::Log("scope",
		"[SPAWN] pawn=%p r_bAimingMode=%d r_bIsInSnipeScope=%d r_bUsingBinoculars=%d "
		"r_nBodyMeshAsmId=%d charId=%d connIdx=%d\n",
		(void*)newpawn,
		(int)newpawn->r_bAimingMode,
		(int)newpawn->r_bIsInSnipeScope,
		(int)newpawn->r_bUsingBinoculars,
		newpawn->r_nBodyMeshAsmId,
		(int)newpawn->s_nCharacterId,
		ConnectionIndex);

	// Update GPawnSessions and reapply skills. SpawnPlayerCharacter fires both
	// on INITIAL connect AND on RESPAWN — but HandlePlayerConnected (which used
	// to be the only Reapply call site) only fires on initial connect, so
	// without doing it here, skill-based effect groups like Fast Regeneration
	// (skill 902, +80HP/2s passive heal-over-time) die with the previous pawn
	// and never re-attach to the respawn pawn.
	//
	// On initial connect SessionGuid might not be set yet (PLAYER_REGISTER
	// IPC arrives async). In that case skip — HandlePlayerConnected:303 will
	// do the mapping + Reapply once SessionGuid lands. On respawn SessionGuid
	// is already populated from the original connection.
	const std::string& session_guid = GClientConnectionsData[ConnectionIndex].SessionGuid;
	if (!session_guid.empty()) {
		// Drop any stale mapping from this player's previous pawn (death
		// cycle). GPawnSessions is keyed by pawn pointer — the dead pawn is
		// still in the map but no longer relevant. NetConnection::Cleanup
		// erases it on disconnect, but respawn doesn't trigger that path.
		for (auto it = GPawnSessions.begin(); it != GPawnSessions.end(); ) {
			if (it->second == session_guid && it->first != (ATgPawn*)newpawn) {
				it = GPawnSessions.erase(it);
			} else {
				++it;
			}
		}
		GPawnSessions[(ATgPawn*)newpawn] = session_guid;

		Logger::Log(GetLogChannel(),
			"SpawnPlayerCharacter: bound newpawn=%p to session=%s; calling ReapplyCharacterSkillTree\n",
			newpawn, session_guid.c_str());
		newpawn->ReapplyCharacterSkillTree();
	}

	Logger::Log("spawn-asm", "=== SpawnPlayerCharacter EXIT final snapshots ===\n");
	LogAssemblySnapshot("[EXIT pawn]", newpawn, newpawn->r_CustomCharacterAssembly);
	LogAssemblySnapshot("[EXIT Orig]", newpawn, newpawn->s_OrigCustomCharacterAssembly);
	{
		ATgRepInfo_Player* pri = (ATgRepInfo_Player*)newpawn->PlayerReplicationInfo;
		if (pri) LogAssemblySnapshot("[EXIT PRI ]", pri, pri->r_CustomCharacterAssembly);
	}
	LogCallEnd();

	return newpawn;
}

