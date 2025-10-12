#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"

std::map<std::string, std::map<std::string, bool>> Actor__GetOptimizedRepList::ObjectIsACache;


bool Actor__GetOptimizedRepList::ObjectIsA(UObject* obj, const char* ClassFullName) {
    std::string objname = obj->GetFullName();  // assuming GetFullName() returns const char*
    std::string classname = ClassFullName;

    if (ObjectIsACache.find(objname) == ObjectIsACache.end()) {
        ObjectIsACache[objname] = std::map<std::string, bool>();
    }

    if (ObjectIsACache[objname].find(classname) != ObjectIsACache[objname].end()) {
        return ObjectIsACache[objname][classname];
    }

    UClass* objclass = nullptr;

    while (!UObject::GObjObjects()) 
        Sleep(100);

    while (!FName::Names()) 
        Sleep(100);

    for (int i = 0; i < UObject::GObjObjects()->Count; ++i) { 
        UObject* Object = UObject::GObjObjects()->Data[i]; 
        if (!Object) continue; 

        if (!_stricmp(Object->GetFullName(), ClassFullName)) {
            objclass = (UClass*) Object;
            break;
        }
    } 

    if (objclass == nullptr) {
        ObjectIsACache[objname][classname] = false;
        return false;
    }

    for (UClass* SuperClass = obj->Class; SuperClass; SuperClass = (UClass*) SuperClass->SuperField) { 
        if (SuperClass == objclass) {
            ObjectIsACache[objname][classname] = true;
            return true; 
        }
    } 

    ObjectIsACache[objname][classname] = false;
    return false; 
}


bool Actor__GetOptimizedRepList::bRepListCached = false;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Actor_BasedActors_BaseClass = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Actor_BasedActors_BaseClass_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Actor_PhysicsVolumeChange_NewVolume = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Actor_PhysicsVolumeChange_NewVolume_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_Actor_Velocity = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_Actor_Velocity_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_RemoteRole = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_RemoteRole_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_Role = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_Role_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bNetOwner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bNetOwner_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bTearOff = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bTearOff_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Actor_DrawScale = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Actor_DrawScale_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_ReplicatedCollisionType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_Actor_ReplicatedCollisionType_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bCollideActors = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bCollideActors_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bCollideWorld = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bCollideWorld_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bBlockActors = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bBlockActors_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bProjTarget = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Actor_bProjTarget_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_GameFramework_GameExplosionActor_InstigatorController = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_GameFramework_GameExplosionActor_InstigatorController_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Actor_Owner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Actor_Owner_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_CameraActor_AspectRatio = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_CameraActor_AspectRatio_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_CameraActor_FOVAngle = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_CameraActor_FOVAngle_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Controller_PlayerReplicationInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Controller_PlayerReplicationInfo_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_CrowdAttractor_bAttractorEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_CrowdAttractor_bAttractorEnabled_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_CrowdReplicationActor_DestroyAllCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_CrowdReplicationActor_DestroyAllCount_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_CrowdReplicationActor_Spawner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_CrowdReplicationActor_Spawner_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_CrowdReplicationActor_bSpawningActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_CrowdReplicationActor_bSpawningActive_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Engine_DroppedPickup_InventoryClass = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Engine_DroppedPickup_InventoryClass_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_DroppedPickup_bFadeOut = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_DroppedPickup_bFadeOut_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_DynamicSMActor_bForceStaticDecals = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_DynamicSMActor_bForceStaticDecals_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Emitter_bCurrentlyActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Emitter_bCurrentlyActive_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_FluidInfluenceActor_bActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_FluidInfluenceActor_bActive_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_FluidInfluenceActor_bToggled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_FluidInfluenceActor_bToggled_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_FogVolumeDensityInfo_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_FogVolumeDensityInfo_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_MatchID = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_MatchID_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_GameReplicationInfo_Winner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_GameReplicationInfo_Winner_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bMatchIsOver = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bMatchIsOver_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bStopCountDown = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bStopCountDown_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_RemainingMinute = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_RemainingMinute_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_AdminEmail = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_AdminEmail_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_AdminName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_AdminName_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_ElapsedTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_ElapsedTime_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Engine_GameReplicationInfo_GameClass = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Engine_GameReplicationInfo_GameClass_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_GoalScore = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_GoalScore_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_MaxLives = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_MaxLives_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_MessageOfTheDay = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_MessageOfTheDay_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_RemainingTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_RemainingTime_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_ServerName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_ServerName_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_ServerRegion = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_ServerRegion_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_ShortName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_GameReplicationInfo_ShortName_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_TimeLimit = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_GameReplicationInfo_TimeLimit_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bIsArbitrated = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bIsArbitrated_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bTrackStats = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_GameReplicationInfo_bTrackStats_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_HeightFog_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_HeightFog_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Inventory_InvManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Inventory_InvManager_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_InventoryManager_InventoryChain = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_InventoryManager_InventoryChain_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_KActor_RBState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_KActor_RBState_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_KActor_ReplicatedDrawScale3D = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_KActor_ReplicatedDrawScale3D_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_KActor_bWakeOnLevelStart = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_KActor_bWakeOnLevelStart_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_KAsset_ReplicatedMesh = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_KAsset_ReplicatedMesh_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_KAsset_ReplicatedPhysAsset = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_KAsset_ReplicatedPhysAsset_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_LensFlareSource_bCurrentlyActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_LensFlareSource_bCurrentlyActive_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Light_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Light_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_MatineeActor_InterpAction = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_MatineeActor_InterpAction_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_MatineeActor_PlayRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_MatineeActor_PlayRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_MatineeActor_Position = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_MatineeActor_Position_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bIsPlaying = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bIsPlaying_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bPaused = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bPaused_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bReversePlayback = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_MatineeActor_bReversePlayback_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_NxForceField_bForceActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_NxForceField_bForceActive_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_DrivenVehicle = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_DrivenVehicle_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Pawn_FlashLocationUpdated_bViaReplication = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Pawn_FlashLocationUpdated_bViaReplication_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_Pawn_HealthMax = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_Pawn_HealthMax_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Engine_Pawn_HitDamageType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Engine_Pawn_HitDamageType_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_PlayerReplicationInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_PlayerReplicationInfo_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_Pawn_TakeHitLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_Pawn_TakeHitLocation_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bIsWalking = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bIsWalking_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bSimulateGravity = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bSimulateGravity_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AccelRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AccelRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AirControl = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AirControl_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AirSpeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_AirSpeed_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Engine_Pawn_ControllerClass = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Engine_Pawn_ControllerClass_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_GroundSpeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_GroundSpeed_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_InvManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_Pawn_InvManager_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_JumpZ = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_JumpZ_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_WaterSpeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Pawn_WaterSpeed_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Pawn_FiringModeUpdated_bViaReplication = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Pawn_FiringModeUpdated_bViaReplication_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Pawn_FlashCountUpdated_bViaReplication = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Pawn_FlashCountUpdated_bViaReplication_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bIsCrouched = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Pawn_bIsCrouched_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_Pawn_TearOffMomentum = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_Pawn_TearOffMomentum_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_Pawn_RemoteViewPitch = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_Pawn_RemoteViewPitch_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PickupFactory_bPickupHidden = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PickupFactory_bPickupHidden_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_Engine_PickupFactory_InventoryType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_Engine_PickupFactory_InventoryType_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerController_TargetEyeHeight = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerController_TargetEyeHeight_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_PlayerController_TargetViewRotation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_PlayerController_TargetViewRotation_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerReplicationInfo_Deaths = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerReplicationInfo_Deaths_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_PlayerReplicationInfo_PlayerAlias = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_PlayerReplicationInfo_PlayerAlias_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_PlayerReplicationInfo_PlayerName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_PlayerReplicationInfo_PlayerName_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_PlayerSkill = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_PlayerSkill_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerReplicationInfo_Score = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_PlayerReplicationInfo_Score_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_StartTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_StartTime_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_PlayerReplicationInfo_Team = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_PlayerReplicationInfo_Team_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_PlayerReplicationInfo_UniqueId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_PlayerReplicationInfo_UniqueId_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bAdmin = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bAdmin_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bHasFlag = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bHasFlag_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsFemale = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsFemale_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_PlayerReplicationInfo_PacketLoss = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_PlayerReplicationInfo_PacketLoss_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_PlayerReplicationInfo_Ping = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_PlayerReplicationInfo_Ping_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_PlayerID = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_PlayerReplicationInfo_PlayerID_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bBot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bBot_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsInactive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PlayerReplicationInfo_bIsInactive_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_PostProcessVolume_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_PostProcessVolume_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Projectile_MaxSpeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Projectile_MaxSpeed_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_Projectile_Speed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_Projectile_Speed_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_RB_CylindricalForceActor_bForceActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_RB_CylindricalForceActor_bForceActive_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_RB_RadialForceActor_bForceActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_RB_RadialForceActor_bForceActive_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_SVehicle_MaxSpeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_SVehicle_MaxSpeed_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_SVehicle_VState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_SVehicle_VState_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_TeamInfo_Score = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_TeamInfo_Score_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_TeamInfo_TeamIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_TeamInfo_TeamIndex_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_TeamInfo_TeamName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_TeamInfo_TeamName_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_Engine_Teleporter_URL = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_Engine_Teleporter_URL_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_Teleporter_TargetVelocity = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_Teleporter_TargetVelocity_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bChangesVelocity = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bChangesVelocity_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bChangesYaw = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bChangesYaw_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesX = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesX_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesY = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesY_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesZ = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Teleporter_bReversesZ_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_Vehicle_bDriving = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_Vehicle_bDriving_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_Engine_SVehicle_DriverViewPitch = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_Engine_SVehicle_DriverViewPitch_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_Engine_WorldInfo_Pauser = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_Engine_WorldInfo_Pauser_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_Engine_WorldInfo_ReplicatedMusicTrack = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_Engine_WorldInfo_ReplicatedMusicTrack_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_WorldInfo_TimeDilation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_WorldInfo_TimeDilation_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_Engine_WorldInfo_WorldGravityZ = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_Engine_WorldInfo_WorldGravityZ_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_Engine_WorldInfo_bHighPriorityLoading = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_Engine_WorldInfo_bHighPriorityLoading_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgChestActor_r_eChestState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgChestActor_r_eChestState_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bDelayDeployed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bDelayDeployed_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDeployable_r_DRI = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDeployable_r_DRI_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bTakeDamage = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDeployable_r_bTakeDamage_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nDeployableId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nDeployableId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nPhysicalType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nPhysicalType_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nTickingTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nTickingTime_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDeployable_r_Owner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDeployable_r_Owner_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nOwnerFireMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDeployable_r_nOwnerFireMode_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDevice_CurrentFireMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDevice_CurrentFireMode_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bIsStealthDevice = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bIsStealthDevice_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDevice_r_eEquippedAt = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDevice_r_eEquippedAt_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nInventoryId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nInventoryId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nMeleeComboSeed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nMeleeComboSeed_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bConsumedOnUse = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_r_bConsumedOnUse_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nDeviceId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nDeviceId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nDeviceInstanceId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nDeviceInstanceId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nQualityValueId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDevice_r_nQualityValueId_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDoor_r_bOpen = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgDoor_r_bOpen_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDoorMarker_r_eStatus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgDoorMarker_r_eStatus_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDroppedItem_r_nItemId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDroppedItem_r_nItemId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgDynamicSMActor_m_sAssembly = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgDynamicSMActor_m_sAssembly_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgDynamicSMActor_r_nHealth = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgDynamicSMActor_r_nHealth_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgEffectManager_r_EventQueue = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgEffectManager_r_EventQueue_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgEffectManager_r_ManagedEffectList = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgEffectManager_r_ManagedEffectList_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgEffectManager_r_Owner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgEffectManager_r_Owner_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex_initial;
UProperty* Actor__GetOptimizedRepList::NameProperty_TgGame_TgEmitter_BoneName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::NameProperty_TgGame_TgEmitter_BoneName_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius_initial;
UProperty* Actor__GetOptimizedRepList::ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgInterpActor_r_sCurrState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgInterpActor_r_sCurrState_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgInventoryManager_r_ItemCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgInventoryManager_r_ItemCount_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nFailCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nFailCount_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nPassCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgKismetTestActor_r_nPassCount_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgLevelCamera_r_bEnabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgLevelCamera_r_bEnabled_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsActive = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsActive_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsLocked = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsLocked_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsPending = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bIsPending_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eStatus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eStatus_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_nObjectiveId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_nObjectiveId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_nPriority = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgMissionObjective_nPriority_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgObjectiveAssignment_r_eState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgObjectiveAssignment_r_eState_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsBot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsBot_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHenchman = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHenchman_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nAllianceId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nAllianceId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBotRankValueId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBotRankValueId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashEvent = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashEvent_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashFireInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashFireInfo_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashQueIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nFlashQueIndex_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPawnId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPawnId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPhysicalType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPhysicalType_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPreyProfileType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPreyProfileType_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nProfileId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nProfileId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nProfileTypeValueId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nProfileTypeValueId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSoundGroupId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSoundGroupId_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vFlashLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vFlashLocation_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vFlashRayDir = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vFlashRayDir_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_vFlashRefireTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_vFlashRefireTime_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_EquipDeviceInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_EquipDeviceInfo_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_bMadeSound = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_bMadeSound_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eDesiredInHand = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eDesiredInHand_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEquippedInHandMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEquippedInHandMode_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nReplicateHit = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nReplicateHit_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_ControlPawn = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_ControlPawn_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_ScannerSettings = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_ScannerSettings_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UIClockState = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UIClockState_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UIClockTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UIClockTime_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_UITextBox1MessageID = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_UITextBox1MessageID_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UITextBox1Packet = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UITextBox1Packet_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UITextBox1Time = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UITextBox1Time_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_UITextBox2MessageID = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_UITextBox2MessageID_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UITextBox2Packet = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_UITextBox2Packet_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UITextBox2Time = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_UITextBox2Time_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bDisableAllDevices = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bDisableAllDevices_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableCrafting = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableCrafting_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableEquip = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableEquip_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableSkills = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bEnableSkills_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInCombatFlag = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInCombatFlag_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMaxControlRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMaxControlRange_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMaxPowerPool = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMaxPowerPool_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fSkillRating = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fSkillRating_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nCurrency = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nCurrency_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nHZPoints = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nHZPoints_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nRestDeviceSlot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nRestDeviceSlot_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nToken = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nToken_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nXp = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nXp_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_DistanceToPushback = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_DistanceToPushback_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_EffectManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_EffectManager_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_FlightAcceleration = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_FlightAcceleration_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_HangingRotation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_HangingRotation_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Owner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Owner_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Pet = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Pet_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_PlayAnimation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_PlayAnimation_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_PushbackDirection = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_PushbackDirection_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Target = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_Target_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_TargetActor = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_TargetActor_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugDestination = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugDestination_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugNextNav = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugNextNav_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugTarget = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_r_aDebugTarget_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_bAimType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_bAimType_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bAimingMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bAimingMode_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bCallingForHelp = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bCallingForHelp_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsAFK = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsAFK_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsCrafting = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsCrafting_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsCrewing = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsCrewing_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsDecoy = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsDecoy_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHacked = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHacked_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHacking = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHacking_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsRappelling = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsRappelling_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsStealthed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bIsStealthed_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bResistTagging = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bResistTagging_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bTagEnemy = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bTagEnemy_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bUsingBinoculars = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_r_bUsingBinoculars_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eCurrentStunType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eCurrentStunType_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eDeathReason = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_eDeathReason_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteLength = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteLength_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteRepnotify = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteRepnotify_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteUpdate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_eEmoteUpdate_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_ePosture = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_r_ePosture_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fDeployRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fDeployRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fGravityZModifier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fGravityZModifier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fPostureRateScale = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fPostureRateScale_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_fWeightBonus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_fWeightBonus_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_iKnockDownFlash = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_iKnockDownFlash_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nApplyStealth = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nApplyStealth_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBotSoundCueId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nBotSoundCueId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugAggroRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugAggroRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugFOV = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugFOV_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugHearingRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugHearingRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugSightRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nDebugSightRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nHealthMaximum = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nHealthMaximum_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPhase = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPhase_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPitchOffset = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nPitchOffset_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nReplicateDying = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nReplicateDying_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nResetCharacter = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nResetCharacter_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSensorAlertLevel = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSensorAlertLevel_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nShieldHealthMax = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nShieldHealthMax_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSilentMode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nSilentMode_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthAggroRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthAggroRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthDisabled = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthDisabled_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthSensorRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthSensorRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthTypeCode = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nStealthTypeCode_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nYawOffset = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_r_nYawOffset_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sDebugAction = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sDebugAction_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sDebugName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sDebugName_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sFactory = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgPawn_r_sFactory_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vDown = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_r_vDown_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nItemProfileId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nItemProfileId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nMorphSettings = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nMorphSettings_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bGMInvisible = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bGMInvisible_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bRove = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgPlayerController_r_bRove_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgProj_Missile_r_aSeeking = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgProj_Missile_r_aSeeking_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgProj_Missile_r_nNumBounces = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgProj_Missile_r_nNumBounces_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgProj_Rocket_FlockIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgProj_Rocket_FlockIndex_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgProj_Rocket_bCurl = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgProj_Rocket_bCurl_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgProjectile_r_Owner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgProjectile_r_Owner_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fAccelRate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fAccelRate_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fDuration = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fDuration_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fRange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgProjectile_r_fRange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgProjectile_r_nProjectileId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgProjectile_r_nProjectileId_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgProjectile_r_vSpawnLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgProjectile_r_vSpawnLocation_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Beacon_r_nName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Beacon_r_nName_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_Game_r_GameType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_Game_r_GameType_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_Scores = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_Scores_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Player_r_bDropped = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_Player_r_bDropped_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_eBonusType = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_eBonusType_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nLevel = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nLevel_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nProfileId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nProfileId_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName_initial;
UProperty* Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName_initial;
UProperty* Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment_initial;
UProperty* Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget_initial;
UProperty* Actor__GetOptimizedRepList::IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus_initial;
UProperty* Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce_initial;
UProperty* Actor__GetOptimizedRepList::ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgTimerManager_r_fRemaining = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgTimerManager_r_fRemaining_initial;
UProperty* Actor__GetOptimizedRepList::FloatProperty_TgGame_TgTimerManager_r_fStartTime = nullptr;
std::map<int, bool> Actor__GetOptimizedRepList::FloatProperty_TgGame_TgTimerManager_r_fStartTime_initial;

int* __fastcall Actor__GetOptimizedRepList::Call(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5) {
	param_3 = CallOriginal(thisxx, edx_dummy, param_1, param_2, param_3, param_4, param_5);
	if (!bRepListCached) {
		bRepListCached = true;
		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (obj) {
				if (strcmp(obj->GetFullName(), "ClassProperty Actor.BasedActors.BaseClass") == 0) {
					ClassProperty_Actor_BasedActors_BaseClass = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Actor.PhysicsVolumeChange.NewVolume") == 0) {
					ObjectProperty_Actor_PhysicsVolumeChange_NewVolume = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.Actor.Velocity") == 0) {
					StructProperty_Engine_Actor_Velocity = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Actor.RemoteRole") == 0) {
					ByteProperty_Engine_Actor_RemoteRole = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Actor.Role") == 0) {
					ByteProperty_Engine_Actor_Role = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bNetOwner") == 0) {
					BoolProperty_Engine_Actor_bNetOwner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bTearOff") == 0) {
					BoolProperty_Engine_Actor_bTearOff = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Actor.DrawScale") == 0) {
					FloatProperty_Engine_Actor_DrawScale = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Actor.ReplicatedCollisionType") == 0) {
					ByteProperty_Engine_Actor_ReplicatedCollisionType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bCollideActors") == 0) {
					BoolProperty_Engine_Actor_bCollideActors = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bCollideWorld") == 0) {
					BoolProperty_Engine_Actor_bCollideWorld = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bBlockActors") == 0) {
					BoolProperty_Engine_Actor_bBlockActors = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Actor.bProjTarget") == 0) {
					BoolProperty_Engine_Actor_bProjTarget = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty GameFramework.GameExplosionActor.InstigatorController") == 0) {
					ObjectProperty_GameFramework_GameExplosionActor_InstigatorController = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Actor.Owner") == 0) {
					ObjectProperty_Engine_Actor_Owner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.AmbientSoundSimpleToggleable.bCurrentlyPlaying") == 0) {
					BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.CameraActor.AspectRatio") == 0) {
					FloatProperty_Engine_CameraActor_AspectRatio = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.CameraActor.FOVAngle") == 0) {
					FloatProperty_Engine_CameraActor_FOVAngle = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgPlayerController.PawnPerformanceTest.bEnablePerfTest") == 0) {
					BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Controller.PlayerReplicationInfo") == 0) {
					ObjectProperty_Engine_Controller_PlayerReplicationInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.CrowdAttractor.bAttractorEnabled") == 0) {
					BoolProperty_Engine_CrowdAttractor_bAttractorEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.CrowdReplicationActor.DestroyAllCount") == 0) {
					IntProperty_Engine_CrowdReplicationActor_DestroyAllCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.CrowdReplicationActor.Spawner") == 0) {
					ObjectProperty_Engine_CrowdReplicationActor_Spawner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.CrowdReplicationActor.bSpawningActive") == 0) {
					BoolProperty_Engine_CrowdReplicationActor_bSpawningActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty Engine.DroppedPickup.InventoryClass") == 0) {
					ClassProperty_Engine_DroppedPickup_InventoryClass = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.DroppedPickup.bFadeOut") == 0) {
					BoolProperty_Engine_DroppedPickup_bFadeOut = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.DynamicSMActor.ReplicatedMaterial") == 0) {
					ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.DynamicSMActor.ReplicatedMesh") == 0) {
					ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.DynamicSMActor.ReplicatedMeshRotation") == 0) {
					StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.DynamicSMActor.ReplicatedMeshScale3D") == 0) {
					StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.DynamicSMActor.ReplicatedMeshTranslation") == 0) {
					StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.DynamicSMActor.bForceStaticDecals") == 0) {
					BoolProperty_Engine_DynamicSMActor_bForceStaticDecals = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Emitter.bCurrentlyActive") == 0) {
					BoolProperty_Engine_Emitter_bCurrentlyActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.PhysXEmitterSpawnable.ParticleTemplate") == 0) {
					ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.FluidInfluenceActor.bActive") == 0) {
					BoolProperty_Engine_FluidInfluenceActor_bActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.FluidInfluenceActor.bToggled") == 0) {
					BoolProperty_Engine_FluidInfluenceActor_bToggled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.FogVolumeDensityInfo.bEnabled") == 0) {
					BoolProperty_Engine_FogVolumeDensityInfo_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.MatchID") == 0) {
					IntProperty_Engine_GameReplicationInfo_MatchID = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.GameReplicationInfo.Winner") == 0) {
					ObjectProperty_Engine_GameReplicationInfo_Winner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.GameReplicationInfo.bMatchHasBegun") == 0) {
					BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.GameReplicationInfo.bMatchIsOver") == 0) {
					BoolProperty_Engine_GameReplicationInfo_bMatchIsOver = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.GameReplicationInfo.bStopCountDown") == 0) {
					BoolProperty_Engine_GameReplicationInfo_bStopCountDown = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.RemainingMinute") == 0) {
					IntProperty_Engine_GameReplicationInfo_RemainingMinute = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.GameReplicationInfo.AdminEmail") == 0) {
					StrProperty_Engine_GameReplicationInfo_AdminEmail = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.GameReplicationInfo.AdminName") == 0) {
					StrProperty_Engine_GameReplicationInfo_AdminName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.ElapsedTime") == 0) {
					IntProperty_Engine_GameReplicationInfo_ElapsedTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty Engine.GameReplicationInfo.GameClass") == 0) {
					ClassProperty_Engine_GameReplicationInfo_GameClass = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.GoalScore") == 0) {
					IntProperty_Engine_GameReplicationInfo_GoalScore = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.MaxLives") == 0) {
					IntProperty_Engine_GameReplicationInfo_MaxLives = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.GameReplicationInfo.MessageOfTheDay") == 0) {
					StrProperty_Engine_GameReplicationInfo_MessageOfTheDay = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.RemainingTime") == 0) {
					IntProperty_Engine_GameReplicationInfo_RemainingTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.GameReplicationInfo.ServerName") == 0) {
					StrProperty_Engine_GameReplicationInfo_ServerName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.ServerRegion") == 0) {
					IntProperty_Engine_GameReplicationInfo_ServerRegion = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.GameReplicationInfo.ShortName") == 0) {
					StrProperty_Engine_GameReplicationInfo_ShortName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.GameReplicationInfo.TimeLimit") == 0) {
					IntProperty_Engine_GameReplicationInfo_TimeLimit = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.GameReplicationInfo.bIsArbitrated") == 0) {
					BoolProperty_Engine_GameReplicationInfo_bIsArbitrated = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.GameReplicationInfo.bTrackStats") == 0) {
					BoolProperty_Engine_GameReplicationInfo_bTrackStats = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.HeightFog.bEnabled") == 0) {
					BoolProperty_Engine_HeightFog_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Inventory.InvManager") == 0) {
					ObjectProperty_Engine_Inventory_InvManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty SeqAct_GiveInventory.InventoryList.InventoryList") == 0) {
					ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.InventoryManager.InventoryChain") == 0) {
					ObjectProperty_Engine_InventoryManager_InventoryChain = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.KActor.RBState") == 0) {
					StructProperty_Engine_KActor_RBState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.KActor.ReplicatedDrawScale3D") == 0) {
					StructProperty_Engine_KActor_ReplicatedDrawScale3D = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.KActor.bWakeOnLevelStart") == 0) {
					BoolProperty_Engine_KActor_bWakeOnLevelStart = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.KAsset.ReplicatedMesh") == 0) {
					ObjectProperty_Engine_KAsset_ReplicatedMesh = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.KAsset.ReplicatedPhysAsset") == 0) {
					ObjectProperty_Engine_KAsset_ReplicatedPhysAsset = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.LensFlareSource.bCurrentlyActive") == 0) {
					BoolProperty_Engine_LensFlareSource_bCurrentlyActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Light.bEnabled") == 0) {
					BoolProperty_Engine_Light_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.MatineeActor.InterpAction") == 0) {
					ObjectProperty_Engine_MatineeActor_InterpAction = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.MatineeActor.PlayRate") == 0) {
					FloatProperty_Engine_MatineeActor_PlayRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.MatineeActor.Position") == 0) {
					FloatProperty_Engine_MatineeActor_Position = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.MatineeActor.bIsPlaying") == 0) {
					BoolProperty_Engine_MatineeActor_bIsPlaying = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.MatineeActor.bPaused") == 0) {
					BoolProperty_Engine_MatineeActor_bPaused = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.MatineeActor.bReversePlayback") == 0) {
					BoolProperty_Engine_MatineeActor_bReversePlayback = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.NxForceField.bForceActive") == 0) {
					BoolProperty_Engine_NxForceField_bForceActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Pawn.DrivenVehicle") == 0) {
					ObjectProperty_Engine_Pawn_DrivenVehicle = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Pawn.FlashLocationUpdated.bViaReplication") == 0) {
					BoolProperty_Pawn_FlashLocationUpdated_bViaReplication = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.Pawn.HealthMax") == 0) {
					IntProperty_Engine_Pawn_HealthMax = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty Engine.Pawn.HitDamageType") == 0) {
					ClassProperty_Engine_Pawn_HitDamageType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Pawn.PlayerReplicationInfo") == 0) {
					ObjectProperty_Engine_Pawn_PlayerReplicationInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.Pawn.TakeHitLocation") == 0) {
					StructProperty_Engine_Pawn_TakeHitLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Pawn.bIsWalking") == 0) {
					BoolProperty_Engine_Pawn_bIsWalking = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Pawn.bSimulateGravity") == 0) {
					BoolProperty_Engine_Pawn_bSimulateGravity = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.AccelRate") == 0) {
					FloatProperty_Engine_Pawn_AccelRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.AirControl") == 0) {
					FloatProperty_Engine_Pawn_AirControl = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.AirSpeed") == 0) {
					FloatProperty_Engine_Pawn_AirSpeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty Engine.Pawn.ControllerClass") == 0) {
					ClassProperty_Engine_Pawn_ControllerClass = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.GroundSpeed") == 0) {
					FloatProperty_Engine_Pawn_GroundSpeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Pawn.InvManager") == 0) {
					ObjectProperty_Engine_Pawn_InvManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.JumpZ") == 0) {
					FloatProperty_Engine_Pawn_JumpZ = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Pawn.WaterSpeed") == 0) {
					FloatProperty_Engine_Pawn_WaterSpeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Pawn.FiringModeUpdated.bViaReplication") == 0) {
					BoolProperty_Pawn_FiringModeUpdated_bViaReplication = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Pawn.FlashCountUpdated.bViaReplication") == 0) {
					BoolProperty_Pawn_FlashCountUpdated_bViaReplication = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Pawn.bIsCrouched") == 0) {
					BoolProperty_Engine_Pawn_bIsCrouched = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.Pawn.TearOffMomentum") == 0) {
					StructProperty_Engine_Pawn_TearOffMomentum = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Pawn.RemoteViewPitch") == 0) {
					ByteProperty_Engine_Pawn_RemoteViewPitch = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.PhysXEmitterSpawnable.ParticleTemplate") == 0) {
					ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PickupFactory.bPickupHidden") == 0) {
					BoolProperty_Engine_PickupFactory_bPickupHidden = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty Engine.PickupFactory.InventoryType") == 0) {
					ClassProperty_Engine_PickupFactory_InventoryType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.PlayerController.TargetEyeHeight") == 0) {
					FloatProperty_Engine_PlayerController_TargetEyeHeight = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.PlayerController.TargetViewRotation") == 0) {
					StructProperty_Engine_PlayerController_TargetViewRotation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.PlayerReplicationInfo.Deaths") == 0) {
					FloatProperty_Engine_PlayerReplicationInfo_Deaths = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.PlayerReplicationInfo.PlayerAlias") == 0) {
					StrProperty_Engine_PlayerReplicationInfo_PlayerAlias = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.PlayerReplicationInfo.PlayerLocationHint") == 0) {
					ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.PlayerReplicationInfo.PlayerName") == 0) {
					StrProperty_Engine_PlayerReplicationInfo_PlayerName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.PlayerReplicationInfo.PlayerSkill") == 0) {
					IntProperty_Engine_PlayerReplicationInfo_PlayerSkill = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.PlayerReplicationInfo.Score") == 0) {
					FloatProperty_Engine_PlayerReplicationInfo_Score = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.PlayerReplicationInfo.StartTime") == 0) {
					IntProperty_Engine_PlayerReplicationInfo_StartTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.PlayerReplicationInfo.Team") == 0) {
					ObjectProperty_Engine_PlayerReplicationInfo_Team = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.PlayerReplicationInfo.UniqueId") == 0) {
					StructProperty_Engine_PlayerReplicationInfo_UniqueId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bAdmin") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bAdmin = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bHasFlag") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bHasFlag = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bIsFemale") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bIsFemale = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bIsSpectator") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bOnlySpectator") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bOutOfLives") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bReadyToPlay") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bWaitingPlayer") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.PlayerReplicationInfo.PacketLoss") == 0) {
					ByteProperty_Engine_PlayerReplicationInfo_PacketLoss = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.PlayerReplicationInfo.Ping") == 0) {
					ByteProperty_Engine_PlayerReplicationInfo_Ping = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.PlayerReplicationInfo.SplitscreenIndex") == 0) {
					IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.PlayerReplicationInfo.PlayerID") == 0) {
					IntProperty_Engine_PlayerReplicationInfo_PlayerID = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bBot") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bBot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PlayerReplicationInfo.bIsInactive") == 0) {
					BoolProperty_Engine_PlayerReplicationInfo_bIsInactive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.PostProcessVolume.bEnabled") == 0) {
					BoolProperty_Engine_PostProcessVolume_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Projectile.MaxSpeed") == 0) {
					FloatProperty_Engine_Projectile_MaxSpeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.Projectile.Speed") == 0) {
					FloatProperty_Engine_Projectile_Speed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.RB_CylindricalForceActor.bForceActive") == 0) {
					BoolProperty_Engine_RB_CylindricalForceActor_bForceActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.RB_LineImpulseActor.ImpulseCount") == 0) {
					ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.RB_RadialForceActor.bForceActive") == 0) {
					BoolProperty_Engine_RB_RadialForceActor_bForceActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.RB_RadialImpulseActor.ImpulseCount") == 0) {
					ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.SVehicle.MaxSpeed") == 0) {
					FloatProperty_Engine_SVehicle_MaxSpeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.SVehicle.VState") == 0) {
					StructProperty_Engine_SVehicle_VState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.SkeletalMeshActor.ReplicatedMaterial") == 0) {
					ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.SkeletalMeshActor.ReplicatedMesh") == 0) {
					ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.TeamInfo.Score") == 0) {
					FloatProperty_Engine_TeamInfo_Score = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.TeamInfo.TeamIndex") == 0) {
					IntProperty_Engine_TeamInfo_TeamIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.TeamInfo.TeamName") == 0) {
					StrProperty_Engine_TeamInfo_TeamName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty Engine.Teleporter.URL") == 0) {
					StrProperty_Engine_Teleporter_URL = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bEnabled") == 0) {
					BoolProperty_Engine_Teleporter_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.Teleporter.TargetVelocity") == 0) {
					StructProperty_Engine_Teleporter_TargetVelocity = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bChangesVelocity") == 0) {
					BoolProperty_Engine_Teleporter_bChangesVelocity = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bChangesYaw") == 0) {
					BoolProperty_Engine_Teleporter_bChangesYaw = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bReversesX") == 0) {
					BoolProperty_Engine_Teleporter_bReversesX = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bReversesY") == 0) {
					BoolProperty_Engine_Teleporter_bReversesY = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Teleporter.bReversesZ") == 0) {
					BoolProperty_Engine_Teleporter_bReversesZ = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.Vehicle.bDriving") == 0) {
					BoolProperty_Engine_Vehicle_bDriving = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.SVehicle.DriverViewPitch") == 0) {
					IntProperty_Engine_SVehicle_DriverViewPitch = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.WorldInfo.Pauser") == 0) {
					ObjectProperty_Engine_WorldInfo_Pauser = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty Engine.WorldInfo.ReplicatedMusicTrack") == 0) {
					StructProperty_Engine_WorldInfo_ReplicatedMusicTrack = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.WorldInfo.TimeDilation") == 0) {
					FloatProperty_Engine_WorldInfo_TimeDilation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty Engine.WorldInfo.WorldGravityZ") == 0) {
					FloatProperty_Engine_WorldInfo_WorldGravityZ = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty Engine.WorldInfo.bHighPriorityLoading") == 0) {
					BoolProperty_Engine_WorldInfo_bHighPriorityLoading = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgChestActor.r_eChestState") == 0) {
					ByteProperty_TgGame_TgChestActor_r_eChestState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDeploy_BeaconEntrance.r_bActive") == 0) {
					BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDeploy_DestructibleCover.r_bHasFired") == 0) {
					BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeploy_Sensor.r_nSensorAudioWarning") == 0) {
					IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeploy_Sensor.r_nTouchedPlayerCount") == 0) {
					IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDeployable.r_bDelayDeployed") == 0) {
					BoolProperty_TgGame_TgDeployable_r_bDelayDeployed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeployable.r_nReplicateDestroyIt") == 0) {
					IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgDeployable.r_DRI") == 0) {
					ObjectProperty_TgGame_TgDeployable_r_DRI = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDeployable.r_bInitialIsEnemy") == 0) {
					BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDeployable.r_bTakeDamage") == 0) {
					BoolProperty_TgGame_TgDeployable_r_bTakeDamage = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgDeployable.r_fClientProximityRadius") == 0) {
					FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgDeployable.r_fCurrentDeployTime") == 0) {
					FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeployable.r_nDeployableId") == 0) {
					IntProperty_TgGame_TgDeployable_r_nDeployableId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeployable.r_nPhysicalType") == 0) {
					IntProperty_TgGame_TgDeployable_r_nPhysicalType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeployable.r_nTickingTime") == 0) {
					IntProperty_TgGame_TgDeployable_r_nTickingTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgDeployable.r_Owner") == 0) {
					ObjectProperty_TgGame_TgDeployable_r_Owner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDeployable.r_nOwnerFireMode") == 0) {
					IntProperty_TgGame_TgDeployable_r_nOwnerFireMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgDevice.CurrentFireMode") == 0) {
					ByteProperty_TgGame_TgDevice_CurrentFireMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDevice.r_bIsStealthDevice") == 0) {
					BoolProperty_TgGame_TgDevice_r_bIsStealthDevice = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgDevice.r_eEquippedAt") == 0) {
					ByteProperty_TgGame_TgDevice_r_eEquippedAt = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDevice.r_nInventoryId") == 0) {
					IntProperty_TgGame_TgDevice_r_nInventoryId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDevice.r_nMeleeComboSeed") == 0) {
					IntProperty_TgGame_TgDevice_r_nMeleeComboSeed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDevice.r_bConsumedOnDeath") == 0) {
					BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDevice.r_bConsumedOnUse") == 0) {
					BoolProperty_TgGame_TgDevice_r_bConsumedOnUse = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDevice.r_nDeviceId") == 0) {
					IntProperty_TgGame_TgDevice_r_nDeviceId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDevice.r_nDeviceInstanceId") == 0) {
					IntProperty_TgGame_TgDevice_r_nDeviceInstanceId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDevice.r_nQualityValueId") == 0) {
					IntProperty_TgGame_TgDevice_r_nQualityValueId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDevice_Morale.r_bIsActivelyFiring") == 0) {
					BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgDoor.r_bOpen") == 0) {
					BoolProperty_TgGame_TgDoor_r_bOpen = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgDoorMarker.r_eStatus") == 0) {
					ByteProperty_TgGame_TgDoorMarker_r_eStatus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDroppedItem.r_nItemId") == 0) {
					IntProperty_TgGame_TgDroppedItem_r_nItemId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDynamicDestructible.r_nDestructibleId") == 0) {
					IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgDynamicDestructible.r_pFactory") == 0) {
					ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgDynamicSMActor.m_sAssembly") == 0) {
					StrProperty_TgGame_TgDynamicSMActor_m_sAssembly = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgDynamicSMActor.r_EffectManager") == 0) {
					ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgDynamicSMActor.r_nHealth") == 0) {
					IntProperty_TgGame_TgDynamicSMActor_r_nHealth = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgEffectManager.r_EventQueue") == 0) {
					StructProperty_TgGame_TgEffectManager_r_EventQueue = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgEffectManager.r_ManagedEffectList") == 0) {
					StructProperty_TgGame_TgEffectManager_r_ManagedEffectList = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgEffectManager.r_Owner") == 0) {
					ObjectProperty_TgGame_TgEffectManager_r_Owner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgEffectManager.r_bRelevancyNotify") == 0) {
					BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgEffectManager.r_nInvulnerableCount") == 0) {
					IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgEffectManager.r_nNextQueueIndex") == 0) {
					IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "NameProperty TgGame.TgEmitter.BoneName") == 0) {
					NameProperty_TgGame_TgEmitter_BoneName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgFlagCaptureVolume.r_eCoalition") == 0) {
					ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgFlagCaptureVolume.r_nTaskForce") == 0) {
					ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgFracturedStaticMeshActor.r_EffectManager") == 0) {
					ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgFracturedStaticMeshActor.r_TakeHitNotifier") == 0) {
					IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgFracturedStaticMeshActor.r_DamageRadius") == 0) {
					FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ClassProperty TgGame.TgFracturedStaticMeshActor.r_HitDamageType") == 0) {
					ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgFracturedStaticMeshActor.r_HitInfo") == 0) {
					StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgFracturedStaticMeshActor.r_vTakeHitLocation") == 0) {
					StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgFracturedStaticMeshActor.r_vTakeHitMomentum") == 0) {
					StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgHexLandMarkActor.r_nMeshAsmId") == 0) {
					IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgInterpActor.r_sCurrState") == 0) {
					StrProperty_TgGame_TgInterpActor_r_sCurrState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgInventoryManager.r_ItemCount") == 0) {
					IntProperty_TgGame_TgInventoryManager_r_ItemCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgKismetTestActor.r_nCurrentTest") == 0) {
					IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgKismetTestActor.r_nFailCount") == 0) {
					IntProperty_TgGame_TgKismetTestActor_r_nFailCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgKismetTestActor.r_nPassCount") == 0) {
					IntProperty_TgGame_TgKismetTestActor_r_nPassCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgLevelCamera.r_bEnabled") == 0) {
					BoolProperty_TgGame_TgLevelCamera_r_bEnabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgMissionObjective.r_ObjectiveAssignment") == 0) {
					ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgMissionObjective.r_bHasBeenCapturedOnce") == 0) {
					BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgMissionObjective.r_bIsActive") == 0) {
					BoolProperty_TgGame_TgMissionObjective_r_bIsActive = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgMissionObjective.r_bIsLocked") == 0) {
					BoolProperty_TgGame_TgMissionObjective_r_bIsLocked = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgMissionObjective.r_bIsPending") == 0) {
					BoolProperty_TgGame_TgMissionObjective_r_bIsPending = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgMissionObjective.r_eOwningCoalition") == 0) {
					ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgMissionObjective.r_eStatus") == 0) {
					ByteProperty_TgGame_TgMissionObjective_r_eStatus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgMissionObjective.r_fCurrCaptureTime") == 0) {
					FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgMissionObjective.r_fLastCompletedTime") == 0) {
					FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgMissionObjective.r_nOwnerTaskForce") == 0) {
					IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgMissionObjective.nObjectiveId") == 0) {
					IntProperty_TgGame_TgMissionObjective_nObjectiveId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgMissionObjective.nPriority") == 0) {
					IntProperty_TgGame_TgMissionObjective_nPriority = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgMissionObjective.r_OpenWorldPlayerDefaultRole") == 0) {
					ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgMissionObjective.r_bUsePendingState") == 0) {
					BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgMissionObjective.r_eDefaultCoalition") == 0) {
					ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgMissionObjective_Bot.r_ObjectiveBot") == 0) {
					ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgMissionObjective_Bot.r_ObjectiveBotInfo") == 0) {
					ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgMissionObjective_Escort.r_AttachedActor") == 0) {
					ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgMissionObjective_Proximity.r_fCaptureRate") == 0) {
					FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgObjectiveAssignment.r_AssignedObjective") == 0) {
					ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgObjectiveAssignment.r_Attackers") == 0) {
					ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgObjectiveAssignment.r_Bots") == 0) {
					ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgObjectiveAssignment.r_Defenders") == 0) {
					ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgObjectiveAssignment.r_eState") == 0) {
					ByteProperty_TgGame_TgObjectiveAssignment_r_eState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsBot") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsBot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHenchman") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHenchman = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bNeedPlaySpawnFx") == 0) {
					BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fMakeVisibleIncreased") == 0) {
					FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nAllianceId") == 0) {
					IntProperty_TgGame_TgPawn_r_nAllianceId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nBodyMeshAsmId") == 0) {
					IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nBotRankValueId") == 0) {
					IntProperty_TgGame_TgPawn_r_nBotRankValueId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nFlashEvent") == 0) {
					IntProperty_TgGame_TgPawn_r_nFlashEvent = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nFlashFireInfo") == 0) {
					IntProperty_TgGame_TgPawn_r_nFlashFireInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nFlashQueIndex") == 0) {
					IntProperty_TgGame_TgPawn_r_nFlashQueIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nPawnId") == 0) {
					IntProperty_TgGame_TgPawn_r_nPawnId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nPhysicalType") == 0) {
					IntProperty_TgGame_TgPawn_r_nPhysicalType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nPreyProfileType") == 0) {
					IntProperty_TgGame_TgPawn_r_nPreyProfileType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nProfileId") == 0) {
					IntProperty_TgGame_TgPawn_r_nProfileId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nProfileTypeValueId") == 0) {
					IntProperty_TgGame_TgPawn_r_nProfileTypeValueId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nSoundGroupId") == 0) {
					IntProperty_TgGame_TgPawn_r_nSoundGroupId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_vFlashLocation") == 0) {
					StructProperty_TgGame_TgPawn_r_vFlashLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_vFlashRayDir") == 0) {
					StructProperty_TgGame_TgPawn_r_vFlashRayDir = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_vFlashRefireTime") == 0) {
					FloatProperty_TgGame_TgPawn_r_vFlashRefireTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_vFlashSituationalAttack") == 0) {
					IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_EquipDeviceInfo") == 0) {
					StructProperty_TgGame_TgPawn_r_EquipDeviceInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bInitialIsEnemy") == 0) {
					BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_bMadeSound") == 0) {
					ByteProperty_TgGame_TgPawn_r_bMadeSound = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_eDesiredInHand") == 0) {
					ByteProperty_TgGame_TgPawn_r_eDesiredInHand = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_eEquippedInHandMode") == 0) {
					IntProperty_TgGame_TgPawn_r_eEquippedInHandMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nReplicateHit") == 0) {
					IntProperty_TgGame_TgPawn_r_nReplicateHit = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_ControlPawn") == 0) {
					ObjectProperty_TgGame_TgPawn_r_ControlPawn = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_CurrentOmegaVolume") == 0) {
					ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_CurrentSubzoneBilboardVol") == 0) {
					ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_CurrentSubzoneVol") == 0) {
					ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_ScannerSettings") == 0) {
					StructProperty_TgGame_TgPawn_r_ScannerSettings = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_UIClockState") == 0) {
					ByteProperty_TgGame_TgPawn_r_UIClockState = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_UIClockTime") == 0) {
					FloatProperty_TgGame_TgPawn_r_UIClockTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_UITextBox1MessageID") == 0) {
					IntProperty_TgGame_TgPawn_r_UITextBox1MessageID = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_UITextBox1Packet") == 0) {
					ByteProperty_TgGame_TgPawn_r_UITextBox1Packet = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_UITextBox1Time") == 0) {
					FloatProperty_TgGame_TgPawn_r_UITextBox1Time = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_UITextBox2MessageID") == 0) {
					IntProperty_TgGame_TgPawn_r_UITextBox2MessageID = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_UITextBox2Packet") == 0) {
					ByteProperty_TgGame_TgPawn_r_UITextBox2Packet = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_UITextBox2Time") == 0) {
					FloatProperty_TgGame_TgPawn_r_UITextBox2Time = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bAllowAddMoralePoints") == 0) {
					BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bDisableAllDevices") == 0) {
					BoolProperty_TgGame_TgPawn_r_bDisableAllDevices = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bEnableCrafting") == 0) {
					BoolProperty_TgGame_TgPawn_r_bEnableCrafting = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bEnableEquip") == 0) {
					BoolProperty_TgGame_TgPawn_r_bEnableEquip = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bEnableSkills") == 0) {
					BoolProperty_TgGame_TgPawn_r_bEnableSkills = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bInCombatFlag") == 0) {
					BoolProperty_TgGame_TgPawn_r_bInCombatFlag = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bInGlobalOffhandCooldown") == 0) {
					BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fCurrentPowerPool") == 0) {
					FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fCurrentServerMoralePoints") == 0) {
					FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fMaxControlRange") == 0) {
					FloatProperty_TgGame_TgPawn_r_fMaxControlRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fMaxPowerPool") == 0) {
					FloatProperty_TgGame_TgPawn_r_fMaxPowerPool = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fMoraleRechargeRate") == 0) {
					FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fRequiredMoralePoints") == 0) {
					FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fSkillRating") == 0) {
					FloatProperty_TgGame_TgPawn_r_fSkillRating = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nCurrency") == 0) {
					IntProperty_TgGame_TgPawn_r_nCurrency = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nHZPoints") == 0) {
					IntProperty_TgGame_TgPawn_r_nHZPoints = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nMoraleDeviceSlot") == 0) {
					IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nRestDeviceSlot") == 0) {
					IntProperty_TgGame_TgPawn_r_nRestDeviceSlot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nToken") == 0) {
					IntProperty_TgGame_TgPawn_r_nToken = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nXp") == 0) {
					IntProperty_TgGame_TgPawn_r_nXp = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_DistanceToPushback") == 0) {
					FloatProperty_TgGame_TgPawn_r_DistanceToPushback = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_EffectManager") == 0) {
					ObjectProperty_TgGame_TgPawn_r_EffectManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_FlightAcceleration") == 0) {
					FloatProperty_TgGame_TgPawn_r_FlightAcceleration = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_HangingRotation") == 0) {
					StructProperty_TgGame_TgPawn_r_HangingRotation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_Owner") == 0) {
					ObjectProperty_TgGame_TgPawn_r_Owner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_Pet") == 0) {
					ObjectProperty_TgGame_TgPawn_r_Pet = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_PlayAnimation") == 0) {
					StructProperty_TgGame_TgPawn_r_PlayAnimation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_PushbackDirection") == 0) {
					StructProperty_TgGame_TgPawn_r_PushbackDirection = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_Target") == 0) {
					ObjectProperty_TgGame_TgPawn_r_Target = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_TargetActor") == 0) {
					ObjectProperty_TgGame_TgPawn_r_TargetActor = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_aDebugDestination") == 0) {
					ObjectProperty_TgGame_TgPawn_r_aDebugDestination = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_aDebugNextNav") == 0) {
					ObjectProperty_TgGame_TgPawn_r_aDebugNextNav = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn.r_aDebugTarget") == 0) {
					ObjectProperty_TgGame_TgPawn_r_aDebugTarget = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_bAimType") == 0) {
					ByteProperty_TgGame_TgPawn_r_bAimType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bAimingMode") == 0) {
					BoolProperty_TgGame_TgPawn_r_bAimingMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bCallingForHelp") == 0) {
					BoolProperty_TgGame_TgPawn_r_bCallingForHelp = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsAFK") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsAFK = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsAnimInStrafeMode") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsCrafting") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsCrafting = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsCrewing") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsCrewing = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsDecoy") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsDecoy = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsGrappleDismounting") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHacked") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHacked = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHacking") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHacking = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHangingDismounting") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHangingDismounting") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsInSnipeScope") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsRappelling") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsRappelling = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsStealthed") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsStealthed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bJumpedFromHanging") == 0) {
					BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bPostureIgnoreTransition") == 0) {
					BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bResistTagging") == 0) {
					BoolProperty_TgGame_TgPawn_r_bResistTagging = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bShouldKnockDownAnimFaceDown") == 0) {
					BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bTagEnemy") == 0) {
					BoolProperty_TgGame_TgPawn_r_bTagEnemy = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bUsingBinoculars") == 0) {
					BoolProperty_TgGame_TgPawn_r_bUsingBinoculars = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_eCurrentStunType") == 0) {
					ByteProperty_TgGame_TgPawn_r_eCurrentStunType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_eDeathReason") == 0) {
					ByteProperty_TgGame_TgPawn_r_eDeathReason = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_eEmoteLength") == 0) {
					IntProperty_TgGame_TgPawn_r_eEmoteLength = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_eEmoteRepnotify") == 0) {
					IntProperty_TgGame_TgPawn_r_eEmoteRepnotify = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_eEmoteUpdate") == 0) {
					IntProperty_TgGame_TgPawn_r_eEmoteUpdate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn.r_ePosture") == 0) {
					ByteProperty_TgGame_TgPawn_r_ePosture = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fDeployRate") == 0) {
					FloatProperty_TgGame_TgPawn_r_fDeployRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fFrictionMultiplier") == 0) {
					FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fGravityZModifier") == 0) {
					FloatProperty_TgGame_TgPawn_r_fGravityZModifier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fKnockDownTimeRemaining") == 0) {
					FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fMakeVisibleFadeRate") == 0) {
					FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fPostureRateScale") == 0) {
					FloatProperty_TgGame_TgPawn_r_fPostureRateScale = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fRappelGravityModifier") == 0) {
					FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn.r_fStealthTransitionTime") == 0) {
					FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_fWeightBonus") == 0) {
					IntProperty_TgGame_TgPawn_r_fWeightBonus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_iKnockDownFlash") == 0) {
					IntProperty_TgGame_TgPawn_r_iKnockDownFlash = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nApplyStealth") == 0) {
					IntProperty_TgGame_TgPawn_r_nApplyStealth = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nBotSoundCueId") == 0) {
					IntProperty_TgGame_TgPawn_r_nBotSoundCueId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nDebugAggroRange") == 0) {
					IntProperty_TgGame_TgPawn_r_nDebugAggroRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nDebugFOV") == 0) {
					IntProperty_TgGame_TgPawn_r_nDebugFOV = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nDebugHearingRange") == 0) {
					IntProperty_TgGame_TgPawn_r_nDebugHearingRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nDebugSightRange") == 0) {
					IntProperty_TgGame_TgPawn_r_nDebugSightRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nGenericAIEventIndex") == 0) {
					IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nHealthMaximum") == 0) {
					IntProperty_TgGame_TgPawn_r_nHealthMaximum = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nNumberTimesCrewed") == 0) {
					IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nPhase") == 0) {
					IntProperty_TgGame_TgPawn_r_nPhase = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nPitchOffset") == 0) {
					IntProperty_TgGame_TgPawn_r_nPitchOffset = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nReplicateDying") == 0) {
					IntProperty_TgGame_TgPawn_r_nReplicateDying = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nResetCharacter") == 0) {
					IntProperty_TgGame_TgPawn_r_nResetCharacter = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nSensorAlertLevel") == 0) {
					IntProperty_TgGame_TgPawn_r_nSensorAlertLevel = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nShieldHealthMax") == 0) {
					IntProperty_TgGame_TgPawn_r_nShieldHealthMax = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nShieldHealthRemaining") == 0) {
					IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nSilentMode") == 0) {
					IntProperty_TgGame_TgPawn_r_nSilentMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nStealthAggroRange") == 0) {
					IntProperty_TgGame_TgPawn_r_nStealthAggroRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nStealthDisabled") == 0) {
					IntProperty_TgGame_TgPawn_r_nStealthDisabled = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nStealthSensorRange") == 0) {
					IntProperty_TgGame_TgPawn_r_nStealthSensorRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nStealthTypeCode") == 0) {
					IntProperty_TgGame_TgPawn_r_nStealthTypeCode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn.r_nYawOffset") == 0) {
					IntProperty_TgGame_TgPawn_r_nYawOffset = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgPawn.r_sDebugAction") == 0) {
					StrProperty_TgGame_TgPawn_r_sDebugAction = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgPawn.r_sDebugName") == 0) {
					StrProperty_TgGame_TgPawn_r_sDebugName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgPawn.r_sFactory") == 0) {
					StrProperty_TgGame_TgPawn_r_sFactory = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn.r_vDown") == 0) {
					StructProperty_TgGame_TgPawn_r_vDown = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn_Ambush.r_bIsDeployed") == 0) {
					BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn_AttackTransport.r_DeathType") == 0) {
					ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn_CTR.r_CustomCharacterAssembly") == 0) {
					StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn_CTR.r_PilotPawn") == 0) {
					ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_CTR.r_nMaxMorphIndexSentFromServer") == 0) {
					IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_CTR.r_nMorphSettings") == 0) {
					IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgPawn_Character.r_CustomCharacterAssembly") == 0) {
					StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn_Character.r_eAttachedMesh") == 0) {
					ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nBoostTimeRemaining") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nHeadMeshAsmId") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nItemProfileId") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nItemProfileId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nItemProfileNbr") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nMaxMorphIndexSentFromServer") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nMorphSettings") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nMorphSettings = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgPawn_Character.r_CurrentVanityPet") == 0) {
					ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Character.r_WallJumpUpperLineCheckOffset") == 0) {
					FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Character.r_WallJumpZ") == 0) {
					FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn_Character.r_bElfGogglesEquipped") == 0) {
					BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nDeviceSlotUnlockGrpId") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_Character.r_nSkillGroupSetId") == 0) {
					IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn_DuneCommander.r_bDoCrashLanding") == 0) {
					BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn_Iris.r_nStartNewScan") == 0) {
					ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Reaper.r_fBatteryPct") == 0) {
					FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPawn_Siege.r_AccelDirection") == 0) {
					ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn_Turret.r_bIsDeployed") == 0) {
					BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Turret.r_fInitDeployTime") == 0) {
					FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Turret.r_fTimeToDeploySecs") == 0) {
					FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Turret.r_fCurrentDeployTime") == 0) {
					FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgPawn_Turret.r_fDeployMaxHealthPCT") == 0) {
					FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgPawn_VanityPet.r_nSpawningItemId") == 0) {
					IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgPlayerController.r_WatchOtherPlayer") == 0) {
					ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPlayerController.r_bEDDebugEffects") == 0) {
					BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPlayerController.r_bGMInvisible") == 0) {
					BoolProperty_TgGame_TgPlayerController_r_bGMInvisible = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPlayerController.r_bIsHackingABot") == 0) {
					BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPlayerController.r_bLockYawRotation") == 0) {
					BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPlayerController.r_bRove") == 0) {
					BoolProperty_TgGame_TgPlayerController_r_bRove = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgProj_Grapple.r_vTargetLocation") == 0) {
					StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgProj_Missile.r_aSeeking") == 0) {
					ObjectProperty_TgGame_TgProj_Missile_r_aSeeking = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgProj_Missile.r_vTargetWorldLocation") == 0) {
					StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgProj_Missile.r_nNumBounces") == 0) {
					IntProperty_TgGame_TgProj_Missile_r_nNumBounces = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgProj_Rocket.FlockIndex") == 0) {
					ByteProperty_TgGame_TgProj_Rocket_FlockIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgProj_Rocket.bCurl") == 0) {
					BoolProperty_TgGame_TgProj_Rocket_bCurl = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgProjectile.r_Owner") == 0) {
					ObjectProperty_TgGame_TgProjectile_r_Owner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgProjectile.r_fAccelRate") == 0) {
					FloatProperty_TgGame_TgProjectile_r_fAccelRate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgProjectile.r_fDuration") == 0) {
					FloatProperty_TgGame_TgProjectile_r_fDuration = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgProjectile.r_fRange") == 0) {
					FloatProperty_TgGame_TgProjectile_r_fRange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgProjectile.r_nOwnerFireModeId") == 0) {
					IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgProjectile.r_nProjectileId") == 0) {
					IntProperty_TgGame_TgProjectile_r_nProjectileId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgProjectile.r_vSpawnLocation") == 0) {
					StructProperty_TgGame_TgProjectile_r_vSpawnLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Beacon.r_bDeployed") == 0) {
					BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Beacon.r_vLoc") == 0) {
					StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgRepInfo_Beacon.r_nName") == 0) {
					StrProperty_TgGame_TgRepInfo_Beacon_r_nName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Deployable.r_InstigatorInfo") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Deployable.r_TaskforceInfo") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Deployable.r_bOwnedByTaskforce") == 0) {
					BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Deployable.r_nHealthCurrent") == 0) {
					IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Deployable.r_DeployableOwner") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgRepInfo_Deployable.r_fDeployMaxHealthPCT") == 0) {
					FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Deployable.r_nDeployableId") == 0) {
					IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Deployable.r_nHealthMaximum") == 0) {
					IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Deployable.r_nUniqueDeployableId") == 0) {
					IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Game.r_MiniMapInfo") == 0) {
					StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bActiveCombat") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bAllowBuildMorale") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bAllowPlayerRelease") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bDefenseAlarm") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bInOverTime") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsTutorialMap") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgRepInfo_Game.r_fGameSpeedModifier") == 0) {
					FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgRepInfo_Game.r_fMissionRemainingTime") == 0) {
					FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgRepInfo_Game.r_fServerTimeLastUpdate") == 0) {
					FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nMaxRoundNumber") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nMissionTimerStateChange") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nMissionTimerStateChange") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nRaidAttackerRespawnBonus") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nRaidDefenderRespawnBonus") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nReleaseDelay") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nRoundNumber") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nSecsToAutoReleaseAttackers") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nSecsToAutoReleaseDefenders") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgRepInfo_Game.r_GameType") == 0) {
					ByteProperty_TgGame_TgRepInfo_Game_r_GameType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Game.r_MapLogoResIds") == 0) {
					StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Game.r_Objectives") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsArena") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsMatch") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsMission") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsPVP") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsRaid") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsTerritoryMap") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Game.r_bIsTraining") == 0) {
					BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nAutoKickTimeout") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nPointsToWin") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Game.r_nVictoryBonusLives") == 0) {
					IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_GameOpenWorld.r_GameTickets") == 0) {
					IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Player.r_ApproxLocation") == 0) {
					StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Player.r_CustomCharacterAssembly") == 0) {
					StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Player.r_EquipDeviceInfo") == 0) {
					StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Player.r_MasterPrep") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Player.r_PawnOwner") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_Scores") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_Scores = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_Player.r_TaskForce") == 0) {
					ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_Player.r_bDropped") == 0) {
					BoolProperty_TgGame_TgRepInfo_Player_r_bDropped = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_eBonusType") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_eBonusType = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nCharacterId") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nHealthCurrent") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nHealthMaximum") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nLevel") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nLevel = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nProfileId") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nProfileId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_Player.r_nTitleMsgId") == 0) {
					IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgRepInfo_Player.r_sAgencyName") == 0) {
					StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgRepInfo_Player.r_sAllianceName") == 0) {
					StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StrProperty TgGame.TgRepInfo_Player.r_sOrigPlayerName") == 0) {
					StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "StructProperty TgGame.TgRepInfo_Player.r_DeviceStats") == 0) {
					StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_TaskForce.r_BeaconManager") == 0) {
					ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_TaskForce.r_CurrActiveObjective") == 0) {
					ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgRepInfo_TaskForce.r_ObjectiveAssignment") == 0) {
					ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgRepInfo_TaskForce.r_bBotOwned") == 0) {
					BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgRepInfo_TaskForce.r_eCoalition") == 0) {
					ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_TaskForce.r_nCurrentPointCount") == 0) {
					IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_TaskForce.r_nLeaderCharId") == 0) {
					IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgRepInfo_TaskForce.r_nLookingForMembers") == 0) {
					FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_TaskForce.r_nNumDeaths") == 0) {
					IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgRepInfo_TaskForce.r_nTaskForce") == 0) {
					ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgRepInfo_TaskForce.r_nTeamId") == 0) {
					IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgSkydiveTarget.m_LandRadius") == 0) {
					FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgSkydivingVolume.r_PawnGravityModifier") == 0) {
					FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgSkydivingVolume.r_PawnLaunchForce") == 0) {
					FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgSkydivingVolume.r_PawnUpForce") == 0) {
					FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgSkydivingVolume.r_SkydiveTarget") == 0) {
					ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgTeamBeaconManager.r_BeaconDestroyed") == 0) {
					IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty TgGame.TgTeamBeaconManager.r_BeaconDestroyed") == 0) {
					IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgTeamBeaconManager.r_BeaconHolder") == 0) {
					ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgTeamBeaconManager.r_BeaconInfo") == 0) {
					ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgTeamBeaconManager.r_BeaconStatus") == 0) {
					ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgTeamBeaconManager.r_TaskForce") == 0) {
					ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgTimerManager.r_byEventQueIndex") == 0) {
					ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgTimerManager.r_byEventQueIndex") == 0) {
					ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgTimerManager.r_fRemaining") == 0) {
					FloatProperty_TgGame_TgTimerManager_r_fRemaining = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "FloatProperty TgGame.TgTimerManager.r_fStartTime") == 0) {
					FloatProperty_TgGame_TgTimerManager_r_fStartTime = (UProperty*)obj;
					continue;
				}
			}
		}

	}

	AActor* actor = (AActor*)thisxx;
	AActor* recent = (AActor*)param_1;
	int repindex = 0;

	if (ObjectIsA(actor, "Class Engine.Actor")) {
		if (((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->bReplicateMovement) && actor->RemoteRole == 1) {
			{
			bool &initialFlag = ClassProperty_Actor_BasedActors_BaseClass_initial[(int)actor];
			if (ClassProperty_Actor_BasedActors_BaseClass != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Base, ((AActor*)actor)->Base, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Actor_BasedActors_BaseClass + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->bReplicateMovement) && (actor->RemoteRole == 1) && (actor->bNetInitial || actor->bNetDirty) || actor->bUpdateSimulatedPosition) {
			{
			bool &initialFlag = ObjectProperty_Actor_PhysicsVolumeChange_NewVolume_initial[(int)actor];
			if (ObjectProperty_Actor_PhysicsVolumeChange_NewVolume != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Physics, ((AActor*)actor)->Physics, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Actor_PhysicsVolumeChange_NewVolume + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_Actor_Velocity_initial[(int)actor];
			if (StructProperty_Engine_Actor_Velocity != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Velocity, ((AActor*)actor)->Velocity, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_Actor_Velocity + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_Engine_Actor_RemoteRole_initial[(int)actor];
			if (ByteProperty_Engine_Actor_RemoteRole != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->RemoteRole, ((AActor*)actor)->RemoteRole, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_Actor_RemoteRole + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_Engine_Actor_Role_initial[(int)actor];
			if (ByteProperty_Engine_Actor_Role != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Role, ((AActor*)actor)->Role, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_Actor_Role + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bNetOwner_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bNetOwner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bNetOwner, ((AActor*)actor)->bNetOwner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bNetOwner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bTearOff_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bTearOff != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bTearOff, ((AActor*)actor)->bTearOff, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bTearOff + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = FloatProperty_Engine_Actor_DrawScale_initial[(int)actor];
			if (FloatProperty_Engine_Actor_DrawScale != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->DrawScale, ((AActor*)actor)->DrawScale, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Actor_DrawScale + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_Engine_Actor_ReplicatedCollisionType_initial[(int)actor];
			if (ByteProperty_Engine_Actor_ReplicatedCollisionType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->ReplicatedCollisionType, ((AActor*)actor)->ReplicatedCollisionType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_Actor_ReplicatedCollisionType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bCollideActors_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bCollideActors != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bCollideActors, ((AActor*)actor)->bCollideActors, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bCollideActors + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bCollideWorld_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bCollideWorld != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bCollideWorld, ((AActor*)actor)->bCollideWorld, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bCollideWorld + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) && actor->bCollideActors || actor->bCollideWorld) {
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bBlockActors_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bBlockActors != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bBlockActors, ((AActor*)actor)->bBlockActors, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bBlockActors + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Actor_bProjTarget_initial[(int)actor];
			if (BoolProperty_Engine_Actor_bProjTarget != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->bProjTarget, ((AActor*)actor)->bProjTarget, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Actor_bProjTarget + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) && actor->bReplicateInstigator) {
			{
			bool &initialFlag = ObjectProperty_GameFramework_GameExplosionActor_InstigatorController_initial[(int)actor];
			if (ObjectProperty_GameFramework_GameExplosionActor_InstigatorController != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Instigator, ((AActor*)actor)->Instigator, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_GameFramework_GameExplosionActor_InstigatorController + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((actor->bNetOwner && !actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ObjectProperty_Engine_Actor_Owner_initial[(int)actor];
			if (ObjectProperty_Engine_Actor_Owner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AActor*)recent)->Owner, ((AActor*)actor)->Owner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Actor_Owner + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.AmbientSoundSimpleToggleable")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying_initial[(int)actor];
			if (BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AAmbientSoundSimpleToggleable*)recent)->bCurrentlyPlaying, ((AAmbientSoundSimpleToggleable*)actor)->bCurrentlyPlaying, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.CameraActor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_Engine_CameraActor_AspectRatio_initial[(int)actor];
			if (FloatProperty_Engine_CameraActor_AspectRatio != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACameraActor*)recent)->AspectRatio, ((ACameraActor*)actor)->AspectRatio, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_CameraActor_AspectRatio + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_CameraActor_FOVAngle_initial[(int)actor];
			if (FloatProperty_Engine_CameraActor_FOVAngle != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACameraActor*)recent)->FOVAngle, ((ACameraActor*)actor)->FOVAngle, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_CameraActor_FOVAngle + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Controller")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest_initial[(int)actor];
			if (BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AController*)recent)->Pawn, ((AController*)actor)->Pawn, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgPlayerController_PawnPerformanceTest_bEnablePerfTest + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_Controller_PlayerReplicationInfo_initial[(int)actor];
			if (ObjectProperty_Engine_Controller_PlayerReplicationInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AController*)recent)->PlayerReplicationInfo, ((AController*)actor)->PlayerReplicationInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Controller_PlayerReplicationInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.CrowdAttractor")) {
		if (actor->bNoDelete) {
			{
			bool &initialFlag = BoolProperty_Engine_CrowdAttractor_bAttractorEnabled_initial[(int)actor];
			if (BoolProperty_Engine_CrowdAttractor_bAttractorEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACrowdAttractor*)recent)->bAttractorEnabled, ((ACrowdAttractor*)actor)->bAttractorEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_CrowdAttractor_bAttractorEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.CrowdReplicationActor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_CrowdReplicationActor_DestroyAllCount_initial[(int)actor];
			if (IntProperty_Engine_CrowdReplicationActor_DestroyAllCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACrowdReplicationActor*)recent)->DestroyAllCount, ((ACrowdReplicationActor*)actor)->DestroyAllCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_CrowdReplicationActor_DestroyAllCount + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_CrowdReplicationActor_Spawner_initial[(int)actor];
			if (ObjectProperty_Engine_CrowdReplicationActor_Spawner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACrowdReplicationActor*)recent)->Spawner, ((ACrowdReplicationActor*)actor)->Spawner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_CrowdReplicationActor_Spawner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_CrowdReplicationActor_bSpawningActive_initial[(int)actor];
			if (BoolProperty_Engine_CrowdReplicationActor_bSpawningActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ACrowdReplicationActor*)recent)->bSpawningActive, ((ACrowdReplicationActor*)actor)->bSpawningActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_CrowdReplicationActor_bSpawningActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.DroppedPickup")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ClassProperty_Engine_DroppedPickup_InventoryClass_initial[(int)actor];
			if (ClassProperty_Engine_DroppedPickup_InventoryClass != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADroppedPickup*)recent)->InventoryClass, ((ADroppedPickup*)actor)->InventoryClass, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Engine_DroppedPickup_InventoryClass + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_DroppedPickup_bFadeOut_initial[(int)actor];
			if (BoolProperty_Engine_DroppedPickup_bFadeOut != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADroppedPickup*)recent)->bFadeOut, ((ADroppedPickup*)actor)->bFadeOut, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_DroppedPickup_bFadeOut + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.DynamicSMActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial_initial[(int)actor];
			if (ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->ReplicatedMaterial, ((ADynamicSMActor*)actor)->ReplicatedMaterial, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh_initial[(int)actor];
			if (ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->ReplicatedMesh, ((ADynamicSMActor*)actor)->ReplicatedMesh, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation_initial[(int)actor];
			if (StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->ReplicatedMeshRotation, ((ADynamicSMActor*)actor)->ReplicatedMeshRotation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D_initial[(int)actor];
			if (StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->ReplicatedMeshScale3D, ((ADynamicSMActor*)actor)->ReplicatedMeshScale3D, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation_initial[(int)actor];
			if (StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->ReplicatedMeshTranslation, ((ADynamicSMActor*)actor)->ReplicatedMeshTranslation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_DynamicSMActor_bForceStaticDecals_initial[(int)actor];
			if (BoolProperty_Engine_DynamicSMActor_bForceStaticDecals != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ADynamicSMActor*)recent)->bForceStaticDecals, ((ADynamicSMActor*)actor)->bForceStaticDecals, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_DynamicSMActor_bForceStaticDecals + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Emitter")) {
		if (actor->bNoDelete) {
			{
			bool &initialFlag = BoolProperty_Engine_Emitter_bCurrentlyActive_initial[(int)actor];
			if (BoolProperty_Engine_Emitter_bCurrentlyActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AEmitter*)recent)->bCurrentlyActive, ((AEmitter*)actor)->bCurrentlyActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Emitter_bCurrentlyActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.EmitterSpawnable")) {
		if ((actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate_initial[(int)actor];
			if (ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AEmitterSpawnable*)recent)->ParticleTemplate, ((AEmitterSpawnable*)actor)->ParticleTemplate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.FluidInfluenceActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_Engine_FluidInfluenceActor_bActive_initial[(int)actor];
			if (BoolProperty_Engine_FluidInfluenceActor_bActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AFluidInfluenceActor*)recent)->bActive, ((AFluidInfluenceActor*)actor)->bActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_FluidInfluenceActor_bActive + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_FluidInfluenceActor_bToggled_initial[(int)actor];
			if (BoolProperty_Engine_FluidInfluenceActor_bToggled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AFluidInfluenceActor*)recent)->bToggled, ((AFluidInfluenceActor*)actor)->bToggled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_FluidInfluenceActor_bToggled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.FogVolumeDensityInfo")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_FogVolumeDensityInfo_bEnabled_initial[(int)actor];
			if (BoolProperty_Engine_FogVolumeDensityInfo_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AFogVolumeDensityInfo*)recent)->bEnabled, ((AFogVolumeDensityInfo*)actor)->bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_FogVolumeDensityInfo_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.GameReplicationInfo")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_MatchID_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_MatchID != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->MatchID, ((AGameReplicationInfo*)actor)->MatchID, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_MatchID + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_GameReplicationInfo_Winner_initial[(int)actor];
			if (ObjectProperty_Engine_GameReplicationInfo_Winner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->Winner, ((AGameReplicationInfo*)actor)->Winner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_GameReplicationInfo_Winner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun_initial[(int)actor];
			if (BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->bMatchHasBegun, ((AGameReplicationInfo*)actor)->bMatchHasBegun, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_GameReplicationInfo_bMatchIsOver_initial[(int)actor];
			if (BoolProperty_Engine_GameReplicationInfo_bMatchIsOver != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->bMatchIsOver, ((AGameReplicationInfo*)actor)->bMatchIsOver, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_GameReplicationInfo_bMatchIsOver + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_GameReplicationInfo_bStopCountDown_initial[(int)actor];
			if (BoolProperty_Engine_GameReplicationInfo_bStopCountDown != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->bStopCountDown, ((AGameReplicationInfo*)actor)->bStopCountDown, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_GameReplicationInfo_bStopCountDown + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((!(actor->bNetInitial || actor->bNetDirty) && (actor->bNetDirty || actor->bNetInitial)) && actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_RemainingMinute_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_RemainingMinute != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->RemainingMinute, ((AGameReplicationInfo*)actor)->RemainingMinute, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_RemainingMinute + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = StrProperty_Engine_GameReplicationInfo_AdminEmail_initial[(int)actor];
			if (StrProperty_Engine_GameReplicationInfo_AdminEmail != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->AdminEmail, ((AGameReplicationInfo*)actor)->AdminEmail, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_GameReplicationInfo_AdminEmail + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_GameReplicationInfo_AdminName_initial[(int)actor];
			if (StrProperty_Engine_GameReplicationInfo_AdminName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->AdminName, ((AGameReplicationInfo*)actor)->AdminName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_GameReplicationInfo_AdminName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_ElapsedTime_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_ElapsedTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->ElapsedTime, ((AGameReplicationInfo*)actor)->ElapsedTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_ElapsedTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ClassProperty_Engine_GameReplicationInfo_GameClass_initial[(int)actor];
			if (ClassProperty_Engine_GameReplicationInfo_GameClass != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->GameClass, ((AGameReplicationInfo*)actor)->GameClass, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Engine_GameReplicationInfo_GameClass + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_GoalScore_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_GoalScore != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->GoalScore, ((AGameReplicationInfo*)actor)->GoalScore, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_GoalScore + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_MaxLives_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_MaxLives != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->MaxLives, ((AGameReplicationInfo*)actor)->MaxLives, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_MaxLives + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_GameReplicationInfo_MessageOfTheDay_initial[(int)actor];
			if (StrProperty_Engine_GameReplicationInfo_MessageOfTheDay != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->MessageOfTheDay, ((AGameReplicationInfo*)actor)->MessageOfTheDay, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_GameReplicationInfo_MessageOfTheDay + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_RemainingTime_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_RemainingTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->RemainingTime, ((AGameReplicationInfo*)actor)->RemainingTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_RemainingTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_GameReplicationInfo_ServerName_initial[(int)actor];
			if (StrProperty_Engine_GameReplicationInfo_ServerName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->ServerName, ((AGameReplicationInfo*)actor)->ServerName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_GameReplicationInfo_ServerName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_ServerRegion_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_ServerRegion != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->ServerRegion, ((AGameReplicationInfo*)actor)->ServerRegion, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_ServerRegion + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_GameReplicationInfo_ShortName_initial[(int)actor];
			if (StrProperty_Engine_GameReplicationInfo_ShortName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->ShortName, ((AGameReplicationInfo*)actor)->ShortName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_GameReplicationInfo_ShortName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_GameReplicationInfo_TimeLimit_initial[(int)actor];
			if (IntProperty_Engine_GameReplicationInfo_TimeLimit != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->TimeLimit, ((AGameReplicationInfo*)actor)->TimeLimit, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_GameReplicationInfo_TimeLimit + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_GameReplicationInfo_bIsArbitrated_initial[(int)actor];
			if (BoolProperty_Engine_GameReplicationInfo_bIsArbitrated != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->bIsArbitrated, ((AGameReplicationInfo*)actor)->bIsArbitrated, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_GameReplicationInfo_bIsArbitrated + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_GameReplicationInfo_bTrackStats_initial[(int)actor];
			if (BoolProperty_Engine_GameReplicationInfo_bTrackStats != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AGameReplicationInfo*)recent)->bTrackStats, ((AGameReplicationInfo*)actor)->bTrackStats, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_GameReplicationInfo_bTrackStats + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.HeightFog")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_HeightFog_bEnabled_initial[(int)actor];
			if (BoolProperty_Engine_HeightFog_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AHeightFog*)recent)->bEnabled, ((AHeightFog*)actor)->bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_HeightFog_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Inventory")) {
		if (((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) && actor->bNetOwner) {
			{
			bool &initialFlag = ObjectProperty_Engine_Inventory_InvManager_initial[(int)actor];
			if (ObjectProperty_Engine_Inventory_InvManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AInventory*)recent)->InvManager, ((AInventory*)actor)->InvManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Inventory_InvManager + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList_initial[(int)actor];
			if (ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AInventory*)recent)->Inventory, ((AInventory*)actor)->Inventory, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_SeqAct_GiveInventory_InventoryList_InventoryList + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.InventoryManager")) {
		if ((((!actor->bSkipActorPropertyReplication || (actor->bNetInitial || actor->bNetDirty)) && actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) && actor->bNetOwner) {
			{
			bool &initialFlag = ObjectProperty_Engine_InventoryManager_InventoryChain_initial[(int)actor];
			if (ObjectProperty_Engine_InventoryManager_InventoryChain != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AInventoryManager*)recent)->InventoryChain, ((AInventoryManager*)actor)->InventoryChain, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_InventoryManager_InventoryChain + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.KActor")) {
		if (!((AKActor*)actor)->bNeedsRBStateReplication && actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_Engine_KActor_RBState_initial[(int)actor];
			if (StructProperty_Engine_KActor_RBState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AKActor*)recent)->RBState, ((AKActor*)actor)->RBState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_KActor_RBState + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_Engine_KActor_ReplicatedDrawScale3D_initial[(int)actor];
			if (StructProperty_Engine_KActor_ReplicatedDrawScale3D != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AKActor*)recent)->ReplicatedDrawScale3D, ((AKActor*)actor)->ReplicatedDrawScale3D, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_KActor_ReplicatedDrawScale3D + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_KActor_bWakeOnLevelStart_initial[(int)actor];
			if (BoolProperty_Engine_KActor_bWakeOnLevelStart != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AKActor*)recent)->bWakeOnLevelStart, ((AKActor*)actor)->bWakeOnLevelStart, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_KActor_bWakeOnLevelStart + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.KAsset")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_Engine_KAsset_ReplicatedMesh_initial[(int)actor];
			if (ObjectProperty_Engine_KAsset_ReplicatedMesh != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AKAsset*)recent)->ReplicatedMesh, ((AKAsset*)actor)->ReplicatedMesh, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_KAsset_ReplicatedMesh + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_KAsset_ReplicatedPhysAsset_initial[(int)actor];
			if (ObjectProperty_Engine_KAsset_ReplicatedPhysAsset != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AKAsset*)recent)->ReplicatedPhysAsset, ((AKAsset*)actor)->ReplicatedPhysAsset, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_KAsset_ReplicatedPhysAsset + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.LensFlareSource")) {
		if (actor->bNoDelete) {
			{
			bool &initialFlag = BoolProperty_Engine_LensFlareSource_bCurrentlyActive_initial[(int)actor];
			if (BoolProperty_Engine_LensFlareSource_bCurrentlyActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ALensFlareSource*)recent)->bCurrentlyActive, ((ALensFlareSource*)actor)->bCurrentlyActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_LensFlareSource_bCurrentlyActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Light")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_Light_bEnabled_initial[(int)actor];
			if (BoolProperty_Engine_Light_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ALight*)recent)->bEnabled, ((ALight*)actor)->bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Light_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.MatineeActor")) {
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_Engine_MatineeActor_InterpAction_initial[(int)actor];
			if (ObjectProperty_Engine_MatineeActor_InterpAction != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->InterpAction, ((AMatineeActor*)actor)->InterpAction, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_MatineeActor_InterpAction + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_Engine_MatineeActor_PlayRate_initial[(int)actor];
			if (FloatProperty_Engine_MatineeActor_PlayRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->PlayRate, ((AMatineeActor*)actor)->PlayRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_MatineeActor_PlayRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_MatineeActor_Position_initial[(int)actor];
			if (FloatProperty_Engine_MatineeActor_Position != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->Position, ((AMatineeActor*)actor)->Position, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_MatineeActor_Position + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_MatineeActor_bIsPlaying_initial[(int)actor];
			if (BoolProperty_Engine_MatineeActor_bIsPlaying != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->bIsPlaying, ((AMatineeActor*)actor)->bIsPlaying, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_MatineeActor_bIsPlaying + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_MatineeActor_bPaused_initial[(int)actor];
			if (BoolProperty_Engine_MatineeActor_bPaused != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->bPaused, ((AMatineeActor*)actor)->bPaused, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_MatineeActor_bPaused + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_MatineeActor_bReversePlayback_initial[(int)actor];
			if (BoolProperty_Engine_MatineeActor_bReversePlayback != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AMatineeActor*)recent)->bReversePlayback, ((AMatineeActor*)actor)->bReversePlayback, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_MatineeActor_bReversePlayback + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.NxForceField")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_Engine_NxForceField_bForceActive_initial[(int)actor];
			if (BoolProperty_Engine_NxForceField_bForceActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ANxForceField*)recent)->bForceActive, ((ANxForceField*)actor)->bForceActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_NxForceField_bForceActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Pawn")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_Engine_Pawn_DrivenVehicle_initial[(int)actor];
			if (ObjectProperty_Engine_Pawn_DrivenVehicle != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->DrivenVehicle, ((APawn*)actor)->DrivenVehicle, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Pawn_DrivenVehicle + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Pawn_FlashLocationUpdated_bViaReplication_initial[(int)actor];
			if (BoolProperty_Pawn_FlashLocationUpdated_bViaReplication != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->FlashLocation, ((APawn*)actor)->FlashLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Pawn_FlashLocationUpdated_bViaReplication + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_Pawn_HealthMax_initial[(int)actor];
			if (IntProperty_Engine_Pawn_HealthMax != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->Health, ((APawn*)actor)->Health, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_Pawn_HealthMax + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ClassProperty_Engine_Pawn_HitDamageType_initial[(int)actor];
			if (ClassProperty_Engine_Pawn_HitDamageType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->HitDamageType, ((APawn*)actor)->HitDamageType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Engine_Pawn_HitDamageType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_Pawn_PlayerReplicationInfo_initial[(int)actor];
			if (ObjectProperty_Engine_Pawn_PlayerReplicationInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->PlayerReplicationInfo, ((APawn*)actor)->PlayerReplicationInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Pawn_PlayerReplicationInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_Pawn_TakeHitLocation_initial[(int)actor];
			if (StructProperty_Engine_Pawn_TakeHitLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->TakeHitLocation, ((APawn*)actor)->TakeHitLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_Pawn_TakeHitLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Pawn_bIsWalking_initial[(int)actor];
			if (BoolProperty_Engine_Pawn_bIsWalking != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->bIsWalking, ((APawn*)actor)->bIsWalking, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Pawn_bIsWalking + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Pawn_bSimulateGravity_initial[(int)actor];
			if (BoolProperty_Engine_Pawn_bSimulateGravity != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->bSimulateGravity, ((APawn*)actor)->bSimulateGravity, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Pawn_bSimulateGravity + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((actor->bNetDirty || actor->bNetInitial) && actor->bNetOwner) && actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_AccelRate_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_AccelRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->AccelRate, ((APawn*)actor)->AccelRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_AccelRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_AirControl_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_AirControl != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->AirControl, ((APawn*)actor)->AirControl, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_AirControl + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_AirSpeed_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_AirSpeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->AirSpeed, ((APawn*)actor)->AirSpeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_AirSpeed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ClassProperty_Engine_Pawn_ControllerClass_initial[(int)actor];
			if (ClassProperty_Engine_Pawn_ControllerClass != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->Controller, ((APawn*)actor)->Controller, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Engine_Pawn_ControllerClass + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_GroundSpeed_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_GroundSpeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->GroundSpeed, ((APawn*)actor)->GroundSpeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_GroundSpeed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_Pawn_InvManager_initial[(int)actor];
			if (ObjectProperty_Engine_Pawn_InvManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->InvManager, ((APawn*)actor)->InvManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_Pawn_InvManager + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_JumpZ_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_JumpZ != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->JumpZ, ((APawn*)actor)->JumpZ, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_JumpZ + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Pawn_WaterSpeed_initial[(int)actor];
			if (FloatProperty_Engine_Pawn_WaterSpeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->WaterSpeed, ((APawn*)actor)->WaterSpeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Pawn_WaterSpeed + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((actor->bNetDirty || actor->bNetInitial) && !actor->bNetOwner || FALSE) && actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Pawn_FiringModeUpdated_bViaReplication_initial[(int)actor];
			if (BoolProperty_Pawn_FiringModeUpdated_bViaReplication != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->FiringMode, ((APawn*)actor)->FiringMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Pawn_FiringModeUpdated_bViaReplication + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Pawn_FlashCountUpdated_bViaReplication_initial[(int)actor];
			if (BoolProperty_Pawn_FlashCountUpdated_bViaReplication != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->FlashCount, ((APawn*)actor)->FlashCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Pawn_FlashCountUpdated_bViaReplication + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Pawn_bIsCrouched_initial[(int)actor];
			if (BoolProperty_Engine_Pawn_bIsCrouched != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->bIsCrouched, ((APawn*)actor)->bIsCrouched, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Pawn_bIsCrouched + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bTearOff && (actor->bNetDirty || actor->bNetInitial)) && actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_Engine_Pawn_TearOffMomentum_initial[(int)actor];
			if (StructProperty_Engine_Pawn_TearOffMomentum != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->TearOffMomentum, ((APawn*)actor)->TearOffMomentum, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_Pawn_TearOffMomentum + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((!actor->bNetOwner || FALSE) && actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_Engine_Pawn_RemoteViewPitch_initial[(int)actor];
			if (ByteProperty_Engine_Pawn_RemoteViewPitch != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APawn*)recent)->RemoteViewPitch, ((APawn*)actor)->RemoteViewPitch, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_Pawn_RemoteViewPitch + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.PhysXEmitterSpawnable")) {
		if ((actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate_initial[(int)actor];
			if (ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APhysXEmitterSpawnable*)recent)->ParticleTemplate, ((APhysXEmitterSpawnable*)actor)->ParticleTemplate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.PickupFactory")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_PickupFactory_bPickupHidden_initial[(int)actor];
			if (BoolProperty_Engine_PickupFactory_bPickupHidden != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APickupFactory*)recent)->bPickupHidden, ((APickupFactory*)actor)->bPickupHidden, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PickupFactory_bPickupHidden + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = ClassProperty_Engine_PickupFactory_InventoryType_initial[(int)actor];
			if (ClassProperty_Engine_PickupFactory_InventoryType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APickupFactory*)recent)->InventoryType, ((APickupFactory*)actor)->InventoryType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_Engine_PickupFactory_InventoryType + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.PlayerController")) {
		if (((actor->bNetOwner && actor->Role == 3) && ((APlayerController*)actor)->ViewTarget != ((APlayerController*)actor)->Pawn) && ((APlayerController*)actor)->ViewTarget != NULL) {
			{
			bool &initialFlag = FloatProperty_Engine_PlayerController_TargetEyeHeight_initial[(int)actor];
			if (FloatProperty_Engine_PlayerController_TargetEyeHeight != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerController*)recent)->TargetEyeHeight, ((APlayerController*)actor)->TargetEyeHeight, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_PlayerController_TargetEyeHeight + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_PlayerController_TargetViewRotation_initial[(int)actor];
			if (StructProperty_Engine_PlayerController_TargetViewRotation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerController*)recent)->TargetViewRotation, ((APlayerController*)actor)->TargetViewRotation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_PlayerController_TargetViewRotation + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.PlayerReplicationInfo")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_Engine_PlayerReplicationInfo_Deaths_initial[(int)actor];
			if (FloatProperty_Engine_PlayerReplicationInfo_Deaths != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->Deaths, ((APlayerReplicationInfo*)actor)->Deaths, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_PlayerReplicationInfo_Deaths + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_PlayerReplicationInfo_PlayerAlias_initial[(int)actor];
			if (StrProperty_Engine_PlayerReplicationInfo_PlayerAlias != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PlayerAlias, ((APlayerReplicationInfo*)actor)->PlayerAlias, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_PlayerReplicationInfo_PlayerAlias + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint_initial[(int)actor];
			if (ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PlayerLocationHint, ((APlayerReplicationInfo*)actor)->PlayerLocationHint, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_PlayerReplicationInfo_PlayerName_initial[(int)actor];
			if (StrProperty_Engine_PlayerReplicationInfo_PlayerName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PlayerName, ((APlayerReplicationInfo*)actor)->PlayerName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_PlayerReplicationInfo_PlayerName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_PlayerReplicationInfo_PlayerSkill_initial[(int)actor];
			if (IntProperty_Engine_PlayerReplicationInfo_PlayerSkill != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PlayerSkill, ((APlayerReplicationInfo*)actor)->PlayerSkill, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_PlayerReplicationInfo_PlayerSkill + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_PlayerReplicationInfo_Score_initial[(int)actor];
			if (FloatProperty_Engine_PlayerReplicationInfo_Score != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->Score, ((APlayerReplicationInfo*)actor)->Score, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_PlayerReplicationInfo_Score + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_PlayerReplicationInfo_StartTime_initial[(int)actor];
			if (IntProperty_Engine_PlayerReplicationInfo_StartTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->StartTime, ((APlayerReplicationInfo*)actor)->StartTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_PlayerReplicationInfo_StartTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_PlayerReplicationInfo_Team_initial[(int)actor];
			if (ObjectProperty_Engine_PlayerReplicationInfo_Team != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->Team, ((APlayerReplicationInfo*)actor)->Team, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_PlayerReplicationInfo_Team + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_PlayerReplicationInfo_UniqueId_initial[(int)actor];
			if (StructProperty_Engine_PlayerReplicationInfo_UniqueId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->UniqueId, ((APlayerReplicationInfo*)actor)->UniqueId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_PlayerReplicationInfo_UniqueId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bAdmin_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bAdmin != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bAdmin, ((APlayerReplicationInfo*)actor)->bAdmin, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bAdmin + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bHasFlag_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bHasFlag != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bHasFlag, ((APlayerReplicationInfo*)actor)->bHasFlag, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bHasFlag + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bIsFemale_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bIsFemale != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bIsFemale, ((APlayerReplicationInfo*)actor)->bIsFemale, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bIsFemale + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bIsSpectator, ((APlayerReplicationInfo*)actor)->bIsSpectator, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bOnlySpectator, ((APlayerReplicationInfo*)actor)->bOnlySpectator, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bOutOfLives, ((APlayerReplicationInfo*)actor)->bOutOfLives, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bReadyToPlay, ((APlayerReplicationInfo*)actor)->bReadyToPlay, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bWaitingPlayer, ((APlayerReplicationInfo*)actor)->bWaitingPlayer, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) && !actor->bNetOwner) {
			{
			bool &initialFlag = ByteProperty_Engine_PlayerReplicationInfo_PacketLoss_initial[(int)actor];
			if (ByteProperty_Engine_PlayerReplicationInfo_PacketLoss != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PacketLoss, ((APlayerReplicationInfo*)actor)->PacketLoss, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_PlayerReplicationInfo_PacketLoss + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_Engine_PlayerReplicationInfo_Ping_initial[(int)actor];
			if (ByteProperty_Engine_PlayerReplicationInfo_Ping != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->Ping, ((APlayerReplicationInfo*)actor)->Ping, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_PlayerReplicationInfo_Ping + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex_initial[(int)actor];
			if (IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->SplitscreenIndex, ((APlayerReplicationInfo*)actor)->SplitscreenIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_PlayerReplicationInfo_PlayerID_initial[(int)actor];
			if (IntProperty_Engine_PlayerReplicationInfo_PlayerID != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->PlayerID, ((APlayerReplicationInfo*)actor)->PlayerID, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_PlayerReplicationInfo_PlayerID + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bBot_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bBot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bBot, ((APlayerReplicationInfo*)actor)->bBot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bBot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_PlayerReplicationInfo_bIsInactive_initial[(int)actor];
			if (BoolProperty_Engine_PlayerReplicationInfo_bIsInactive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APlayerReplicationInfo*)recent)->bIsInactive, ((APlayerReplicationInfo*)actor)->bIsInactive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PlayerReplicationInfo_bIsInactive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.PostProcessVolume")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_Engine_PostProcessVolume_bEnabled_initial[(int)actor];
			if (BoolProperty_Engine_PostProcessVolume_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((APostProcessVolume*)recent)->bEnabled, ((APostProcessVolume*)actor)->bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_PostProcessVolume_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Projectile")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = FloatProperty_Engine_Projectile_MaxSpeed_initial[(int)actor];
			if (FloatProperty_Engine_Projectile_MaxSpeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AProjectile*)recent)->MaxSpeed, ((AProjectile*)actor)->MaxSpeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Projectile_MaxSpeed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_Projectile_Speed_initial[(int)actor];
			if (FloatProperty_Engine_Projectile_Speed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AProjectile*)recent)->Speed, ((AProjectile*)actor)->Speed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_Projectile_Speed + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.RB_CylindricalForceActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_Engine_RB_CylindricalForceActor_bForceActive_initial[(int)actor];
			if (BoolProperty_Engine_RB_CylindricalForceActor_bForceActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ARB_CylindricalForceActor*)recent)->bForceActive, ((ARB_CylindricalForceActor*)actor)->bForceActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_RB_CylindricalForceActor_bForceActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.RB_LineImpulseActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount_initial[(int)actor];
			if (ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ARB_LineImpulseActor*)recent)->ImpulseCount, ((ARB_LineImpulseActor*)actor)->ImpulseCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.RB_RadialForceActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_Engine_RB_RadialForceActor_bForceActive_initial[(int)actor];
			if (BoolProperty_Engine_RB_RadialForceActor_bForceActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ARB_RadialForceActor*)recent)->bForceActive, ((ARB_RadialForceActor*)actor)->bForceActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_RB_RadialForceActor_bForceActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.RB_RadialImpulseActor")) {
		if ((actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount_initial[(int)actor];
			if (ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ARB_RadialImpulseActor*)recent)->ImpulseCount, ((ARB_RadialImpulseActor*)actor)->ImpulseCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.SVehicle")) {
		if (actor->Physics == 10) {
			{
			bool &initialFlag = FloatProperty_Engine_SVehicle_MaxSpeed_initial[(int)actor];
			if (FloatProperty_Engine_SVehicle_MaxSpeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ASVehicle*)recent)->MaxSpeed, ((ASVehicle*)actor)->MaxSpeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_SVehicle_MaxSpeed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_SVehicle_VState_initial[(int)actor];
			if (StructProperty_Engine_SVehicle_VState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ASVehicle*)recent)->VState, ((ASVehicle*)actor)->VState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_SVehicle_VState + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.SkeletalMeshActor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial_initial[(int)actor];
			if (ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ASkeletalMeshActor*)recent)->ReplicatedMaterial, ((ASkeletalMeshActor*)actor)->ReplicatedMaterial, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh_initial[(int)actor];
			if (ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ASkeletalMeshActor*)recent)->ReplicatedMesh, ((ASkeletalMeshActor*)actor)->ReplicatedMesh, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.TeamInfo")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_Engine_TeamInfo_Score_initial[(int)actor];
			if (FloatProperty_Engine_TeamInfo_Score != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeamInfo*)recent)->Score, ((ATeamInfo*)actor)->Score, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_TeamInfo_Score + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_TeamInfo_TeamIndex_initial[(int)actor];
			if (IntProperty_Engine_TeamInfo_TeamIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeamInfo*)recent)->TeamIndex, ((ATeamInfo*)actor)->TeamIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_TeamInfo_TeamIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_Engine_TeamInfo_TeamName_initial[(int)actor];
			if (StrProperty_Engine_TeamInfo_TeamName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeamInfo*)recent)->TeamName, ((ATeamInfo*)actor)->TeamName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_TeamInfo_TeamName + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Teleporter")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StrProperty_Engine_Teleporter_URL_initial[(int)actor];
			if (StrProperty_Engine_Teleporter_URL != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->URL, ((ATeleporter*)actor)->URL, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_Engine_Teleporter_URL + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bEnabled_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bEnabled, ((ATeleporter*)actor)->bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_Engine_Teleporter_TargetVelocity_initial[(int)actor];
			if (StructProperty_Engine_Teleporter_TargetVelocity != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->TargetVelocity, ((ATeleporter*)actor)->TargetVelocity, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_Teleporter_TargetVelocity + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bChangesVelocity_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bChangesVelocity != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bChangesVelocity, ((ATeleporter*)actor)->bChangesVelocity, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bChangesVelocity + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bChangesYaw_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bChangesYaw != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bChangesYaw, ((ATeleporter*)actor)->bChangesYaw, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bChangesYaw + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bReversesX_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bReversesX != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bReversesX, ((ATeleporter*)actor)->bReversesX, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bReversesX + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bReversesY_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bReversesY != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bReversesY, ((ATeleporter*)actor)->bReversesY, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bReversesY + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_Teleporter_bReversesZ_initial[(int)actor];
			if (BoolProperty_Engine_Teleporter_bReversesZ != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATeleporter*)recent)->bReversesZ, ((ATeleporter*)actor)->bReversesZ, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Teleporter_bReversesZ + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.Vehicle")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_Engine_Vehicle_bDriving_initial[(int)actor];
			if (BoolProperty_Engine_Vehicle_bDriving != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AVehicle*)recent)->bDriving, ((AVehicle*)actor)->bDriving, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_Vehicle_bDriving + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (((actor->bNetDirty || actor->bNetInitial) && (actor->bNetOwner || ((AVehicle*)actor)->Driver == NULL) || !((AVehicle*)actor)->Driver->bHidden) && actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_Engine_SVehicle_DriverViewPitch_initial[(int)actor];
			if (IntProperty_Engine_SVehicle_DriverViewPitch != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AVehicle*)recent)->Driver, ((AVehicle*)actor)->Driver, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_Engine_SVehicle_DriverViewPitch + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class Engine.WorldInfo")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_Engine_WorldInfo_Pauser_initial[(int)actor];
			if (ObjectProperty_Engine_WorldInfo_Pauser != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AWorldInfo*)recent)->Pauser, ((AWorldInfo*)actor)->Pauser, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_Engine_WorldInfo_Pauser + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_Engine_WorldInfo_ReplicatedMusicTrack_initial[(int)actor];
			if (StructProperty_Engine_WorldInfo_ReplicatedMusicTrack != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AWorldInfo*)recent)->ReplicatedMusicTrack, ((AWorldInfo*)actor)->ReplicatedMusicTrack, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_Engine_WorldInfo_ReplicatedMusicTrack + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_WorldInfo_TimeDilation_initial[(int)actor];
			if (FloatProperty_Engine_WorldInfo_TimeDilation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AWorldInfo*)recent)->TimeDilation, ((AWorldInfo*)actor)->TimeDilation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_WorldInfo_TimeDilation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_Engine_WorldInfo_WorldGravityZ_initial[(int)actor];
			if (FloatProperty_Engine_WorldInfo_WorldGravityZ != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AWorldInfo*)recent)->WorldGravityZ, ((AWorldInfo*)actor)->WorldGravityZ, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_Engine_WorldInfo_WorldGravityZ + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_Engine_WorldInfo_bHighPriorityLoading_initial[(int)actor];
			if (BoolProperty_Engine_WorldInfo_bHighPriorityLoading != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((AWorldInfo*)recent)->bHighPriorityLoading, ((AWorldInfo*)actor)->bHighPriorityLoading, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_Engine_WorldInfo_bHighPriorityLoading + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgChestActor")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgChestActor_r_eChestState_initial[(int)actor];
			if (ByteProperty_TgGame_TgChestActor_r_eChestState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgChestActor*)recent)->r_eChestState, ((ATgChestActor*)actor)->r_eChestState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgChestActor_r_eChestState + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDeploy_BeaconEntrance")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive_initial[(int)actor];
			if (BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeploy_BeaconEntrance*)recent)->r_bActive, ((ATgDeploy_BeaconEntrance*)actor)->r_bActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDeploy_DestructibleCover")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired_initial[(int)actor];
			if (BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeploy_DestructibleCover*)recent)->r_bHasFired, ((ATgDeploy_DestructibleCover*)actor)->r_bHasFired, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDeploy_Sensor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning_initial[(int)actor];
			if (IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeploy_Sensor*)recent)->r_nSensorAudioWarning, ((ATgDeploy_Sensor*)actor)->r_nSensorAudioWarning, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount_initial[(int)actor];
			if (IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeploy_Sensor*)recent)->r_nTouchedPlayerCount, ((ATgDeploy_Sensor*)actor)->r_nTouchedPlayerCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDeployable")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDeployable_r_bDelayDeployed_initial[(int)actor];
			if (BoolProperty_TgGame_TgDeployable_r_bDelayDeployed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_bDelayDeployed, ((ATgDeployable*)actor)->r_bDelayDeployed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDeployable_r_bDelayDeployed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt_initial[(int)actor];
			if (IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_nReplicateDestroyIt, ((ATgDeployable*)actor)->r_nReplicateDestroyIt, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgDeployable_r_DRI_initial[(int)actor];
			if (ObjectProperty_TgGame_TgDeployable_r_DRI != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_DRI, ((ATgDeployable*)actor)->r_DRI, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgDeployable_r_DRI + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy_initial[(int)actor];
			if (BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_bInitialIsEnemy, ((ATgDeployable*)actor)->r_bInitialIsEnemy, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgDeployable_r_bTakeDamage_initial[(int)actor];
			if (BoolProperty_TgGame_TgDeployable_r_bTakeDamage != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_bTakeDamage, ((ATgDeployable*)actor)->r_bTakeDamage, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDeployable_r_bTakeDamage + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius_initial[(int)actor];
			if (FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_fClientProximityRadius, ((ATgDeployable*)actor)->r_fClientProximityRadius, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_fCurrentDeployTime, ((ATgDeployable*)actor)->r_fCurrentDeployTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeployable_r_nDeployableId_initial[(int)actor];
			if (IntProperty_TgGame_TgDeployable_r_nDeployableId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_nDeployableId, ((ATgDeployable*)actor)->r_nDeployableId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeployable_r_nDeployableId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeployable_r_nPhysicalType_initial[(int)actor];
			if (IntProperty_TgGame_TgDeployable_r_nPhysicalType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_nPhysicalType, ((ATgDeployable*)actor)->r_nPhysicalType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeployable_r_nPhysicalType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeployable_r_nTickingTime_initial[(int)actor];
			if (IntProperty_TgGame_TgDeployable_r_nTickingTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_nTickingTime, ((ATgDeployable*)actor)->r_nTickingTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeployable_r_nTickingTime + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && actor->bNetOwner) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgDeployable_r_Owner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgDeployable_r_Owner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_Owner, ((ATgDeployable*)actor)->r_Owner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgDeployable_r_Owner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDeployable_r_nOwnerFireMode_initial[(int)actor];
			if (IntProperty_TgGame_TgDeployable_r_nOwnerFireMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDeployable*)recent)->r_nOwnerFireMode, ((ATgDeployable*)actor)->r_nOwnerFireMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDeployable_r_nOwnerFireMode + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDevice")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgDevice_CurrentFireMode_initial[(int)actor];
			if (ByteProperty_TgGame_TgDevice_CurrentFireMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->CurrentFireMode, ((ATgDevice*)actor)->CurrentFireMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgDevice_CurrentFireMode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgDevice_r_bIsStealthDevice_initial[(int)actor];
			if (BoolProperty_TgGame_TgDevice_r_bIsStealthDevice != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_bIsStealthDevice, ((ATgDevice*)actor)->r_bIsStealthDevice, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDevice_r_bIsStealthDevice + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgDevice_r_eEquippedAt_initial[(int)actor];
			if (ByteProperty_TgGame_TgDevice_r_eEquippedAt != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_eEquippedAt, ((ATgDevice*)actor)->r_eEquippedAt, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgDevice_r_eEquippedAt + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDevice_r_nInventoryId_initial[(int)actor];
			if (IntProperty_TgGame_TgDevice_r_nInventoryId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_nInventoryId, ((ATgDevice*)actor)->r_nInventoryId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDevice_r_nInventoryId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDevice_r_nMeleeComboSeed_initial[(int)actor];
			if (IntProperty_TgGame_TgDevice_r_nMeleeComboSeed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_nMeleeComboSeed, ((ATgDevice*)actor)->r_nMeleeComboSeed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDevice_r_nMeleeComboSeed + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath_initial[(int)actor];
			if (BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_bConsumedOnDeath, ((ATgDevice*)actor)->r_bConsumedOnDeath, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgDevice_r_bConsumedOnUse_initial[(int)actor];
			if (BoolProperty_TgGame_TgDevice_r_bConsumedOnUse != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_bConsumedOnUse, ((ATgDevice*)actor)->r_bConsumedOnUse, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDevice_r_bConsumedOnUse + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDevice_r_nDeviceId_initial[(int)actor];
			if (IntProperty_TgGame_TgDevice_r_nDeviceId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_nDeviceId, ((ATgDevice*)actor)->r_nDeviceId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDevice_r_nDeviceId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDevice_r_nDeviceInstanceId_initial[(int)actor];
			if (IntProperty_TgGame_TgDevice_r_nDeviceInstanceId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_nDeviceInstanceId, ((ATgDevice*)actor)->r_nDeviceInstanceId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDevice_r_nDeviceInstanceId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgDevice_r_nQualityValueId_initial[(int)actor];
			if (IntProperty_TgGame_TgDevice_r_nQualityValueId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice*)recent)->r_nQualityValueId, ((ATgDevice*)actor)->r_nQualityValueId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDevice_r_nQualityValueId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDevice_Morale")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring_initial[(int)actor];
			if (BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDevice_Morale*)recent)->r_bIsActivelyFiring, ((ATgDevice_Morale*)actor)->r_bIsActivelyFiring, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDoor")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgDoor_r_bOpen_initial[(int)actor];
			if (BoolProperty_TgGame_TgDoor_r_bOpen != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDoor*)recent)->r_bOpen, ((ATgDoor*)actor)->r_bOpen, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgDoor_r_bOpen + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDoorMarker")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgDoorMarker_r_eStatus_initial[(int)actor];
			if (ByteProperty_TgGame_TgDoorMarker_r_eStatus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDoorMarker*)recent)->r_eStatus, ((ATgDoorMarker*)actor)->r_eStatus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgDoorMarker_r_eStatus + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDroppedItem")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = IntProperty_TgGame_TgDroppedItem_r_nItemId_initial[(int)actor];
			if (IntProperty_TgGame_TgDroppedItem_r_nItemId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDroppedItem*)recent)->r_nItemId, ((ATgDroppedItem*)actor)->r_nItemId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDroppedItem_r_nItemId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDynamicDestructible")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId_initial[(int)actor];
			if (IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDynamicDestructible*)recent)->r_nDestructibleId, ((ATgDynamicDestructible*)actor)->r_nDestructibleId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory_initial[(int)actor];
			if (ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDynamicDestructible*)recent)->r_pFactory, ((ATgDynamicDestructible*)actor)->r_pFactory, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgDynamicSMActor")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = StrProperty_TgGame_TgDynamicSMActor_m_sAssembly_initial[(int)actor];
			if (StrProperty_TgGame_TgDynamicSMActor_m_sAssembly != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDynamicSMActor*)recent)->m_sAssembly, ((ATgDynamicSMActor*)actor)->m_sAssembly, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgDynamicSMActor_m_sAssembly + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager_initial[(int)actor];
			if (ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDynamicSMActor*)recent)->r_EffectManager, ((ATgDynamicSMActor*)actor)->r_EffectManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgDynamicSMActor_r_nHealth_initial[(int)actor];
			if (IntProperty_TgGame_TgDynamicSMActor_r_nHealth != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgDynamicSMActor*)recent)->r_nHealth, ((ATgDynamicSMActor*)actor)->r_nHealth, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgDynamicSMActor_r_nHealth + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgEffectManager")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_TgGame_TgEffectManager_r_EventQueue_initial[(int)actor];
			if (StructProperty_TgGame_TgEffectManager_r_EventQueue != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_EventQueue[i], ((ATgEffectManager*)actor)->r_EventQueue[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgEffectManager_r_EventQueue + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgEffectManager_r_ManagedEffectList_initial[(int)actor];
			if (StructProperty_TgGame_TgEffectManager_r_ManagedEffectList != nullptr) {
				for (int i = 0; i < 0x10; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_ManagedEffectList[i], ((ATgEffectManager*)actor)->r_ManagedEffectList[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgEffectManager_r_ManagedEffectList + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgEffectManager_r_Owner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgEffectManager_r_Owner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_Owner, ((ATgEffectManager*)actor)->r_Owner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgEffectManager_r_Owner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify_initial[(int)actor];
			if (BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_bRelevancyNotify, ((ATgEffectManager*)actor)->r_bRelevancyNotify, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount_initial[(int)actor];
			if (IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_nInvulnerableCount, ((ATgEffectManager*)actor)->r_nInvulnerableCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex_initial[(int)actor];
			if (IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEffectManager*)recent)->r_nNextQueueIndex, ((ATgEffectManager*)actor)->r_nNextQueueIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgEmitter")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = NameProperty_TgGame_TgEmitter_BoneName_initial[(int)actor];
			if (NameProperty_TgGame_TgEmitter_BoneName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgEmitter*)recent)->BoneName, ((ATgEmitter*)actor)->BoneName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)NameProperty_TgGame_TgEmitter_BoneName + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgFlagCaptureVolume")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition_initial[(int)actor];
			if (ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFlagCaptureVolume*)recent)->r_eCoalition, ((ATgFlagCaptureVolume*)actor)->r_eCoalition, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce_initial[(int)actor];
			if (ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFlagCaptureVolume*)recent)->r_nTaskForce, ((ATgFlagCaptureVolume*)actor)->r_nTaskForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgFracturedStaticMeshActor")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager_initial[(int)actor];
			if (ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_EffectManager, ((ATgFracturedStaticMeshActor*)actor)->r_EffectManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier_initial[(int)actor];
			if (IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_TakeHitNotifier, ((ATgFracturedStaticMeshActor*)actor)->r_TakeHitNotifier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius_initial[(int)actor];
			if (FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_DamageRadius, ((ATgFracturedStaticMeshActor*)actor)->r_DamageRadius, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType_initial[(int)actor];
			if (ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_HitDamageType, ((ATgFracturedStaticMeshActor*)actor)->r_HitDamageType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo_initial[(int)actor];
			if (StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_HitInfo, ((ATgFracturedStaticMeshActor*)actor)->r_HitInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_vTakeHitLocation, ((ATgFracturedStaticMeshActor*)actor)->r_vTakeHitLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum_initial[(int)actor];
			if (StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgFracturedStaticMeshActor*)recent)->r_vTakeHitMomentum, ((ATgFracturedStaticMeshActor*)actor)->r_vTakeHitMomentum, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgHexLandMarkActor")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId_initial[(int)actor];
			if (IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgHexLandMarkActor*)recent)->r_nMeshAsmId, ((ATgHexLandMarkActor*)actor)->r_nMeshAsmId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgInterpActor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StrProperty_TgGame_TgInterpActor_r_sCurrState_initial[(int)actor];
			if (StrProperty_TgGame_TgInterpActor_r_sCurrState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgInterpActor*)recent)->r_sCurrState, ((ATgInterpActor*)actor)->r_sCurrState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgInterpActor_r_sCurrState + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgInventoryManager")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgInventoryManager_r_ItemCount_initial[(int)actor];
			if (IntProperty_TgGame_TgInventoryManager_r_ItemCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgInventoryManager*)recent)->r_ItemCount, ((ATgInventoryManager*)actor)->r_ItemCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgInventoryManager_r_ItemCount + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgKismetTestActor")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest_initial[(int)actor];
			if (IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgKismetTestActor*)recent)->r_nCurrentTest, ((ATgKismetTestActor*)actor)->r_nCurrentTest, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgKismetTestActor_r_nFailCount_initial[(int)actor];
			if (IntProperty_TgGame_TgKismetTestActor_r_nFailCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgKismetTestActor*)recent)->r_nFailCount, ((ATgKismetTestActor*)actor)->r_nFailCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgKismetTestActor_r_nFailCount + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgKismetTestActor_r_nPassCount_initial[(int)actor];
			if (IntProperty_TgGame_TgKismetTestActor_r_nPassCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgKismetTestActor*)recent)->r_nPassCount, ((ATgKismetTestActor*)actor)->r_nPassCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgKismetTestActor_r_nPassCount + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgLevelCamera")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgLevelCamera_r_bEnabled_initial[(int)actor];
			if (BoolProperty_TgGame_TgLevelCamera_r_bEnabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgLevelCamera*)recent)->r_bEnabled, ((ATgLevelCamera*)actor)->r_bEnabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgLevelCamera_r_bEnabled + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgMissionObjective")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment_initial[(int)actor];
			if (ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_ObjectiveAssignment, ((ATgMissionObjective*)actor)->r_ObjectiveAssignment, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce_initial[(int)actor];
			if (BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_bHasBeenCapturedOnce, ((ATgMissionObjective*)actor)->r_bHasBeenCapturedOnce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgMissionObjective_r_bIsActive_initial[(int)actor];
			if (BoolProperty_TgGame_TgMissionObjective_r_bIsActive != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_bIsActive, ((ATgMissionObjective*)actor)->r_bIsActive, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgMissionObjective_r_bIsActive + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgMissionObjective_r_bIsLocked_initial[(int)actor];
			if (BoolProperty_TgGame_TgMissionObjective_r_bIsLocked != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_bIsLocked, ((ATgMissionObjective*)actor)->r_bIsLocked, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgMissionObjective_r_bIsLocked + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgMissionObjective_r_bIsPending_initial[(int)actor];
			if (BoolProperty_TgGame_TgMissionObjective_r_bIsPending != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_bIsPending, ((ATgMissionObjective*)actor)->r_bIsPending, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgMissionObjective_r_bIsPending + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition_initial[(int)actor];
			if (ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_eOwningCoalition, ((ATgMissionObjective*)actor)->r_eOwningCoalition, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgMissionObjective_r_eStatus_initial[(int)actor];
			if (ByteProperty_TgGame_TgMissionObjective_r_eStatus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_eStatus, ((ATgMissionObjective*)actor)->r_eStatus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgMissionObjective_r_eStatus + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_fCurrCaptureTime, ((ATgMissionObjective*)actor)->r_fCurrCaptureTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_fLastCompletedTime, ((ATgMissionObjective*)actor)->r_fLastCompletedTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce_initial[(int)actor];
			if (IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_nOwnerTaskForce, ((ATgMissionObjective*)actor)->r_nOwnerTaskForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = IntProperty_TgGame_TgMissionObjective_nObjectiveId_initial[(int)actor];
			if (IntProperty_TgGame_TgMissionObjective_nObjectiveId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->nObjectiveId, ((ATgMissionObjective*)actor)->nObjectiveId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgMissionObjective_nObjectiveId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgMissionObjective_nPriority_initial[(int)actor];
			if (IntProperty_TgGame_TgMissionObjective_nPriority != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->nPriority, ((ATgMissionObjective*)actor)->nPriority, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgMissionObjective_nPriority + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole_initial[(int)actor];
			if (ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_OpenWorldPlayerDefaultRole, ((ATgMissionObjective*)actor)->r_OpenWorldPlayerDefaultRole, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState_initial[(int)actor];
			if (BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_bUsePendingState, ((ATgMissionObjective*)actor)->r_bUsePendingState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition_initial[(int)actor];
			if (ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective*)recent)->r_eDefaultCoalition, ((ATgMissionObjective*)actor)->r_eDefaultCoalition, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgMissionObjective_Bot")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot_initial[(int)actor];
			if (ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective_Bot*)recent)->r_ObjectiveBot, ((ATgMissionObjective_Bot*)actor)->r_ObjectiveBot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo_initial[(int)actor];
			if (ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective_Bot*)recent)->r_ObjectiveBotInfo, ((ATgMissionObjective_Bot*)actor)->r_ObjectiveBotInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgMissionObjective_Escort")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor_initial[(int)actor];
			if (ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective_Escort*)recent)->r_AttachedActor, ((ATgMissionObjective_Escort*)actor)->r_AttachedActor, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgMissionObjective_Proximity")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate_initial[(int)actor];
			if (FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgMissionObjective_Proximity*)recent)->r_fCaptureRate, ((ATgMissionObjective_Proximity*)actor)->r_fCaptureRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgObjectiveAssignment")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective_initial[(int)actor];
			if (ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgObjectiveAssignment*)recent)->r_AssignedObjective, ((ATgObjectiveAssignment*)actor)->r_AssignedObjective, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers_initial[(int)actor];
			if (ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgObjectiveAssignment*)recent)->r_Attackers, ((ATgObjectiveAssignment*)actor)->r_Attackers, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots_initial[(int)actor];
			if (ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgObjectiveAssignment*)recent)->r_Bots, ((ATgObjectiveAssignment*)actor)->r_Bots, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders_initial[(int)actor];
			if (ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgObjectiveAssignment*)recent)->r_Defenders, ((ATgObjectiveAssignment*)actor)->r_Defenders, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgObjectiveAssignment_r_eState_initial[(int)actor];
			if (ByteProperty_TgGame_TgObjectiveAssignment_r_eState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgObjectiveAssignment*)recent)->r_eState, ((ATgObjectiveAssignment*)actor)->r_eState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgObjectiveAssignment_r_eState + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsBot_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsBot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsBot, ((ATgPawn*)actor)->r_bIsBot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsBot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsHenchman_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsHenchman != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsHenchman, ((ATgPawn*)actor)->r_bIsHenchman, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsHenchman + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bNeedPlaySpawnFx, ((ATgPawn*)actor)->r_bNeedPlaySpawnFx, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fMakeVisibleIncreased, ((ATgPawn*)actor)->r_fMakeVisibleIncreased, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nAllianceId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nAllianceId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nAllianceId, ((ATgPawn*)actor)->r_nAllianceId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nAllianceId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nBodyMeshAsmId, ((ATgPawn*)actor)->r_nBodyMeshAsmId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nBotRankValueId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nBotRankValueId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nBotRankValueId, ((ATgPawn*)actor)->r_nBotRankValueId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nBotRankValueId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nFlashEvent_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nFlashEvent != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nFlashEvent[i], ((ATgPawn*)actor)->r_nFlashEvent[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nFlashEvent + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nFlashFireInfo_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nFlashFireInfo != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nFlashFireInfo[i], ((ATgPawn*)actor)->r_nFlashFireInfo[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nFlashFireInfo + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nFlashQueIndex_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nFlashQueIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nFlashQueIndex, ((ATgPawn*)actor)->r_nFlashQueIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nFlashQueIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nPawnId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nPawnId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nPawnId, ((ATgPawn*)actor)->r_nPawnId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nPawnId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nPhysicalType_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nPhysicalType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nPhysicalType, ((ATgPawn*)actor)->r_nPhysicalType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nPhysicalType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nPreyProfileType_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nPreyProfileType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nPreyProfileType, ((ATgPawn*)actor)->r_nPreyProfileType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nPreyProfileType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nProfileId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nProfileId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nProfileId, ((ATgPawn*)actor)->r_nProfileId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nProfileId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nProfileTypeValueId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nProfileTypeValueId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nProfileTypeValueId, ((ATgPawn*)actor)->r_nProfileTypeValueId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nProfileTypeValueId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nSoundGroupId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nSoundGroupId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nSoundGroupId, ((ATgPawn*)actor)->r_nSoundGroupId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nSoundGroupId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_vFlashLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_vFlashLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_vFlashLocation, ((ATgPawn*)actor)->r_vFlashLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_vFlashLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_vFlashRayDir_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_vFlashRayDir != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_vFlashRayDir, ((ATgPawn*)actor)->r_vFlashRayDir, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_vFlashRayDir + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_vFlashRefireTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_vFlashRefireTime != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_vFlashRefireTime[i], ((ATgPawn*)actor)->r_vFlashRefireTime[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_vFlashRefireTime + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_vFlashSituationalAttack[i], ((ATgPawn*)actor)->r_vFlashSituationalAttack[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
		if ((actor->Role == 3) && !actor->bNetOwner) {
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_EquipDeviceInfo_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_EquipDeviceInfo != nullptr) {
				for (int i = 0; i < 0x19; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_EquipDeviceInfo[i], ((ATgPawn*)actor)->r_EquipDeviceInfo[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_EquipDeviceInfo + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bInitialIsEnemy, ((ATgPawn*)actor)->r_bInitialIsEnemy, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_bMadeSound_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_bMadeSound != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bMadeSound, ((ATgPawn*)actor)->r_bMadeSound, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_bMadeSound + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_eDesiredInHand_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_eDesiredInHand != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eDesiredInHand, ((ATgPawn*)actor)->r_eDesiredInHand, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_eDesiredInHand + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_eEquippedInHandMode_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_eEquippedInHandMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eEquippedInHandMode, ((ATgPawn*)actor)->r_eEquippedInHandMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_eEquippedInHandMode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nReplicateHit_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nReplicateHit != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nReplicateHit, ((ATgPawn*)actor)->r_nReplicateHit, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nReplicateHit + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && actor->bNetOwner) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_ControlPawn_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_ControlPawn != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_ControlPawn, ((ATgPawn*)actor)->r_ControlPawn, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_ControlPawn + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_CurrentOmegaVolume, ((ATgPawn*)actor)->r_CurrentOmegaVolume, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_CurrentSubzoneBilboardVol, ((ATgPawn*)actor)->r_CurrentSubzoneBilboardVol, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_CurrentSubzoneVol, ((ATgPawn*)actor)->r_CurrentSubzoneVol, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_ScannerSettings_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_ScannerSettings != nullptr) {
				for (int i = 0; i < 0x2; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_ScannerSettings[i], ((ATgPawn*)actor)->r_ScannerSettings[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_ScannerSettings + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_UIClockState_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_UIClockState != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UIClockState, ((ATgPawn*)actor)->r_UIClockState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_UIClockState + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_UIClockTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_UIClockTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UIClockTime, ((ATgPawn*)actor)->r_UIClockTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_UIClockTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_UITextBox1MessageID_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_UITextBox1MessageID != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox1MessageID, ((ATgPawn*)actor)->r_UITextBox1MessageID, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_UITextBox1MessageID + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_UITextBox1Packet_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_UITextBox1Packet != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox1Packet, ((ATgPawn*)actor)->r_UITextBox1Packet, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_UITextBox1Packet + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_UITextBox1Time_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_UITextBox1Time != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox1Time, ((ATgPawn*)actor)->r_UITextBox1Time, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_UITextBox1Time + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_UITextBox2MessageID_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_UITextBox2MessageID != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox2MessageID, ((ATgPawn*)actor)->r_UITextBox2MessageID, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_UITextBox2MessageID + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_UITextBox2Packet_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_UITextBox2Packet != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox2Packet, ((ATgPawn*)actor)->r_UITextBox2Packet, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_UITextBox2Packet + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_UITextBox2Time_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_UITextBox2Time != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_UITextBox2Time, ((ATgPawn*)actor)->r_UITextBox2Time, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_UITextBox2Time + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bAllowAddMoralePoints, ((ATgPawn*)actor)->r_bAllowAddMoralePoints, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bDisableAllDevices_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bDisableAllDevices != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bDisableAllDevices, ((ATgPawn*)actor)->r_bDisableAllDevices, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bDisableAllDevices + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bEnableCrafting_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bEnableCrafting != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bEnableCrafting, ((ATgPawn*)actor)->r_bEnableCrafting, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bEnableCrafting + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bEnableEquip_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bEnableEquip != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bEnableEquip, ((ATgPawn*)actor)->r_bEnableEquip, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bEnableEquip + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bEnableSkills_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bEnableSkills != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bEnableSkills, ((ATgPawn*)actor)->r_bEnableSkills, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bEnableSkills + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bInCombatFlag_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bInCombatFlag != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bInCombatFlag, ((ATgPawn*)actor)->r_bInCombatFlag, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bInCombatFlag + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bInGlobalOffhandCooldown, ((ATgPawn*)actor)->r_bInGlobalOffhandCooldown, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fCurrentPowerPool, ((ATgPawn*)actor)->r_fCurrentPowerPool, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fCurrentServerMoralePoints, ((ATgPawn*)actor)->r_fCurrentServerMoralePoints, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fMaxControlRange_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fMaxControlRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fMaxControlRange, ((ATgPawn*)actor)->r_fMaxControlRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fMaxControlRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fMaxPowerPool_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fMaxPowerPool != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fMaxPowerPool, ((ATgPawn*)actor)->r_fMaxPowerPool, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fMaxPowerPool + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fMoraleRechargeRate, ((ATgPawn*)actor)->r_fMoraleRechargeRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fRequiredMoralePoints, ((ATgPawn*)actor)->r_fRequiredMoralePoints, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fSkillRating_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fSkillRating != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fSkillRating, ((ATgPawn*)actor)->r_fSkillRating, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fSkillRating + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nCurrency_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nCurrency != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nCurrency, ((ATgPawn*)actor)->r_nCurrency, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nCurrency + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nHZPoints_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nHZPoints != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nHZPoints, ((ATgPawn*)actor)->r_nHZPoints, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nHZPoints + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nMoraleDeviceSlot, ((ATgPawn*)actor)->r_nMoraleDeviceSlot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nRestDeviceSlot_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nRestDeviceSlot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nRestDeviceSlot, ((ATgPawn*)actor)->r_nRestDeviceSlot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nRestDeviceSlot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nToken_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nToken != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nToken, ((ATgPawn*)actor)->r_nToken, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nToken + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nXp_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nXp != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nXp, ((ATgPawn*)actor)->r_nXp, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nXp + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_DistanceToPushback_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_DistanceToPushback != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_DistanceToPushback, ((ATgPawn*)actor)->r_DistanceToPushback, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_DistanceToPushback + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_EffectManager_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_EffectManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_EffectManager, ((ATgPawn*)actor)->r_EffectManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_EffectManager + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_FlightAcceleration_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_FlightAcceleration != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_FlightAcceleration, ((ATgPawn*)actor)->r_FlightAcceleration, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_FlightAcceleration + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_HangingRotation_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_HangingRotation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_HangingRotation, ((ATgPawn*)actor)->r_HangingRotation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_HangingRotation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_Owner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_Owner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_Owner, ((ATgPawn*)actor)->r_Owner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_Owner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_Pet_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_Pet != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_Pet, ((ATgPawn*)actor)->r_Pet, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_Pet + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_PlayAnimation_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_PlayAnimation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_PlayAnimation, ((ATgPawn*)actor)->r_PlayAnimation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_PlayAnimation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_PushbackDirection_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_PushbackDirection != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_PushbackDirection, ((ATgPawn*)actor)->r_PushbackDirection, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_PushbackDirection + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_Target_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_Target != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_Target, ((ATgPawn*)actor)->r_Target, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_Target + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_TargetActor_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_TargetActor != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_TargetActor, ((ATgPawn*)actor)->r_TargetActor, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_TargetActor + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_aDebugDestination_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_aDebugDestination != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_aDebugDestination, ((ATgPawn*)actor)->r_aDebugDestination, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_aDebugDestination + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_aDebugNextNav_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_aDebugNextNav != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_aDebugNextNav, ((ATgPawn*)actor)->r_aDebugNextNav, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_aDebugNextNav + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_r_aDebugTarget_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_r_aDebugTarget != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_aDebugTarget, ((ATgPawn*)actor)->r_aDebugTarget, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_r_aDebugTarget + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_bAimType_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_bAimType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bAimType, ((ATgPawn*)actor)->r_bAimType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_bAimType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bAimingMode_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bAimingMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bAimingMode, ((ATgPawn*)actor)->r_bAimingMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bAimingMode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bCallingForHelp_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bCallingForHelp != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bCallingForHelp, ((ATgPawn*)actor)->r_bCallingForHelp, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bCallingForHelp + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsAFK_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsAFK != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsAFK, ((ATgPawn*)actor)->r_bIsAFK, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsAFK + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsAnimInStrafeMode, ((ATgPawn*)actor)->r_bIsAnimInStrafeMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsCrafting_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsCrafting != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsCrafting, ((ATgPawn*)actor)->r_bIsCrafting, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsCrafting + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsCrewing_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsCrewing != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsCrewing, ((ATgPawn*)actor)->r_bIsCrewing, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsCrewing + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsDecoy_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsDecoy != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsDecoy, ((ATgPawn*)actor)->r_bIsDecoy, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsDecoy + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsGrappleDismounting, ((ATgPawn*)actor)->r_bIsGrappleDismounting, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsHacked_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsHacked != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsHacked, ((ATgPawn*)actor)->r_bIsHacked, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsHacked + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsHacking_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsHacking != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsHacking, ((ATgPawn*)actor)->r_bIsHacking, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsHacking + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsHanging, ((ATgPawn*)actor)->r_bIsHanging, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsHanging, ((ATgPawn*)actor)->r_bIsHanging, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsInSnipeScope, ((ATgPawn*)actor)->r_bIsInSnipeScope, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsRappelling_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsRappelling != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsRappelling, ((ATgPawn*)actor)->r_bIsRappelling, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsRappelling + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bIsStealthed_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bIsStealthed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bIsStealthed, ((ATgPawn*)actor)->r_bIsStealthed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bIsStealthed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bJumpedFromHanging, ((ATgPawn*)actor)->r_bJumpedFromHanging, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bPostureIgnoreTransition, ((ATgPawn*)actor)->r_bPostureIgnoreTransition, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bResistTagging_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bResistTagging != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bResistTagging, ((ATgPawn*)actor)->r_bResistTagging, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bResistTagging + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bShouldKnockDownAnimFaceDown, ((ATgPawn*)actor)->r_bShouldKnockDownAnimFaceDown, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bTagEnemy_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bTagEnemy != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bTagEnemy, ((ATgPawn*)actor)->r_bTagEnemy, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bTagEnemy + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_r_bUsingBinoculars_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_r_bUsingBinoculars != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_bUsingBinoculars, ((ATgPawn*)actor)->r_bUsingBinoculars, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_r_bUsingBinoculars + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_eCurrentStunType_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_eCurrentStunType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eCurrentStunType, ((ATgPawn*)actor)->r_eCurrentStunType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_eCurrentStunType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_eDeathReason_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_eDeathReason != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eDeathReason, ((ATgPawn*)actor)->r_eDeathReason, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_eDeathReason + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_eEmoteLength_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_eEmoteLength != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eEmoteLength, ((ATgPawn*)actor)->r_eEmoteLength, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_eEmoteLength + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_eEmoteRepnotify_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_eEmoteRepnotify != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eEmoteRepnotify, ((ATgPawn*)actor)->r_eEmoteRepnotify, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_eEmoteRepnotify + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_eEmoteUpdate_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_eEmoteUpdate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_eEmoteUpdate, ((ATgPawn*)actor)->r_eEmoteUpdate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_eEmoteUpdate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_r_ePosture_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_r_ePosture != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_ePosture, ((ATgPawn*)actor)->r_ePosture, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_r_ePosture + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fDeployRate_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fDeployRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fDeployRate, ((ATgPawn*)actor)->r_fDeployRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fDeployRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fFrictionMultiplier, ((ATgPawn*)actor)->r_fFrictionMultiplier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fGravityZModifier_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fGravityZModifier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fGravityZModifier, ((ATgPawn*)actor)->r_fGravityZModifier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fGravityZModifier + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fKnockDownTimeRemaining, ((ATgPawn*)actor)->r_fKnockDownTimeRemaining, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fMakeVisibleFadeRate, ((ATgPawn*)actor)->r_fMakeVisibleFadeRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fPostureRateScale_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fPostureRateScale != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fPostureRateScale, ((ATgPawn*)actor)->r_fPostureRateScale, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fPostureRateScale + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fRappelGravityModifier, ((ATgPawn*)actor)->r_fRappelGravityModifier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fStealthTransitionTime, ((ATgPawn*)actor)->r_fStealthTransitionTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_fWeightBonus_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_fWeightBonus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_fWeightBonus, ((ATgPawn*)actor)->r_fWeightBonus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_fWeightBonus + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_iKnockDownFlash_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_iKnockDownFlash != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_iKnockDownFlash, ((ATgPawn*)actor)->r_iKnockDownFlash, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_iKnockDownFlash + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nApplyStealth_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nApplyStealth != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nApplyStealth, ((ATgPawn*)actor)->r_nApplyStealth, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nApplyStealth + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nBotSoundCueId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nBotSoundCueId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nBotSoundCueId, ((ATgPawn*)actor)->r_nBotSoundCueId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nBotSoundCueId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nDebugAggroRange_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nDebugAggroRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nDebugAggroRange, ((ATgPawn*)actor)->r_nDebugAggroRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nDebugAggroRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nDebugFOV_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nDebugFOV != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nDebugFOV, ((ATgPawn*)actor)->r_nDebugFOV, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nDebugFOV + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nDebugHearingRange_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nDebugHearingRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nDebugHearingRange, ((ATgPawn*)actor)->r_nDebugHearingRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nDebugHearingRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nDebugSightRange_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nDebugSightRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nDebugSightRange, ((ATgPawn*)actor)->r_nDebugSightRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nDebugSightRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nGenericAIEventIndex, ((ATgPawn*)actor)->r_nGenericAIEventIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nHealthMaximum_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nHealthMaximum != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nHealthMaximum, ((ATgPawn*)actor)->r_nHealthMaximum, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nHealthMaximum + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nNumberTimesCrewed, ((ATgPawn*)actor)->r_nNumberTimesCrewed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nPhase_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nPhase != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nPhase, ((ATgPawn*)actor)->r_nPhase, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nPhase + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nPitchOffset_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nPitchOffset != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nPitchOffset, ((ATgPawn*)actor)->r_nPitchOffset, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nPitchOffset + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nReplicateDying_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nReplicateDying != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nReplicateDying, ((ATgPawn*)actor)->r_nReplicateDying, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nReplicateDying + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nResetCharacter_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nResetCharacter != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nResetCharacter, ((ATgPawn*)actor)->r_nResetCharacter, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nResetCharacter + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nSensorAlertLevel_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nSensorAlertLevel != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nSensorAlertLevel, ((ATgPawn*)actor)->r_nSensorAlertLevel, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nSensorAlertLevel + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nShieldHealthMax_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nShieldHealthMax != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nShieldHealthMax, ((ATgPawn*)actor)->r_nShieldHealthMax, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nShieldHealthMax + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nShieldHealthRemaining, ((ATgPawn*)actor)->r_nShieldHealthRemaining, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nSilentMode_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nSilentMode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nSilentMode, ((ATgPawn*)actor)->r_nSilentMode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nSilentMode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nStealthAggroRange_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nStealthAggroRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nStealthAggroRange, ((ATgPawn*)actor)->r_nStealthAggroRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nStealthAggroRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nStealthDisabled_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nStealthDisabled != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nStealthDisabled, ((ATgPawn*)actor)->r_nStealthDisabled, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nStealthDisabled + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nStealthSensorRange_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nStealthSensorRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nStealthSensorRange, ((ATgPawn*)actor)->r_nStealthSensorRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nStealthSensorRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nStealthTypeCode_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nStealthTypeCode != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nStealthTypeCode, ((ATgPawn*)actor)->r_nStealthTypeCode, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nStealthTypeCode + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_r_nYawOffset_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_r_nYawOffset != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_nYawOffset, ((ATgPawn*)actor)->r_nYawOffset, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_r_nYawOffset + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgPawn_r_sDebugAction_initial[(int)actor];
			if (StrProperty_TgGame_TgPawn_r_sDebugAction != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_sDebugAction, ((ATgPawn*)actor)->r_sDebugAction, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgPawn_r_sDebugAction + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgPawn_r_sDebugName_initial[(int)actor];
			if (StrProperty_TgGame_TgPawn_r_sDebugName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_sDebugName, ((ATgPawn*)actor)->r_sDebugName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgPawn_r_sDebugName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgPawn_r_sFactory_initial[(int)actor];
			if (StrProperty_TgGame_TgPawn_r_sFactory != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_sFactory, ((ATgPawn*)actor)->r_sFactory, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgPawn_r_sFactory + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_r_vDown_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_r_vDown != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn*)recent)->r_vDown, ((ATgPawn*)actor)->r_vDown, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_r_vDown + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Ambush")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Ambush*)recent)->r_bIsDeployed, ((ATgPawn_Ambush*)actor)->r_bIsDeployed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_AttackTransport")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_AttackTransport*)recent)->r_DeathType, ((ATgPawn_AttackTransport*)actor)->r_DeathType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_CTR")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial) || (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_CTR*)recent)->r_CustomCharacterAssembly, ((ATgPawn_CTR*)actor)->r_CustomCharacterAssembly, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_CTR*)recent)->r_PilotPawn, ((ATgPawn_CTR*)actor)->r_PilotPawn, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_CTR*)recent)->r_nMaxMorphIndexSentFromServer, ((ATgPawn_CTR*)actor)->r_nMaxMorphIndexSentFromServer, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings != nullptr) {
				for (int i = 0; i < 0xFF; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_CTR*)recent)->r_nMorphSettings[i], ((ATgPawn_CTR*)actor)->r_nMorphSettings[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Character")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial) || (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly_initial[(int)actor];
			if (StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_CustomCharacterAssembly, ((ATgPawn_Character*)actor)->r_CustomCharacterAssembly, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_eAttachedMesh, ((ATgPawn_Character*)actor)->r_eAttachedMesh, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nBoostTimeRemaining, ((ATgPawn_Character*)actor)->r_nBoostTimeRemaining, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nHeadMeshAsmId, ((ATgPawn_Character*)actor)->r_nHeadMeshAsmId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nItemProfileId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nItemProfileId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nItemProfileId, ((ATgPawn_Character*)actor)->r_nItemProfileId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nItemProfileId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nItemProfileNbr, ((ATgPawn_Character*)actor)->r_nItemProfileNbr, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nMaxMorphIndexSentFromServer, ((ATgPawn_Character*)actor)->r_nMaxMorphIndexSentFromServer, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nMorphSettings_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nMorphSettings != nullptr) {
				for (int i = 0; i < 0xFF; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nMorphSettings[i], ((ATgPawn_Character*)actor)->r_nMorphSettings[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nMorphSettings + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
		if (((actor->Role == 3) && actor->bNetOwner) && (actor->bNetInitial || actor->bNetDirty) || (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet_initial[(int)actor];
			if (ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_CurrentVanityPet, ((ATgPawn_Character*)actor)->r_CurrentVanityPet, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_WallJumpUpperLineCheckOffset, ((ATgPawn_Character*)actor)->r_WallJumpUpperLineCheckOffset, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_WallJumpZ, ((ATgPawn_Character*)actor)->r_WallJumpZ, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_bElfGogglesEquipped, ((ATgPawn_Character*)actor)->r_bElfGogglesEquipped, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nDeviceSlotUnlockGrpId, ((ATgPawn_Character*)actor)->r_nDeviceSlotUnlockGrpId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Character*)recent)->r_nSkillGroupSetId, ((ATgPawn_Character*)actor)->r_nSkillGroupSetId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_DuneCommander")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial) || (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_DuneCommander*)recent)->r_bDoCrashLanding, ((ATgPawn_DuneCommander*)actor)->r_bDoCrashLanding, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Iris")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Iris*)recent)->r_nStartNewScan, ((ATgPawn_Iris*)actor)->r_nStartNewScan, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Reaper")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Reaper*)recent)->r_fBatteryPct, ((ATgPawn_Reaper*)actor)->r_fBatteryPct, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Siege")) {
		if ((actor->Role == 3) && (actor->bNetDirty || actor->bNetInitial)) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection_initial[(int)actor];
			if (ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Siege*)recent)->r_AccelDirection, ((ATgPawn_Siege*)actor)->r_AccelDirection, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_Turret")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed_initial[(int)actor];
			if (BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Turret*)recent)->r_bIsDeployed, ((ATgPawn_Turret*)actor)->r_bIsDeployed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Turret*)recent)->r_fInitDeployTime, ((ATgPawn_Turret*)actor)->r_fInitDeployTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Turret*)recent)->r_fTimeToDeploySecs, ((ATgPawn_Turret*)actor)->r_fTimeToDeploySecs, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Turret*)recent)->r_fCurrentDeployTime, ((ATgPawn_Turret*)actor)->r_fCurrentDeployTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT_initial[(int)actor];
			if (FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_Turret*)recent)->r_fDeployMaxHealthPCT, ((ATgPawn_Turret*)actor)->r_fDeployMaxHealthPCT, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPawn_VanityPet")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId_initial[(int)actor];
			if (IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPawn_VanityPet*)recent)->r_nSpawningItemId, ((ATgPawn_VanityPet*)actor)->r_nSpawningItemId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgPlayerController")) {
		if ((actor->Role == 3) && actor->bNetOwner) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer_initial[(int)actor];
			if (ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_WatchOtherPlayer, ((ATgPlayerController*)actor)->r_WatchOtherPlayer, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects_initial[(int)actor];
			if (BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_bEDDebugEffects, ((ATgPlayerController*)actor)->r_bEDDebugEffects, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPlayerController_r_bGMInvisible_initial[(int)actor];
			if (BoolProperty_TgGame_TgPlayerController_r_bGMInvisible != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_bGMInvisible, ((ATgPlayerController*)actor)->r_bGMInvisible, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPlayerController_r_bGMInvisible + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot_initial[(int)actor];
			if (BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_bIsHackingABot, ((ATgPlayerController*)actor)->r_bIsHackingABot, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation_initial[(int)actor];
			if (BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_bLockYawRotation, ((ATgPlayerController*)actor)->r_bLockYawRotation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgPlayerController_r_bRove_initial[(int)actor];
			if (BoolProperty_TgGame_TgPlayerController_r_bRove != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgPlayerController*)recent)->r_bRove, ((ATgPlayerController*)actor)->r_bRove, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgPlayerController_r_bRove + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgProj_Grapple")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Grapple*)recent)->r_vTargetLocation, ((ATgProj_Grapple*)actor)->r_vTargetLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgProj_Missile")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgProj_Missile_r_aSeeking_initial[(int)actor];
			if (ObjectProperty_TgGame_TgProj_Missile_r_aSeeking != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Missile*)recent)->r_aSeeking, ((ATgProj_Missile*)actor)->r_aSeeking, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgProj_Missile_r_aSeeking + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Missile*)recent)->r_vTargetWorldLocation, ((ATgProj_Missile*)actor)->r_vTargetWorldLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = IntProperty_TgGame_TgProj_Missile_r_nNumBounces_initial[(int)actor];
			if (IntProperty_TgGame_TgProj_Missile_r_nNumBounces != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Missile*)recent)->r_nNumBounces, ((ATgProj_Missile*)actor)->r_nNumBounces, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgProj_Missile_r_nNumBounces + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgProj_Rocket")) {
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgProj_Rocket_FlockIndex_initial[(int)actor];
			if (ByteProperty_TgGame_TgProj_Rocket_FlockIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Rocket*)recent)->FlockIndex, ((ATgProj_Rocket*)actor)->FlockIndex, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgProj_Rocket_FlockIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgProj_Rocket_bCurl_initial[(int)actor];
			if (BoolProperty_TgGame_TgProj_Rocket_bCurl != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProj_Rocket*)recent)->bCurl, ((ATgProj_Rocket*)actor)->bCurl, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgProj_Rocket_bCurl + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgProjectile")) {
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgProjectile_r_Owner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgProjectile_r_Owner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_Owner, ((ATgProjectile*)actor)->r_Owner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgProjectile_r_Owner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgProjectile_r_fAccelRate_initial[(int)actor];
			if (FloatProperty_TgGame_TgProjectile_r_fAccelRate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_fAccelRate, ((ATgProjectile*)actor)->r_fAccelRate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgProjectile_r_fAccelRate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgProjectile_r_fDuration_initial[(int)actor];
			if (FloatProperty_TgGame_TgProjectile_r_fDuration != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_fDuration, ((ATgProjectile*)actor)->r_fDuration, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgProjectile_r_fDuration + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgProjectile_r_fRange_initial[(int)actor];
			if (FloatProperty_TgGame_TgProjectile_r_fRange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_fRange, ((ATgProjectile*)actor)->r_fRange, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgProjectile_r_fRange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId_initial[(int)actor];
			if (IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_nOwnerFireModeId, ((ATgProjectile*)actor)->r_nOwnerFireModeId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgProjectile_r_nProjectileId_initial[(int)actor];
			if (IntProperty_TgGame_TgProjectile_r_nProjectileId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_nProjectileId, ((ATgProjectile*)actor)->r_nProjectileId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgProjectile_r_nProjectileId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgProjectile_r_vSpawnLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgProjectile_r_vSpawnLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgProjectile*)recent)->r_vSpawnLocation, ((ATgProjectile*)actor)->r_vSpawnLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgProjectile_r_vSpawnLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_Beacon")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Beacon*)recent)->r_bDeployed, ((ATgRepInfo_Beacon*)actor)->r_bDeployed, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Beacon*)recent)->r_vLoc, ((ATgRepInfo_Beacon*)actor)->r_vLoc, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = StrProperty_TgGame_TgRepInfo_Beacon_r_nName_initial[(int)actor];
			if (StrProperty_TgGame_TgRepInfo_Beacon_r_nName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Beacon*)recent)->r_nName, ((ATgRepInfo_Beacon*)actor)->r_nName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgRepInfo_Beacon_r_nName + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_Deployable")) {
		if ((actor->bNetDirty || actor->bNetInitial) && actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_InstigatorInfo, ((ATgRepInfo_Deployable*)actor)->r_InstigatorInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_TaskforceInfo, ((ATgRepInfo_Deployable*)actor)->r_TaskforceInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_bOwnedByTaskforce, ((ATgRepInfo_Deployable*)actor)->r_bOwnedByTaskforce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_nHealthCurrent, ((ATgRepInfo_Deployable*)actor)->r_nHealthCurrent, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->bNetInitial || actor->bNetDirty) && actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_DeployableOwner, ((ATgRepInfo_Deployable*)actor)->r_DeployableOwner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT_initial[(int)actor];
			if (FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_fDeployMaxHealthPCT, ((ATgRepInfo_Deployable*)actor)->r_fDeployMaxHealthPCT, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_nDeployableId, ((ATgRepInfo_Deployable*)actor)->r_nDeployableId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_nHealthMaximum, ((ATgRepInfo_Deployable*)actor)->r_nHealthMaximum, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Deployable*)recent)->r_nUniqueDeployableId, ((ATgRepInfo_Deployable*)actor)->r_nUniqueDeployableId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_Game")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_MiniMapInfo, ((ATgRepInfo_Game*)actor)->r_MiniMapInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bActiveCombat, ((ATgRepInfo_Game*)actor)->r_bActiveCombat, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bAllowBuildMorale, ((ATgRepInfo_Game*)actor)->r_bAllowBuildMorale, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bAllowPlayerRelease, ((ATgRepInfo_Game*)actor)->r_bAllowPlayerRelease, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bDefenseAlarm, ((ATgRepInfo_Game*)actor)->r_bDefenseAlarm, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bInOverTime, ((ATgRepInfo_Game*)actor)->r_bInOverTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsTutorialMap, ((ATgRepInfo_Game*)actor)->r_bIsTutorialMap, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier_initial[(int)actor];
			if (FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_fGameSpeedModifier, ((ATgRepInfo_Game*)actor)->r_fGameSpeedModifier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_fMissionRemainingTime, ((ATgRepInfo_Game*)actor)->r_fMissionRemainingTime, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate_initial[(int)actor];
			if (FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_fServerTimeLastUpdate, ((ATgRepInfo_Game*)actor)->r_fServerTimeLastUpdate, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nMaxRoundNumber, ((ATgRepInfo_Game*)actor)->r_nMaxRoundNumber, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nMissionTimerState, ((ATgRepInfo_Game*)actor)->r_nMissionTimerState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nMissionTimerState, ((ATgRepInfo_Game*)actor)->r_nMissionTimerState, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nRaidAttackerRespawnBonus, ((ATgRepInfo_Game*)actor)->r_nRaidAttackerRespawnBonus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nRaidDefenderRespawnBonus, ((ATgRepInfo_Game*)actor)->r_nRaidDefenderRespawnBonus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nReleaseDelay, ((ATgRepInfo_Game*)actor)->r_nReleaseDelay, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nRoundNumber, ((ATgRepInfo_Game*)actor)->r_nRoundNumber, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nSecsToAutoReleaseAttackers, ((ATgRepInfo_Game*)actor)->r_nSecsToAutoReleaseAttackers, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nSecsToAutoReleaseDefenders, ((ATgRepInfo_Game*)actor)->r_nSecsToAutoReleaseDefenders, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgRepInfo_Game_r_GameType_initial[(int)actor];
			if (ByteProperty_TgGame_TgRepInfo_Game_r_GameType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_GameType, ((ATgRepInfo_Game*)actor)->r_GameType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgRepInfo_Game_r_GameType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_MapLogoResIds, ((ATgRepInfo_Game*)actor)->r_MapLogoResIds, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives != nullptr) {
				for (int i = 0; i < 0x4B; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_Objectives[i], ((ATgRepInfo_Game*)actor)->r_Objectives[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsArena, ((ATgRepInfo_Game*)actor)->r_bIsArena, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsMatch, ((ATgRepInfo_Game*)actor)->r_bIsMatch, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsMission, ((ATgRepInfo_Game*)actor)->r_bIsMission, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsPVP, ((ATgRepInfo_Game*)actor)->r_bIsPVP, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsRaid, ((ATgRepInfo_Game*)actor)->r_bIsRaid, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsTerritoryMap, ((ATgRepInfo_Game*)actor)->r_bIsTerritoryMap, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_bIsTraining, ((ATgRepInfo_Game*)actor)->r_bIsTraining, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nAutoKickTimeout, ((ATgRepInfo_Game*)actor)->r_nAutoKickTimeout, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nPointsToWin, ((ATgRepInfo_Game*)actor)->r_nPointsToWin, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Game*)recent)->r_nVictoryBonusLives, ((ATgRepInfo_Game*)actor)->r_nVictoryBonusLives, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_GameOpenWorld")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets != nullptr) {
				for (int i = 0; i < 0x3; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_GameOpenWorld*)recent)->r_GameTickets[i], ((ATgRepInfo_GameOpenWorld*)actor)->r_GameTickets[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_Player")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_ApproxLocation, ((ATgRepInfo_Player*)actor)->r_ApproxLocation, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_CustomCharacterAssembly, ((ATgRepInfo_Player*)actor)->r_CustomCharacterAssembly, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo != nullptr) {
				for (int i = 0; i < 0x19; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_EquipDeviceInfo[i], ((ATgRepInfo_Player*)actor)->r_EquipDeviceInfo[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_MasterPrep, ((ATgRepInfo_Player*)actor)->r_MasterPrep, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_PawnOwner, ((ATgRepInfo_Player*)actor)->r_PawnOwner, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_Scores_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_Scores != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_Scores, ((ATgRepInfo_Player*)actor)->r_Scores, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_Scores + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_TaskForce, ((ATgRepInfo_Player*)actor)->r_TaskForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_Player_r_bDropped_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_Player_r_bDropped != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_bDropped, ((ATgRepInfo_Player*)actor)->r_bDropped, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_Player_r_bDropped + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_eBonusType_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_eBonusType != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_eBonusType, ((ATgRepInfo_Player*)actor)->r_eBonusType, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_eBonusType + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nCharacterId, ((ATgRepInfo_Player*)actor)->r_nCharacterId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nHealthCurrent, ((ATgRepInfo_Player*)actor)->r_nHealthCurrent, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nHealthMaximum, ((ATgRepInfo_Player*)actor)->r_nHealthMaximum, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nLevel_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nLevel != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nLevel, ((ATgRepInfo_Player*)actor)->r_nLevel, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nLevel + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nProfileId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nProfileId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nProfileId, ((ATgRepInfo_Player*)actor)->r_nProfileId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nProfileId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_nTitleMsgId, ((ATgRepInfo_Player*)actor)->r_nTitleMsgId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName_initial[(int)actor];
			if (StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_sAgencyName, ((ATgRepInfo_Player*)actor)->r_sAgencyName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName_initial[(int)actor];
			if (StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_sAllianceName, ((ATgRepInfo_Player*)actor)->r_sAllianceName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName_initial[(int)actor];
			if (StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_sOrigPlayerName, ((ATgRepInfo_Player*)actor)->r_sOrigPlayerName, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if (actor->bNetOwner && actor->Role == 3) {
			{
			bool &initialFlag = StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats_initial[(int)actor];
			if (StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats != nullptr) {
				for (int i = 0; i < 0x9; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_Player*)recent)->r_DeviceStats[i], ((ATgRepInfo_Player*)actor)->r_DeviceStats[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgRepInfo_TaskForce")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_BeaconManager, ((ATgRepInfo_TaskForce*)actor)->r_BeaconManager, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_CurrActiveObjective, ((ATgRepInfo_TaskForce*)actor)->r_CurrActiveObjective, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment_initial[(int)actor];
			if (ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_ObjectiveAssignment, ((ATgRepInfo_TaskForce*)actor)->r_ObjectiveAssignment, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned_initial[(int)actor];
			if (BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_bBotOwned, ((ATgRepInfo_TaskForce*)actor)->r_bBotOwned, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition_initial[(int)actor];
			if (ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_eCoalition, ((ATgRepInfo_TaskForce*)actor)->r_eCoalition, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nCurrentPointCount, ((ATgRepInfo_TaskForce*)actor)->r_nCurrentPointCount, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nLeaderCharId, ((ATgRepInfo_TaskForce*)actor)->r_nLeaderCharId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers_initial[(int)actor];
			if (FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nLookingForMembers, ((ATgRepInfo_TaskForce*)actor)->r_nLookingForMembers, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nNumDeaths, ((ATgRepInfo_TaskForce*)actor)->r_nNumDeaths, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce_initial[(int)actor];
			if (ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nTaskForce, ((ATgRepInfo_TaskForce*)actor)->r_nTaskForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId_initial[(int)actor];
			if (IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgRepInfo_TaskForce*)recent)->r_nTeamId, ((ATgRepInfo_TaskForce*)actor)->r_nTeamId, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgSkydiveTarget")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius_initial[(int)actor];
			if (FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgSkydiveTarget*)recent)->m_LandRadius, ((ATgSkydiveTarget*)actor)->m_LandRadius, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgSkydivingVolume")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier_initial[(int)actor];
			if (FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgSkydivingVolume*)recent)->r_PawnGravityModifier, ((ATgSkydivingVolume*)actor)->r_PawnGravityModifier, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce_initial[(int)actor];
			if (FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgSkydivingVolume*)recent)->r_PawnLaunchForce, ((ATgSkydivingVolume*)actor)->r_PawnLaunchForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce_initial[(int)actor];
			if (FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgSkydivingVolume*)recent)->r_PawnUpForce, ((ATgSkydivingVolume*)actor)->r_PawnUpForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget_initial[(int)actor];
			if (ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgSkydivingVolume*)recent)->r_SkydiveTarget, ((ATgSkydivingVolume*)actor)->r_SkydiveTarget, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgTeamBeaconManager")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed_initial[(int)actor];
			if (IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_Beacon, ((ATgTeamBeaconManager*)actor)->r_Beacon, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed_initial[(int)actor];
			if (IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_Beacon, ((ATgTeamBeaconManager*)actor)->r_Beacon, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder_initial[(int)actor];
			if (ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_BeaconHolder, ((ATgTeamBeaconManager*)actor)->r_BeaconHolder, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo_initial[(int)actor];
			if (ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_BeaconInfo, ((ATgTeamBeaconManager*)actor)->r_BeaconInfo, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus_initial[(int)actor];
			if (ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_BeaconStatus, ((ATgTeamBeaconManager*)actor)->r_BeaconStatus, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
		if ((actor->Role == 3) && (actor->bNetInitial || actor->bNetDirty)) {
			{
			bool &initialFlag = ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce_initial[(int)actor];
			if (ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTeamBeaconManager*)recent)->r_TaskForce, ((ATgTeamBeaconManager*)actor)->r_TaskForce, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce + 0x5E);
				*param_3++ = repindex;
			}
			}
		}
	}
	if (ObjectIsA(actor, "Class TgGame.TgTimerManager")) {
		if (actor->Role == 3) {
			{
			bool &initialFlag = ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex_initial[(int)actor];
			if (ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTimerManager*)recent)->r_byEventQue, ((ATgTimerManager*)actor)->r_byEventQue, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex_initial[(int)actor];
			if (ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTimerManager*)recent)->r_byEventQue, ((ATgTimerManager*)actor)->r_byEventQue, (void*)param_4, (void*)param_5))) {
				initialFlag = true;
				repindex = *(int*)((char*)ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex + 0x5E);
				*param_3++ = repindex;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgTimerManager_r_fRemaining_initial[(int)actor];
			if (FloatProperty_TgGame_TgTimerManager_r_fRemaining != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTimerManager*)recent)->r_fRemaining[i], ((ATgTimerManager*)actor)->r_fRemaining[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)FloatProperty_TgGame_TgTimerManager_r_fRemaining + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
			{
			bool &initialFlag = FloatProperty_TgGame_TgTimerManager_r_fStartTime_initial[(int)actor];
			if (FloatProperty_TgGame_TgTimerManager_r_fStartTime != nullptr) {
				for (int i = 0; i < 0x20; i++) {
					if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((ATgTimerManager*)recent)->r_fStartTime[i], ((ATgTimerManager*)actor)->r_fStartTime[i], (void*)param_4, (void*)param_5)) {
							repindex = *(int*)((char*)FloatProperty_TgGame_TgTimerManager_r_fStartTime + 0x5E);
							*param_3++ = repindex+i;
					}
				}
				initialFlag = true;
			}
			}
		}
	}


	return param_3;
}

