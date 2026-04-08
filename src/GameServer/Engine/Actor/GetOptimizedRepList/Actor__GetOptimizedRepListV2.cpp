#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/Utils/Macros.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

bool Actor__GetOptimizedRepList::bRepListCached = false;
UProperty* ObjectProperty_Engine_Actor_Base = nullptr;
std::map<int, bool> ObjectProperty_Engine_Actor_Base_initial;
UProperty* ByteProperty_Engine_Actor_Physics = nullptr;
std::map<int, bool> ByteProperty_Engine_Actor_Physics_initial;
UProperty* StructProperty_Engine_Actor_Velocity = nullptr;
std::map<int, bool> StructProperty_Engine_Actor_Velocity_initial;
UProperty* ByteProperty_Engine_Actor_RemoteRole = nullptr;
std::map<int, bool> ByteProperty_Engine_Actor_RemoteRole_initial;
UProperty* ByteProperty_Engine_Actor_Role = nullptr;
std::map<int, bool> ByteProperty_Engine_Actor_Role_initial;
UProperty* BoolProperty_Engine_Actor_bNetOwner = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bNetOwner_initial;
UProperty* BoolProperty_Engine_Actor_bTearOff = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bTearOff_initial;
UProperty* FloatProperty_Engine_Actor_DrawScale = nullptr;
std::map<int, bool> FloatProperty_Engine_Actor_DrawScale_initial;
UProperty* ByteProperty_Engine_Actor_ReplicatedCollisionType = nullptr;
std::map<int, bool> ByteProperty_Engine_Actor_ReplicatedCollisionType_initial;
UProperty* BoolProperty_Engine_Actor_bCollideActors = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bCollideActors_initial;
UProperty* BoolProperty_Engine_Actor_bCollideWorld = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bCollideWorld_initial;
UProperty* BoolProperty_Engine_Actor_bBlockActors = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bBlockActors_initial;
UProperty* BoolProperty_Engine_Actor_bProjTarget = nullptr;
std::map<int, bool> BoolProperty_Engine_Actor_bProjTarget_initial;
UProperty* ObjectProperty_Engine_Actor_Instigator = nullptr;
std::map<int, bool> ObjectProperty_Engine_Actor_Instigator_initial;
UProperty* ObjectProperty_Engine_Actor_Owner = nullptr;
std::map<int, bool> ObjectProperty_Engine_Actor_Owner_initial;
UProperty* BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying = nullptr;
std::map<int, bool> BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying_initial;
UProperty* FloatProperty_Engine_CameraActor_AspectRatio = nullptr;
std::map<int, bool> FloatProperty_Engine_CameraActor_AspectRatio_initial;
UProperty* FloatProperty_Engine_CameraActor_FOVAngle = nullptr;
std::map<int, bool> FloatProperty_Engine_CameraActor_FOVAngle_initial;
UProperty* ObjectProperty_Engine_Controller_Pawn = nullptr;
std::map<int, bool> ObjectProperty_Engine_Controller_Pawn_initial;
UProperty* ObjectProperty_Engine_Controller_PlayerReplicationInfo = nullptr;
std::map<int, bool> ObjectProperty_Engine_Controller_PlayerReplicationInfo_initial;
UProperty* BoolProperty_Engine_CrowdAttractor_bAttractorEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_CrowdAttractor_bAttractorEnabled_initial;
UProperty* IntProperty_Engine_CrowdReplicationActor_DestroyAllCount = nullptr;
std::map<int, bool> IntProperty_Engine_CrowdReplicationActor_DestroyAllCount_initial;
UProperty* ObjectProperty_Engine_CrowdReplicationActor_Spawner = nullptr;
std::map<int, bool> ObjectProperty_Engine_CrowdReplicationActor_Spawner_initial;
UProperty* BoolProperty_Engine_CrowdReplicationActor_bSpawningActive = nullptr;
std::map<int, bool> BoolProperty_Engine_CrowdReplicationActor_bSpawningActive_initial;
UProperty* ClassProperty_Engine_DroppedPickup_InventoryClass = nullptr;
std::map<int, bool> ClassProperty_Engine_DroppedPickup_InventoryClass_initial;
UProperty* BoolProperty_Engine_DroppedPickup_bFadeOut = nullptr;
std::map<int, bool> BoolProperty_Engine_DroppedPickup_bFadeOut_initial;
UProperty* ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial = nullptr;
std::map<int, bool> ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial_initial;
UProperty* ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh = nullptr;
std::map<int, bool> ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh_initial;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation = nullptr;
std::map<int, bool> StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation_initial;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D = nullptr;
std::map<int, bool> StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D_initial;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation = nullptr;
std::map<int, bool> StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation_initial;
UProperty* BoolProperty_Engine_DynamicSMActor_bForceStaticDecals = nullptr;
std::map<int, bool> BoolProperty_Engine_DynamicSMActor_bForceStaticDecals_initial;
UProperty* BoolProperty_Engine_Emitter_bCurrentlyActive = nullptr;
std::map<int, bool> BoolProperty_Engine_Emitter_bCurrentlyActive_initial;
UProperty* ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate = nullptr;
std::map<int, bool> ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate_initial;
UProperty* BoolProperty_Engine_FluidInfluenceActor_bActive = nullptr;
std::map<int, bool> BoolProperty_Engine_FluidInfluenceActor_bActive_initial;
UProperty* BoolProperty_Engine_FluidInfluenceActor_bToggled = nullptr;
std::map<int, bool> BoolProperty_Engine_FluidInfluenceActor_bToggled_initial;
UProperty* BoolProperty_Engine_FogVolumeDensityInfo_bEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_FogVolumeDensityInfo_bEnabled_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_MatchID = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_MatchID_initial;
UProperty* ObjectProperty_Engine_GameReplicationInfo_Winner = nullptr;
std::map<int, bool> ObjectProperty_Engine_GameReplicationInfo_Winner_initial;
UProperty* BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun = nullptr;
std::map<int, bool> BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun_initial;
UProperty* BoolProperty_Engine_GameReplicationInfo_bMatchIsOver = nullptr;
std::map<int, bool> BoolProperty_Engine_GameReplicationInfo_bMatchIsOver_initial;
UProperty* BoolProperty_Engine_GameReplicationInfo_bStopCountDown = nullptr;
std::map<int, bool> BoolProperty_Engine_GameReplicationInfo_bStopCountDown_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_RemainingMinute = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_RemainingMinute_initial;
UProperty* StrProperty_Engine_GameReplicationInfo_AdminEmail = nullptr;
std::map<int, bool> StrProperty_Engine_GameReplicationInfo_AdminEmail_initial;
UProperty* StrProperty_Engine_GameReplicationInfo_AdminName = nullptr;
std::map<int, bool> StrProperty_Engine_GameReplicationInfo_AdminName_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_ElapsedTime = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_ElapsedTime_initial;
UProperty* ClassProperty_Engine_GameReplicationInfo_GameClass = nullptr;
std::map<int, bool> ClassProperty_Engine_GameReplicationInfo_GameClass_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_GoalScore = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_GoalScore_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_MaxLives = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_MaxLives_initial;
UProperty* StrProperty_Engine_GameReplicationInfo_MessageOfTheDay = nullptr;
std::map<int, bool> StrProperty_Engine_GameReplicationInfo_MessageOfTheDay_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_RemainingTime = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_RemainingTime_initial;
UProperty* StrProperty_Engine_GameReplicationInfo_ServerName = nullptr;
std::map<int, bool> StrProperty_Engine_GameReplicationInfo_ServerName_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_ServerRegion = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_ServerRegion_initial;
UProperty* StrProperty_Engine_GameReplicationInfo_ShortName = nullptr;
std::map<int, bool> StrProperty_Engine_GameReplicationInfo_ShortName_initial;
UProperty* IntProperty_Engine_GameReplicationInfo_TimeLimit = nullptr;
std::map<int, bool> IntProperty_Engine_GameReplicationInfo_TimeLimit_initial;
UProperty* BoolProperty_Engine_GameReplicationInfo_bIsArbitrated = nullptr;
std::map<int, bool> BoolProperty_Engine_GameReplicationInfo_bIsArbitrated_initial;
UProperty* BoolProperty_Engine_GameReplicationInfo_bTrackStats = nullptr;
std::map<int, bool> BoolProperty_Engine_GameReplicationInfo_bTrackStats_initial;
UProperty* BoolProperty_Engine_HeightFog_bEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_HeightFog_bEnabled_initial;
UProperty* ObjectProperty_Engine_Inventory_InvManager = nullptr;
std::map<int, bool> ObjectProperty_Engine_Inventory_InvManager_initial;
UProperty* ObjectProperty_Engine_Inventory_Inventory = nullptr;
std::map<int, bool> ObjectProperty_Engine_Inventory_Inventory_initial;
UProperty* ObjectProperty_Engine_InventoryManager_InventoryChain = nullptr;
std::map<int, bool> ObjectProperty_Engine_InventoryManager_InventoryChain_initial;
UProperty* StructProperty_Engine_KActor_RBState = nullptr;
std::map<int, bool> StructProperty_Engine_KActor_RBState_initial;
UProperty* StructProperty_Engine_KActor_ReplicatedDrawScale3D = nullptr;
std::map<int, bool> StructProperty_Engine_KActor_ReplicatedDrawScale3D_initial;
UProperty* BoolProperty_Engine_KActor_bWakeOnLevelStart = nullptr;
std::map<int, bool> BoolProperty_Engine_KActor_bWakeOnLevelStart_initial;
UProperty* ObjectProperty_Engine_KAsset_ReplicatedMesh = nullptr;
std::map<int, bool> ObjectProperty_Engine_KAsset_ReplicatedMesh_initial;
UProperty* ObjectProperty_Engine_KAsset_ReplicatedPhysAsset = nullptr;
std::map<int, bool> ObjectProperty_Engine_KAsset_ReplicatedPhysAsset_initial;
UProperty* BoolProperty_Engine_LensFlareSource_bCurrentlyActive = nullptr;
std::map<int, bool> BoolProperty_Engine_LensFlareSource_bCurrentlyActive_initial;
UProperty* BoolProperty_Engine_Light_bEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_Light_bEnabled_initial;
UProperty* ObjectProperty_Engine_MatineeActor_InterpAction = nullptr;
std::map<int, bool> ObjectProperty_Engine_MatineeActor_InterpAction_initial;
UProperty* FloatProperty_Engine_MatineeActor_PlayRate = nullptr;
std::map<int, bool> FloatProperty_Engine_MatineeActor_PlayRate_initial;
UProperty* FloatProperty_Engine_MatineeActor_Position = nullptr;
std::map<int, bool> FloatProperty_Engine_MatineeActor_Position_initial;
UProperty* BoolProperty_Engine_MatineeActor_bIsPlaying = nullptr;
std::map<int, bool> BoolProperty_Engine_MatineeActor_bIsPlaying_initial;
UProperty* BoolProperty_Engine_MatineeActor_bPaused = nullptr;
std::map<int, bool> BoolProperty_Engine_MatineeActor_bPaused_initial;
UProperty* BoolProperty_Engine_MatineeActor_bReversePlayback = nullptr;
std::map<int, bool> BoolProperty_Engine_MatineeActor_bReversePlayback_initial;
UProperty* BoolProperty_Engine_NxForceField_bForceActive = nullptr;
std::map<int, bool> BoolProperty_Engine_NxForceField_bForceActive_initial;
UProperty* ObjectProperty_Engine_Pawn_DrivenVehicle = nullptr;
std::map<int, bool> ObjectProperty_Engine_Pawn_DrivenVehicle_initial;
UProperty* StructProperty_Engine_Pawn_FlashLocation = nullptr;
std::map<int, bool> StructProperty_Engine_Pawn_FlashLocation_initial;
UProperty* IntProperty_Engine_Pawn_Health = nullptr;
std::map<int, bool> IntProperty_Engine_Pawn_Health_initial;
UProperty* ClassProperty_Engine_Pawn_HitDamageType = nullptr;
std::map<int, bool> ClassProperty_Engine_Pawn_HitDamageType_initial;
UProperty* ObjectProperty_Engine_Pawn_PlayerReplicationInfo = nullptr;
std::map<int, bool> ObjectProperty_Engine_Pawn_PlayerReplicationInfo_initial;
UProperty* StructProperty_Engine_Pawn_TakeHitLocation = nullptr;
std::map<int, bool> StructProperty_Engine_Pawn_TakeHitLocation_initial;
UProperty* BoolProperty_Engine_Pawn_bIsWalking = nullptr;
std::map<int, bool> BoolProperty_Engine_Pawn_bIsWalking_initial;
UProperty* BoolProperty_Engine_Pawn_bSimulateGravity = nullptr;
std::map<int, bool> BoolProperty_Engine_Pawn_bSimulateGravity_initial;
UProperty* FloatProperty_Engine_Pawn_AccelRate = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_AccelRate_initial;
UProperty* FloatProperty_Engine_Pawn_AirControl = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_AirControl_initial;
UProperty* FloatProperty_Engine_Pawn_AirSpeed = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_AirSpeed_initial;
UProperty* ObjectProperty_Engine_Pawn_Controller = nullptr;
std::map<int, bool> ObjectProperty_Engine_Pawn_Controller_initial;
UProperty* FloatProperty_Engine_Pawn_GroundSpeed = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_GroundSpeed_initial;
UProperty* ObjectProperty_Engine_Pawn_InvManager = nullptr;
std::map<int, bool> ObjectProperty_Engine_Pawn_InvManager_initial;
UProperty* FloatProperty_Engine_Pawn_JumpZ = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_JumpZ_initial;
UProperty* FloatProperty_Engine_Pawn_WaterSpeed = nullptr;
std::map<int, bool> FloatProperty_Engine_Pawn_WaterSpeed_initial;
UProperty* ByteProperty_Engine_Pawn_FiringMode = nullptr;
std::map<int, bool> ByteProperty_Engine_Pawn_FiringMode_initial;
UProperty* ByteProperty_Engine_Pawn_FlashCount = nullptr;
std::map<int, bool> ByteProperty_Engine_Pawn_FlashCount_initial;
UProperty* BoolProperty_Engine_Pawn_bIsCrouched = nullptr;
std::map<int, bool> BoolProperty_Engine_Pawn_bIsCrouched_initial;
UProperty* StructProperty_Engine_Pawn_TearOffMomentum = nullptr;
std::map<int, bool> StructProperty_Engine_Pawn_TearOffMomentum_initial;
UProperty* ByteProperty_Engine_Pawn_RemoteViewPitch = nullptr;
std::map<int, bool> ByteProperty_Engine_Pawn_RemoteViewPitch_initial;
UProperty* ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = nullptr;
std::map<int, bool> ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate_initial;
UProperty* BoolProperty_Engine_PickupFactory_bPickupHidden = nullptr;
std::map<int, bool> BoolProperty_Engine_PickupFactory_bPickupHidden_initial;
UProperty* ClassProperty_Engine_PickupFactory_InventoryType = nullptr;
std::map<int, bool> ClassProperty_Engine_PickupFactory_InventoryType_initial;
UProperty* FloatProperty_Engine_PlayerController_TargetEyeHeight = nullptr;
std::map<int, bool> FloatProperty_Engine_PlayerController_TargetEyeHeight_initial;
UProperty* StructProperty_Engine_PlayerController_TargetViewRotation = nullptr;
std::map<int, bool> StructProperty_Engine_PlayerController_TargetViewRotation_initial;
UProperty* FloatProperty_Engine_PlayerReplicationInfo_Deaths = nullptr;
std::map<int, bool> FloatProperty_Engine_PlayerReplicationInfo_Deaths_initial;
UProperty* StrProperty_Engine_PlayerReplicationInfo_PlayerAlias = nullptr;
std::map<int, bool> StrProperty_Engine_PlayerReplicationInfo_PlayerAlias_initial;
UProperty* ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint = nullptr;
std::map<int, bool> ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint_initial;
UProperty* StrProperty_Engine_PlayerReplicationInfo_PlayerName = nullptr;
std::map<int, bool> StrProperty_Engine_PlayerReplicationInfo_PlayerName_initial;
UProperty* IntProperty_Engine_PlayerReplicationInfo_PlayerSkill = nullptr;
std::map<int, bool> IntProperty_Engine_PlayerReplicationInfo_PlayerSkill_initial;
UProperty* FloatProperty_Engine_PlayerReplicationInfo_Score = nullptr;
std::map<int, bool> FloatProperty_Engine_PlayerReplicationInfo_Score_initial;
UProperty* IntProperty_Engine_PlayerReplicationInfo_StartTime = nullptr;
std::map<int, bool> IntProperty_Engine_PlayerReplicationInfo_StartTime_initial;
UProperty* ObjectProperty_Engine_PlayerReplicationInfo_Team = nullptr;
std::map<int, bool> ObjectProperty_Engine_PlayerReplicationInfo_Team_initial;
UProperty* StructProperty_Engine_PlayerReplicationInfo_UniqueId = nullptr;
std::map<int, bool> StructProperty_Engine_PlayerReplicationInfo_UniqueId_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bAdmin = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bAdmin_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bHasFlag = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bHasFlag_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsFemale = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bIsFemale_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer_initial;
UProperty* ByteProperty_Engine_PlayerReplicationInfo_PacketLoss = nullptr;
std::map<int, bool> ByteProperty_Engine_PlayerReplicationInfo_PacketLoss_initial;
UProperty* ByteProperty_Engine_PlayerReplicationInfo_Ping = nullptr;
std::map<int, bool> ByteProperty_Engine_PlayerReplicationInfo_Ping_initial;
UProperty* IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex = nullptr;
std::map<int, bool> IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex_initial;
UProperty* IntProperty_Engine_PlayerReplicationInfo_PlayerID = nullptr;
std::map<int, bool> IntProperty_Engine_PlayerReplicationInfo_PlayerID_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bBot = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bBot_initial;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsInactive = nullptr;
std::map<int, bool> BoolProperty_Engine_PlayerReplicationInfo_bIsInactive_initial;
UProperty* BoolProperty_Engine_PostProcessVolume_bEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_PostProcessVolume_bEnabled_initial;
UProperty* FloatProperty_Engine_Projectile_MaxSpeed = nullptr;
std::map<int, bool> FloatProperty_Engine_Projectile_MaxSpeed_initial;
UProperty* FloatProperty_Engine_Projectile_Speed = nullptr;
std::map<int, bool> FloatProperty_Engine_Projectile_Speed_initial;
UProperty* BoolProperty_Engine_RB_CylindricalForceActor_bForceActive = nullptr;
std::map<int, bool> BoolProperty_Engine_RB_CylindricalForceActor_bForceActive_initial;
UProperty* ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount = nullptr;
std::map<int, bool> ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount_initial;
UProperty* BoolProperty_Engine_RB_RadialForceActor_bForceActive = nullptr;
std::map<int, bool> BoolProperty_Engine_RB_RadialForceActor_bForceActive_initial;
UProperty* ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount = nullptr;
std::map<int, bool> ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount_initial;
UProperty* FloatProperty_Engine_SVehicle_MaxSpeed = nullptr;
std::map<int, bool> FloatProperty_Engine_SVehicle_MaxSpeed_initial;
UProperty* StructProperty_Engine_SVehicle_VState = nullptr;
std::map<int, bool> StructProperty_Engine_SVehicle_VState_initial;
UProperty* ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial = nullptr;
std::map<int, bool> ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial_initial;
UProperty* ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh = nullptr;
std::map<int, bool> ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh_initial;
UProperty* FloatProperty_Engine_TeamInfo_Score = nullptr;
std::map<int, bool> FloatProperty_Engine_TeamInfo_Score_initial;
UProperty* IntProperty_Engine_TeamInfo_TeamIndex = nullptr;
std::map<int, bool> IntProperty_Engine_TeamInfo_TeamIndex_initial;
UProperty* StrProperty_Engine_TeamInfo_TeamName = nullptr;
std::map<int, bool> StrProperty_Engine_TeamInfo_TeamName_initial;
UProperty* StrProperty_Engine_Teleporter_URL = nullptr;
std::map<int, bool> StrProperty_Engine_Teleporter_URL_initial;
UProperty* BoolProperty_Engine_Teleporter_bEnabled = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bEnabled_initial;
UProperty* StructProperty_Engine_Teleporter_TargetVelocity = nullptr;
std::map<int, bool> StructProperty_Engine_Teleporter_TargetVelocity_initial;
UProperty* BoolProperty_Engine_Teleporter_bChangesVelocity = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bChangesVelocity_initial;
UProperty* BoolProperty_Engine_Teleporter_bChangesYaw = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bChangesYaw_initial;
UProperty* BoolProperty_Engine_Teleporter_bReversesX = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bReversesX_initial;
UProperty* BoolProperty_Engine_Teleporter_bReversesY = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bReversesY_initial;
UProperty* BoolProperty_Engine_Teleporter_bReversesZ = nullptr;
std::map<int, bool> BoolProperty_Engine_Teleporter_bReversesZ_initial;
UProperty* BoolProperty_Engine_Vehicle_bDriving = nullptr;
std::map<int, bool> BoolProperty_Engine_Vehicle_bDriving_initial;
UProperty* ObjectProperty_Engine_Vehicle_Driver = nullptr;
std::map<int, bool> ObjectProperty_Engine_Vehicle_Driver_initial;
UProperty* ObjectProperty_Engine_WorldInfo_Pauser = nullptr;
std::map<int, bool> ObjectProperty_Engine_WorldInfo_Pauser_initial;
UProperty* StructProperty_Engine_WorldInfo_ReplicatedMusicTrack = nullptr;
std::map<int, bool> StructProperty_Engine_WorldInfo_ReplicatedMusicTrack_initial;
UProperty* FloatProperty_Engine_WorldInfo_TimeDilation = nullptr;
std::map<int, bool> FloatProperty_Engine_WorldInfo_TimeDilation_initial;
UProperty* FloatProperty_Engine_WorldInfo_WorldGravityZ = nullptr;
std::map<int, bool> FloatProperty_Engine_WorldInfo_WorldGravityZ_initial;
UProperty* BoolProperty_Engine_WorldInfo_bHighPriorityLoading = nullptr;
std::map<int, bool> BoolProperty_Engine_WorldInfo_bHighPriorityLoading_initial;
UProperty* ByteProperty_TgGame_TgChestActor_r_eChestState = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgChestActor_r_eChestState_initial;
UProperty* BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive_initial;
UProperty* BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired_initial;
UProperty* IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning_initial;
UProperty* IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount_initial;
UProperty* BoolProperty_TgGame_TgDeployable_r_bDelayDeployed = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDeployable_r_bDelayDeployed_initial;
UProperty* IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt_initial;
UProperty* ObjectProperty_TgGame_TgDeployable_r_DRI = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgDeployable_r_DRI_initial;
UProperty* BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy_initial;
UProperty* BoolProperty_TgGame_TgDeployable_r_bTakeDamage = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDeployable_r_bTakeDamage_initial;
UProperty* FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius_initial;
UProperty* FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime_initial;
UProperty* IntProperty_TgGame_TgDeployable_r_nDeployableId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeployable_r_nDeployableId_initial;
UProperty* IntProperty_TgGame_TgDeployable_r_nPhysicalType = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeployable_r_nPhysicalType_initial;
UProperty* IntProperty_TgGame_TgDeployable_r_nTickingTime = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeployable_r_nTickingTime_initial;
UProperty* ObjectProperty_TgGame_TgDeployable_r_Owner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgDeployable_r_Owner_initial;
UProperty* IntProperty_TgGame_TgDeployable_r_nOwnerFireMode = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDeployable_r_nOwnerFireMode_initial;
UProperty* ByteProperty_TgGame_TgDevice_CurrentFireMode = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgDevice_CurrentFireMode_initial;
UProperty* BoolProperty_TgGame_TgDevice_r_bIsStealthDevice = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDevice_r_bIsStealthDevice_initial;
UProperty* ByteProperty_TgGame_TgDevice_r_eEquippedAt = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgDevice_r_eEquippedAt_initial;
UProperty* IntProperty_TgGame_TgDevice_r_nInventoryId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDevice_r_nInventoryId_initial;
UProperty* IntProperty_TgGame_TgDevice_r_nMeleeComboSeed = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDevice_r_nMeleeComboSeed_initial;
UProperty* BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath_initial;
UProperty* BoolProperty_TgGame_TgDevice_r_bConsumedOnUse = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDevice_r_bConsumedOnUse_initial;
UProperty* IntProperty_TgGame_TgDevice_r_nDeviceId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDevice_r_nDeviceId_initial;
UProperty* IntProperty_TgGame_TgDevice_r_nDeviceInstanceId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDevice_r_nDeviceInstanceId_initial;
UProperty* IntProperty_TgGame_TgDevice_r_nQualityValueId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDevice_r_nQualityValueId_initial;
UProperty* BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring_initial;
UProperty* BoolProperty_TgGame_TgDoor_r_bOpen = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgDoor_r_bOpen_initial;
UProperty* ByteProperty_TgGame_TgDoorMarker_r_eStatus = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgDoorMarker_r_eStatus_initial;
UProperty* IntProperty_TgGame_TgDroppedItem_r_nItemId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDroppedItem_r_nItemId_initial;
UProperty* IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId_initial;
UProperty* ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory_initial;
UProperty* StrProperty_TgGame_TgDynamicSMActor_m_sAssembly = nullptr;
std::map<int, bool> StrProperty_TgGame_TgDynamicSMActor_m_sAssembly_initial;
UProperty* ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager_initial;
UProperty* IntProperty_TgGame_TgDynamicSMActor_r_nHealth = nullptr;
std::map<int, bool> IntProperty_TgGame_TgDynamicSMActor_r_nHealth_initial;
UProperty* StructProperty_TgGame_TgEffectManager_r_EventQueue = nullptr;
std::map<int, bool> StructProperty_TgGame_TgEffectManager_r_EventQueue_initial;
UProperty* StructProperty_TgGame_TgEffectManager_r_ManagedEffectList = nullptr;
std::map<int, bool> StructProperty_TgGame_TgEffectManager_r_ManagedEffectList_initial;
UProperty* ObjectProperty_TgGame_TgEffectManager_r_Owner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgEffectManager_r_Owner_initial;
UProperty* BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify_initial;
UProperty* IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount_initial;
UProperty* IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex = nullptr;
std::map<int, bool> IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex_initial;
UProperty* NameProperty_TgGame_TgEmitter_BoneName = nullptr;
std::map<int, bool> NameProperty_TgGame_TgEmitter_BoneName_initial;
UProperty* ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition_initial;
UProperty* ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce_initial;
UProperty* ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager_initial;
UProperty* IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier = nullptr;
std::map<int, bool> IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier_initial;
UProperty* FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius_initial;
UProperty* ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType = nullptr;
std::map<int, bool> ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType_initial;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo = nullptr;
std::map<int, bool> StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo_initial;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation_initial;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum = nullptr;
std::map<int, bool> StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum_initial;
UProperty* IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId_initial;
UProperty* StrProperty_TgGame_TgInterpActor_r_sCurrState = nullptr;
std::map<int, bool> StrProperty_TgGame_TgInterpActor_r_sCurrState_initial;
UProperty* IntProperty_TgGame_TgInventoryManager_r_ItemCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgInventoryManager_r_ItemCount_initial;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest = nullptr;
std::map<int, bool> IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest_initial;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nFailCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgKismetTestActor_r_nFailCount_initial;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nPassCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgKismetTestActor_r_nPassCount_initial;
UProperty* BoolProperty_TgGame_TgLevelCamera_r_bEnabled = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgLevelCamera_r_bEnabled_initial;
UProperty* ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment_initial;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce_initial;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsActive = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgMissionObjective_r_bIsActive_initial;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsLocked = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgMissionObjective_r_bIsLocked_initial;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsPending = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgMissionObjective_r_bIsPending_initial;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition_initial;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eStatus = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgMissionObjective_r_eStatus_initial;
UProperty* FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime_initial;
UProperty* FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime_initial;
UProperty* IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce = nullptr;
std::map<int, bool> IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce_initial;
UProperty* IntProperty_TgGame_TgMissionObjective_nObjectiveId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgMissionObjective_nObjectiveId_initial;
UProperty* IntProperty_TgGame_TgMissionObjective_nPriority = nullptr;
std::map<int, bool> IntProperty_TgGame_TgMissionObjective_nPriority_initial;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole_initial;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState_initial;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition_initial;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot_initial;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo_initial;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor_initial;
UProperty* FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate_initial;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective_initial;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers_initial;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots_initial;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders_initial;
UProperty* ByteProperty_TgGame_TgObjectiveAssignment_r_eState = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgObjectiveAssignment_r_eState_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsBot = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsBot_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHenchman = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsHenchman_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nAllianceId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nAllianceId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nBotRankValueId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nBotRankValueId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashEvent = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nFlashEvent_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashFireInfo = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nFlashFireInfo_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashQueIndex = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nFlashQueIndex_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nPawnId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nPawnId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nPhysicalType = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nPhysicalType_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nPreyProfileType = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nPreyProfileType_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nProfileId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nProfileId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nProfileTypeValueId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nProfileTypeValueId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nSoundGroupId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nSoundGroupId_initial;
UProperty* StructProperty_TgGame_TgPawn_r_vFlashLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_vFlashLocation_initial;
UProperty* StructProperty_TgGame_TgPawn_r_vFlashRayDir = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_vFlashRayDir_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_vFlashRefireTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_vFlashRefireTime_initial;
UProperty* IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack_initial;
UProperty* StructProperty_TgGame_TgPawn_r_EquipDeviceInfo = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_EquipDeviceInfo_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_bMadeSound = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_bMadeSound_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_eDesiredInHand = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_eDesiredInHand_initial;
UProperty* IntProperty_TgGame_TgPawn_r_eEquippedInHandMode = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_eEquippedInHandMode_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nReplicateHit = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nReplicateHit_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_ControlPawn = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_ControlPawn_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol_initial;
UProperty* StructProperty_TgGame_TgPawn_r_ScannerSettings = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_ScannerSettings_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_UIClockState = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_UIClockState_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_UIClockTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_UIClockTime_initial;
UProperty* IntProperty_TgGame_TgPawn_r_UITextBox1MessageID = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_UITextBox1MessageID_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_UITextBox1Packet = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_UITextBox1Packet_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_UITextBox1Time = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_UITextBox1Time_initial;
UProperty* IntProperty_TgGame_TgPawn_r_UITextBox2MessageID = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_UITextBox2MessageID_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_UITextBox2Packet = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_UITextBox2Packet_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_UITextBox2Time = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_UITextBox2Time_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bDisableAllDevices = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bDisableAllDevices_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableCrafting = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bEnableCrafting_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableEquip = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bEnableEquip_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableSkills = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bEnableSkills_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bInCombatFlag = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bInCombatFlag_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fMaxControlRange = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fMaxControlRange_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fMaxPowerPool = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fMaxPowerPool_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fSkillRating = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fSkillRating_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nCurrency = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nCurrency_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nHZPoints = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nHZPoints_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nRestDeviceSlot = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nRestDeviceSlot_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nToken = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nToken_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nXp = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nXp_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_DistanceToPushback = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_DistanceToPushback_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_EffectManager = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_EffectManager_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_FlightAcceleration = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_FlightAcceleration_initial;
UProperty* StructProperty_TgGame_TgPawn_r_HangingRotation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_HangingRotation_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_Owner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_Owner_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_Pet = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_Pet_initial;
UProperty* StructProperty_TgGame_TgPawn_r_PlayAnimation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_PlayAnimation_initial;
UProperty* StructProperty_TgGame_TgPawn_r_PushbackDirection = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_PushbackDirection_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_Target = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_Target_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_TargetActor = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_TargetActor_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugDestination = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_aDebugDestination_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugNextNav = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_aDebugNextNav_initial;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugTarget = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_r_aDebugTarget_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_bAimType = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_bAimType_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bAimingMode = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bAimingMode_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bCallingForHelp = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bCallingForHelp_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsAFK = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsAFK_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsCrafting = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsCrafting_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsCrewing = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsCrewing_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsDecoy = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsDecoy_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHacked = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsHacked_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHacking = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsHacking_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHanging = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsHanging_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsRappelling = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsRappelling_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsStealthed = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bIsStealthed_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bResistTagging = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bResistTagging_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bTagEnemy = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bTagEnemy_initial;
UProperty* BoolProperty_TgGame_TgPawn_r_bUsingBinoculars = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_r_bUsingBinoculars_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_eCurrentStunType = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_eCurrentStunType_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_eDeathReason = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_eDeathReason_initial;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteLength = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_eEmoteLength_initial;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteRepnotify = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_eEmoteRepnotify_initial;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteUpdate = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_eEmoteUpdate_initial;
UProperty* ByteProperty_TgGame_TgPawn_r_ePosture = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_r_ePosture_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fDeployRate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fDeployRate_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fGravityZModifier = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fGravityZModifier_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fPostureRateScale = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fPostureRateScale_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier_initial;
UProperty* FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime_initial;
UProperty* IntProperty_TgGame_TgPawn_r_fWeightBonus = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_fWeightBonus_initial;
UProperty* IntProperty_TgGame_TgPawn_r_iKnockDownFlash = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_iKnockDownFlash_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nApplyStealth = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nApplyStealth_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nBotSoundCueId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nBotSoundCueId_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugAggroRange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nDebugAggroRange_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugFOV = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nDebugFOV_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugHearingRange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nDebugHearingRange_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugSightRange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nDebugSightRange_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nHealthMaximum = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nHealthMaximum_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nPhase = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nPhase_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nPitchOffset = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nPitchOffset_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nReplicateDying = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nReplicateDying_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nResetCharacter = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nResetCharacter_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nSensorAlertLevel = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nSensorAlertLevel_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nShieldHealthMax = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nShieldHealthMax_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nSilentMode = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nSilentMode_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthAggroRange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nStealthAggroRange_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthDisabled = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nStealthDisabled_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthSensorRange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nStealthSensorRange_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthTypeCode = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nStealthTypeCode_initial;
UProperty* IntProperty_TgGame_TgPawn_r_nYawOffset = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_r_nYawOffset_initial;
UProperty* StrProperty_TgGame_TgPawn_r_sDebugAction = nullptr;
std::map<int, bool> StrProperty_TgGame_TgPawn_r_sDebugAction_initial;
UProperty* StrProperty_TgGame_TgPawn_r_sDebugName = nullptr;
std::map<int, bool> StrProperty_TgGame_TgPawn_r_sDebugName_initial;
UProperty* StrProperty_TgGame_TgPawn_r_sFactory = nullptr;
std::map<int, bool> StrProperty_TgGame_TgPawn_r_sFactory_initial;
UProperty* StructProperty_TgGame_TgPawn_r_vDown = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_r_vDown_initial;
UProperty* BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed_initial;
UProperty* ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType_initial;
UProperty* StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly_initial;
UProperty* ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn_initial;
UProperty* IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer_initial;
UProperty* IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings_initial;
UProperty* StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly_initial;
UProperty* ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nItemProfileId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nItemProfileId_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nMorphSettings = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nMorphSettings_initial;
UProperty* ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet_initial;
UProperty* FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset_initial;
UProperty* FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ_initial;
UProperty* BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId_initial;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId_initial;
UProperty* BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding_initial;
UProperty* ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan_initial;
UProperty* FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct_initial;
UProperty* ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection_initial;
UProperty* BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed_initial;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime_initial;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs_initial;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime_initial;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT_initial;
UProperty* IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId_initial;
UProperty* ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer_initial;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects_initial;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bGMInvisible = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPlayerController_r_bGMInvisible_initial;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot_initial;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation_initial;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bRove = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgPlayerController_r_bRove_initial;
UProperty* StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation_initial;
UProperty* ObjectProperty_TgGame_TgProj_Missile_r_aSeeking = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgProj_Missile_r_aSeeking_initial;
UProperty* StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation_initial;
UProperty* IntProperty_TgGame_TgProj_Missile_r_nNumBounces = nullptr;
std::map<int, bool> IntProperty_TgGame_TgProj_Missile_r_nNumBounces_initial;
UProperty* ByteProperty_TgGame_TgProj_Rocket_FlockIndex = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgProj_Rocket_FlockIndex_initial;
UProperty* BoolProperty_TgGame_TgProj_Rocket_bCurl = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgProj_Rocket_bCurl_initial;
UProperty* ObjectProperty_TgGame_TgProjectile_r_Owner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgProjectile_r_Owner_initial;
UProperty* FloatProperty_TgGame_TgProjectile_r_fAccelRate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgProjectile_r_fAccelRate_initial;
UProperty* FloatProperty_TgGame_TgProjectile_r_fDuration = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgProjectile_r_fDuration_initial;
UProperty* FloatProperty_TgGame_TgProjectile_r_fRange = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgProjectile_r_fRange_initial;
UProperty* IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId_initial;
UProperty* IntProperty_TgGame_TgProjectile_r_nProjectileId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgProjectile_r_nProjectileId_initial;
UProperty* StructProperty_TgGame_TgProjectile_r_vSpawnLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgProjectile_r_vSpawnLocation_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc_initial;
UProperty* StrProperty_TgGame_TgRepInfo_Beacon_r_nName = nullptr;
std::map<int, bool> StrProperty_TgGame_TgRepInfo_Beacon_r_nName_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner_initial;
UProperty* FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap_initial;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier_initial;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime_initial;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber_initial;
UProperty* ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders_initial;
UProperty* ByteProperty_TgGame_TgRepInfo_Game_r_GameType = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgRepInfo_Game_r_GameType_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives_initial;
UProperty* IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_Scores = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_Scores_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_Player_r_bDropped = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_Player_r_bDropped_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_eBonusType = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_eBonusType_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nLevel = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nLevel_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nProfileId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nProfileId_initial;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId_initial;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName = nullptr;
std::map<int, bool> StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName_initial;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName = nullptr;
std::map<int, bool> StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName_initial;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName = nullptr;
std::map<int, bool> StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName_initial;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats = nullptr;
std::map<int, bool> StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective_initial;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment_initial;
UProperty* BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned = nullptr;
std::map<int, bool> BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned_initial;
UProperty* ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition_initial;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount_initial;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId_initial;
UProperty* FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers_initial;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths_initial;
UProperty* ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce_initial;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId = nullptr;
std::map<int, bool> IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId_initial;
UProperty* FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius_initial;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier_initial;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce_initial;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce_initial;
UProperty* ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget_initial;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon_initial;
UProperty* IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = nullptr;
std::map<int, bool> IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed_initial;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder_initial;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo_initial;
UProperty* ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus_initial;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce = nullptr;
std::map<int, bool> ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce_initial;
UProperty* ByteProperty_TgGame_TgTimerManager_r_byEventQue = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgTimerManager_r_byEventQue_initial;
UProperty* ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = nullptr;
std::map<int, bool> ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex_initial;
UProperty* FloatProperty_TgGame_TgTimerManager_r_fRemaining = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgTimerManager_r_fRemaining_initial;
UProperty* FloatProperty_TgGame_TgTimerManager_r_fStartTime = nullptr;
std::map<int, bool> FloatProperty_TgGame_TgTimerManager_r_fStartTime_initial;


static void ResolveRepListProperties() {
	ObjectProperty_Engine_Actor_Base = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Actor.Base");
	ByteProperty_Engine_Actor_Physics = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Actor.Physics");
	StructProperty_Engine_Actor_Velocity = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.Actor.Velocity");
	ByteProperty_Engine_Actor_RemoteRole = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Actor.RemoteRole");
	ByteProperty_Engine_Actor_Role = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Actor.Role");
	BoolProperty_Engine_Actor_bNetOwner = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bNetOwner");
	BoolProperty_Engine_Actor_bTearOff = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bTearOff");
	FloatProperty_Engine_Actor_DrawScale = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Actor.DrawScale");
	ByteProperty_Engine_Actor_ReplicatedCollisionType = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Actor.ReplicatedCollisionType");
	BoolProperty_Engine_Actor_bCollideActors = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bCollideActors");
	BoolProperty_Engine_Actor_bCollideWorld = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bCollideWorld");
	BoolProperty_Engine_Actor_bBlockActors = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bBlockActors");
	BoolProperty_Engine_Actor_bProjTarget = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Actor.bProjTarget");
	ObjectProperty_Engine_Actor_Instigator = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Actor.Instigator");
	ObjectProperty_Engine_Actor_Owner = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Actor.Owner");
	BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.AmbientSoundSimpleToggleable.bCurrentlyPlaying");
	FloatProperty_Engine_CameraActor_AspectRatio = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.CameraActor.AspectRatio");
	FloatProperty_Engine_CameraActor_FOVAngle = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.CameraActor.FOVAngle");
	ObjectProperty_Engine_Controller_Pawn = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Controller.Pawn");
	ObjectProperty_Engine_Controller_PlayerReplicationInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Controller.PlayerReplicationInfo");
	BoolProperty_Engine_CrowdAttractor_bAttractorEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.CrowdAttractor.bAttractorEnabled");
	IntProperty_Engine_CrowdReplicationActor_DestroyAllCount = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.CrowdReplicationActor.DestroyAllCount");
	ObjectProperty_Engine_CrowdReplicationActor_Spawner = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.CrowdReplicationActor.Spawner");
	BoolProperty_Engine_CrowdReplicationActor_bSpawningActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.CrowdReplicationActor.bSpawningActive");
	ClassProperty_Engine_DroppedPickup_InventoryClass = (UProperty*)ClassPreloader::GetObject("ClassProperty Engine.DroppedPickup.InventoryClass");
	BoolProperty_Engine_DroppedPickup_bFadeOut = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.DroppedPickup.bFadeOut");
	ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.DynamicSMActor.ReplicatedMaterial");
	ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.DynamicSMActor.ReplicatedMesh");
	StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.DynamicSMActor.ReplicatedMeshRotation");
	StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.DynamicSMActor.ReplicatedMeshScale3D");
	StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.DynamicSMActor.ReplicatedMeshTranslation");
	BoolProperty_Engine_DynamicSMActor_bForceStaticDecals = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.DynamicSMActor.bForceStaticDecals");
	BoolProperty_Engine_Emitter_bCurrentlyActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Emitter.bCurrentlyActive");
	ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.EmitterSpawnable.ParticleTemplate");
	BoolProperty_Engine_FluidInfluenceActor_bActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.FluidInfluenceActor.bActive");
	BoolProperty_Engine_FluidInfluenceActor_bToggled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.FluidInfluenceActor.bToggled");
	BoolProperty_Engine_FogVolumeDensityInfo_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.FogVolumeDensityInfo.bEnabled");
	IntProperty_Engine_GameReplicationInfo_MatchID = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.MatchID");
	ObjectProperty_Engine_GameReplicationInfo_Winner = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.GameReplicationInfo.Winner");
	BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.GameReplicationInfo.bMatchHasBegun");
	BoolProperty_Engine_GameReplicationInfo_bMatchIsOver = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.GameReplicationInfo.bMatchIsOver");
	BoolProperty_Engine_GameReplicationInfo_bStopCountDown = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.GameReplicationInfo.bStopCountDown");
	IntProperty_Engine_GameReplicationInfo_RemainingMinute = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.RemainingMinute");
	StrProperty_Engine_GameReplicationInfo_AdminEmail = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.GameReplicationInfo.AdminEmail");
	StrProperty_Engine_GameReplicationInfo_AdminName = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.GameReplicationInfo.AdminName");
	IntProperty_Engine_GameReplicationInfo_ElapsedTime = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.ElapsedTime");
	ClassProperty_Engine_GameReplicationInfo_GameClass = (UProperty*)ClassPreloader::GetObject("ClassProperty Engine.GameReplicationInfo.GameClass");
	IntProperty_Engine_GameReplicationInfo_GoalScore = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.GoalScore");
	IntProperty_Engine_GameReplicationInfo_MaxLives = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.MaxLives");
	StrProperty_Engine_GameReplicationInfo_MessageOfTheDay = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.GameReplicationInfo.MessageOfTheDay");
	IntProperty_Engine_GameReplicationInfo_RemainingTime = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.RemainingTime");
	StrProperty_Engine_GameReplicationInfo_ServerName = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.GameReplicationInfo.ServerName");
	IntProperty_Engine_GameReplicationInfo_ServerRegion = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.ServerRegion");
	StrProperty_Engine_GameReplicationInfo_ShortName = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.GameReplicationInfo.ShortName");
	IntProperty_Engine_GameReplicationInfo_TimeLimit = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.GameReplicationInfo.TimeLimit");
	BoolProperty_Engine_GameReplicationInfo_bIsArbitrated = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.GameReplicationInfo.bIsArbitrated");
	BoolProperty_Engine_GameReplicationInfo_bTrackStats = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.GameReplicationInfo.bTrackStats");
	BoolProperty_Engine_HeightFog_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.HeightFog.bEnabled");
	ObjectProperty_Engine_Inventory_InvManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Inventory.InvManager");
	ObjectProperty_Engine_Inventory_Inventory = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Inventory.Inventory");
	ObjectProperty_Engine_InventoryManager_InventoryChain = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.InventoryManager.InventoryChain");
	StructProperty_Engine_KActor_RBState = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.KActor.RBState");
	StructProperty_Engine_KActor_ReplicatedDrawScale3D = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.KActor.ReplicatedDrawScale3D");
	BoolProperty_Engine_KActor_bWakeOnLevelStart = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.KActor.bWakeOnLevelStart");
	ObjectProperty_Engine_KAsset_ReplicatedMesh = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.KAsset.ReplicatedMesh");
	ObjectProperty_Engine_KAsset_ReplicatedPhysAsset = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.KAsset.ReplicatedPhysAsset");
	BoolProperty_Engine_LensFlareSource_bCurrentlyActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.LensFlareSource.bCurrentlyActive");
	BoolProperty_Engine_Light_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Light.bEnabled");
	ObjectProperty_Engine_MatineeActor_InterpAction = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.MatineeActor.InterpAction");
	FloatProperty_Engine_MatineeActor_PlayRate = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.MatineeActor.PlayRate");
	FloatProperty_Engine_MatineeActor_Position = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.MatineeActor.Position");
	BoolProperty_Engine_MatineeActor_bIsPlaying = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.MatineeActor.bIsPlaying");
	BoolProperty_Engine_MatineeActor_bPaused = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.MatineeActor.bPaused");
	BoolProperty_Engine_MatineeActor_bReversePlayback = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.MatineeActor.bReversePlayback");
	BoolProperty_Engine_NxForceField_bForceActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.NxForceField.bForceActive");
	ObjectProperty_Engine_Pawn_DrivenVehicle = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Pawn.DrivenVehicle");
	StructProperty_Engine_Pawn_FlashLocation = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.Pawn.FlashLocation");
	IntProperty_Engine_Pawn_Health = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.Pawn.Health");
	ClassProperty_Engine_Pawn_HitDamageType = (UProperty*)ClassPreloader::GetObject("ClassProperty Engine.Pawn.HitDamageType");
	ObjectProperty_Engine_Pawn_PlayerReplicationInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Pawn.PlayerReplicationInfo");
	StructProperty_Engine_Pawn_TakeHitLocation = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.Pawn.TakeHitLocation");
	BoolProperty_Engine_Pawn_bIsWalking = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Pawn.bIsWalking");
	BoolProperty_Engine_Pawn_bSimulateGravity = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Pawn.bSimulateGravity");
	FloatProperty_Engine_Pawn_AccelRate = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.AccelRate");
	FloatProperty_Engine_Pawn_AirControl = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.AirControl");
	FloatProperty_Engine_Pawn_AirSpeed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.AirSpeed");
	ObjectProperty_Engine_Pawn_Controller = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Pawn.Controller");
	FloatProperty_Engine_Pawn_GroundSpeed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.GroundSpeed");
	ObjectProperty_Engine_Pawn_InvManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Pawn.InvManager");
	FloatProperty_Engine_Pawn_JumpZ = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.JumpZ");
	FloatProperty_Engine_Pawn_WaterSpeed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Pawn.WaterSpeed");
	ByteProperty_Engine_Pawn_FiringMode = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Pawn.FiringMode");
	ByteProperty_Engine_Pawn_FlashCount = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Pawn.FlashCount");
	BoolProperty_Engine_Pawn_bIsCrouched = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Pawn.bIsCrouched");
	StructProperty_Engine_Pawn_TearOffMomentum = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.Pawn.TearOffMomentum");
	ByteProperty_Engine_Pawn_RemoteViewPitch = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.Pawn.RemoteViewPitch");
	ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.PhysXEmitterSpawnable.ParticleTemplate");
	BoolProperty_Engine_PickupFactory_bPickupHidden = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PickupFactory.bPickupHidden");
	ClassProperty_Engine_PickupFactory_InventoryType = (UProperty*)ClassPreloader::GetObject("ClassProperty Engine.PickupFactory.InventoryType");
	FloatProperty_Engine_PlayerController_TargetEyeHeight = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.PlayerController.TargetEyeHeight");
	StructProperty_Engine_PlayerController_TargetViewRotation = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.PlayerController.TargetViewRotation");
	FloatProperty_Engine_PlayerReplicationInfo_Deaths = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.PlayerReplicationInfo.Deaths");
	StrProperty_Engine_PlayerReplicationInfo_PlayerAlias = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.PlayerReplicationInfo.PlayerAlias");
	ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.PlayerReplicationInfo.PlayerLocationHint");
	StrProperty_Engine_PlayerReplicationInfo_PlayerName = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.PlayerReplicationInfo.PlayerName");
	IntProperty_Engine_PlayerReplicationInfo_PlayerSkill = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.PlayerReplicationInfo.PlayerSkill");
	FloatProperty_Engine_PlayerReplicationInfo_Score = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.PlayerReplicationInfo.Score");
	IntProperty_Engine_PlayerReplicationInfo_StartTime = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.PlayerReplicationInfo.StartTime");
	ObjectProperty_Engine_PlayerReplicationInfo_Team = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.PlayerReplicationInfo.Team");
	StructProperty_Engine_PlayerReplicationInfo_UniqueId = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.PlayerReplicationInfo.UniqueId");
	BoolProperty_Engine_PlayerReplicationInfo_bAdmin = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bAdmin");
	BoolProperty_Engine_PlayerReplicationInfo_bHasFlag = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bHasFlag");
	BoolProperty_Engine_PlayerReplicationInfo_bIsFemale = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bIsFemale");
	BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bIsSpectator");
	BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bOnlySpectator");
	BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bOutOfLives");
	BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bReadyToPlay");
	BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bWaitingPlayer");
	ByteProperty_Engine_PlayerReplicationInfo_PacketLoss = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.PlayerReplicationInfo.PacketLoss");
	ByteProperty_Engine_PlayerReplicationInfo_Ping = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.PlayerReplicationInfo.Ping");
	IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.PlayerReplicationInfo.SplitscreenIndex");
	IntProperty_Engine_PlayerReplicationInfo_PlayerID = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.PlayerReplicationInfo.PlayerID");
	BoolProperty_Engine_PlayerReplicationInfo_bBot = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bBot");
	BoolProperty_Engine_PlayerReplicationInfo_bIsInactive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PlayerReplicationInfo.bIsInactive");
	BoolProperty_Engine_PostProcessVolume_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.PostProcessVolume.bEnabled");
	FloatProperty_Engine_Projectile_MaxSpeed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Projectile.MaxSpeed");
	FloatProperty_Engine_Projectile_Speed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.Projectile.Speed");
	BoolProperty_Engine_RB_CylindricalForceActor_bForceActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.RB_CylindricalForceActor.bForceActive");
	ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.RB_LineImpulseActor.ImpulseCount");
	BoolProperty_Engine_RB_RadialForceActor_bForceActive = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.RB_RadialForceActor.bForceActive");
	ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount = (UProperty*)ClassPreloader::GetObject("ByteProperty Engine.RB_RadialImpulseActor.ImpulseCount");
	FloatProperty_Engine_SVehicle_MaxSpeed = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.SVehicle.MaxSpeed");
	StructProperty_Engine_SVehicle_VState = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.SVehicle.VState");
	ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.SkeletalMeshActor.ReplicatedMaterial");
	ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.SkeletalMeshActor.ReplicatedMesh");
	FloatProperty_Engine_TeamInfo_Score = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.TeamInfo.Score");
	IntProperty_Engine_TeamInfo_TeamIndex = (UProperty*)ClassPreloader::GetObject("IntProperty Engine.TeamInfo.TeamIndex");
	StrProperty_Engine_TeamInfo_TeamName = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.TeamInfo.TeamName");
	StrProperty_Engine_Teleporter_URL = (UProperty*)ClassPreloader::GetObject("StrProperty Engine.Teleporter.URL");
	BoolProperty_Engine_Teleporter_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bEnabled");
	StructProperty_Engine_Teleporter_TargetVelocity = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.Teleporter.TargetVelocity");
	BoolProperty_Engine_Teleporter_bChangesVelocity = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bChangesVelocity");
	BoolProperty_Engine_Teleporter_bChangesYaw = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bChangesYaw");
	BoolProperty_Engine_Teleporter_bReversesX = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bReversesX");
	BoolProperty_Engine_Teleporter_bReversesY = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bReversesY");
	BoolProperty_Engine_Teleporter_bReversesZ = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Teleporter.bReversesZ");
	BoolProperty_Engine_Vehicle_bDriving = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.Vehicle.bDriving");
	ObjectProperty_Engine_Vehicle_Driver = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.Vehicle.Driver");
	ObjectProperty_Engine_WorldInfo_Pauser = (UProperty*)ClassPreloader::GetObject("ObjectProperty Engine.WorldInfo.Pauser");
	StructProperty_Engine_WorldInfo_ReplicatedMusicTrack = (UProperty*)ClassPreloader::GetObject("StructProperty Engine.WorldInfo.ReplicatedMusicTrack");
	FloatProperty_Engine_WorldInfo_TimeDilation = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.WorldInfo.TimeDilation");
	FloatProperty_Engine_WorldInfo_WorldGravityZ = (UProperty*)ClassPreloader::GetObject("FloatProperty Engine.WorldInfo.WorldGravityZ");
	BoolProperty_Engine_WorldInfo_bHighPriorityLoading = (UProperty*)ClassPreloader::GetObject("BoolProperty Engine.WorldInfo.bHighPriorityLoading");
	ByteProperty_TgGame_TgChestActor_r_eChestState = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgChestActor.r_eChestState");
	BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDeploy_BeaconEntrance.r_bActive");
	BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDeploy_DestructibleCover.r_bHasFired");
	IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeploy_Sensor.r_nSensorAudioWarning");
	IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeploy_Sensor.r_nTouchedPlayerCount");
	BoolProperty_TgGame_TgDeployable_r_bDelayDeployed = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDeployable.r_bDelayDeployed");
	IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nReplicateDestroyIt");
	ObjectProperty_TgGame_TgDeployable_r_DRI = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgDeployable.r_DRI");
	BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDeployable.r_bInitialIsEnemy");
	BoolProperty_TgGame_TgDeployable_r_bTakeDamage = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDeployable.r_bTakeDamage");
	FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgDeployable.r_fClientProximityRadius");
	FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgDeployable.r_fCurrentDeployTime");
	IntProperty_TgGame_TgDeployable_r_nDeployableId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nDeployableId");
	IntProperty_TgGame_TgDeployable_r_nPhysicalType = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nPhysicalType");
	IntProperty_TgGame_TgDeployable_r_nTickingTime = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nTickingTime");
	ObjectProperty_TgGame_TgDeployable_r_Owner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgDeployable.r_Owner");
	IntProperty_TgGame_TgDeployable_r_nOwnerFireMode = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nOwnerFireMode");
	ByteProperty_TgGame_TgDevice_CurrentFireMode = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgDevice.CurrentFireMode");
	BoolProperty_TgGame_TgDevice_r_bIsStealthDevice = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDevice.r_bIsStealthDevice");
	ByteProperty_TgGame_TgDevice_r_eEquippedAt = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgDevice.r_eEquippedAt");
	IntProperty_TgGame_TgDevice_r_nInventoryId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDevice.r_nInventoryId");
	IntProperty_TgGame_TgDevice_r_nMeleeComboSeed = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDevice.r_nMeleeComboSeed");
	BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDevice.r_bConsumedOnDeath");
	BoolProperty_TgGame_TgDevice_r_bConsumedOnUse = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDevice.r_bConsumedOnUse");
	IntProperty_TgGame_TgDevice_r_nDeviceId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDevice.r_nDeviceId");
	IntProperty_TgGame_TgDevice_r_nDeviceInstanceId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDevice.r_nDeviceInstanceId");
	IntProperty_TgGame_TgDevice_r_nQualityValueId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDevice.r_nQualityValueId");
	BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDevice_Morale.r_bIsActivelyFiring");
	BoolProperty_TgGame_TgDoor_r_bOpen = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgDoor.r_bOpen");
	ByteProperty_TgGame_TgDoorMarker_r_eStatus = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgDoorMarker.r_eStatus");
	IntProperty_TgGame_TgDroppedItem_r_nItemId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDroppedItem.r_nItemId");
	IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDynamicDestructible.r_nDestructibleId");
	ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgDynamicDestructible.r_pFactory");
	StrProperty_TgGame_TgDynamicSMActor_m_sAssembly = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgDynamicSMActor.m_sAssembly");
	ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgDynamicSMActor.r_EffectManager");
	IntProperty_TgGame_TgDynamicSMActor_r_nHealth = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDynamicSMActor.r_nHealth");
	StructProperty_TgGame_TgEffectManager_r_EventQueue = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgEffectManager.r_EventQueue");
	StructProperty_TgGame_TgEffectManager_r_ManagedEffectList = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgEffectManager.r_ManagedEffectList");
	ObjectProperty_TgGame_TgEffectManager_r_Owner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgEffectManager.r_Owner");
	BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgEffectManager.r_bRelevancyNotify");
	IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgEffectManager.r_nInvulnerableCount");
	IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgEffectManager.r_nNextQueueIndex");
	NameProperty_TgGame_TgEmitter_BoneName = (UProperty*)ClassPreloader::GetObject("NameProperty TgGame.TgEmitter.BoneName");
	ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgFlagCaptureVolume.r_eCoalition");
	ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgFlagCaptureVolume.r_nTaskForce");
	ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgFracturedStaticMeshActor.r_EffectManager");
	IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgFracturedStaticMeshActor.r_TakeHitNotifier");
	FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgFracturedStaticMeshActor.r_DamageRadius");
	ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType = (UProperty*)ClassPreloader::GetObject("ClassProperty TgGame.TgFracturedStaticMeshActor.r_HitDamageType");
	StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgFracturedStaticMeshActor.r_HitInfo");
	StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgFracturedStaticMeshActor.r_vTakeHitLocation");
	StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgFracturedStaticMeshActor.r_vTakeHitMomentum");
	IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgHexLandMarkActor.r_nMeshAsmId");
	StrProperty_TgGame_TgInterpActor_r_sCurrState = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgInterpActor.r_sCurrState");
	IntProperty_TgGame_TgInventoryManager_r_ItemCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgInventoryManager.r_ItemCount");
	IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgKismetTestActor.r_nCurrentTest");
	IntProperty_TgGame_TgKismetTestActor_r_nFailCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgKismetTestActor.r_nFailCount");
	IntProperty_TgGame_TgKismetTestActor_r_nPassCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgKismetTestActor.r_nPassCount");
	BoolProperty_TgGame_TgLevelCamera_r_bEnabled = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgLevelCamera.r_bEnabled");
	ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgMissionObjective.r_ObjectiveAssignment");
	BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgMissionObjective.r_bHasBeenCapturedOnce");
	BoolProperty_TgGame_TgMissionObjective_r_bIsActive = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgMissionObjective.r_bIsActive");
	BoolProperty_TgGame_TgMissionObjective_r_bIsLocked = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgMissionObjective.r_bIsLocked");
	BoolProperty_TgGame_TgMissionObjective_r_bIsPending = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgMissionObjective.r_bIsPending");
	ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgMissionObjective.r_eOwningCoalition");
	ByteProperty_TgGame_TgMissionObjective_r_eStatus = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgMissionObjective.r_eStatus");
	FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgMissionObjective.r_fCurrCaptureTime");
	FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgMissionObjective.r_fLastCompletedTime");
	IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgMissionObjective.r_nOwnerTaskForce");
	IntProperty_TgGame_TgMissionObjective_nObjectiveId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgMissionObjective.nObjectiveId");
	IntProperty_TgGame_TgMissionObjective_nPriority = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgMissionObjective.nPriority");
	ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgMissionObjective.r_OpenWorldPlayerDefaultRole");
	BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgMissionObjective.r_bUsePendingState");
	ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgMissionObjective.r_eDefaultCoalition");
	ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgMissionObjective_Bot.r_ObjectiveBot");
	ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgMissionObjective_Bot.r_ObjectiveBotInfo");
	ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgMissionObjective_Escort.r_AttachedActor");
	FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgMissionObjective_Proximity.r_fCaptureRate");
	ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgObjectiveAssignment.r_AssignedObjective");
	ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgObjectiveAssignment.r_Attackers");
	ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgObjectiveAssignment.r_Bots");
	ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgObjectiveAssignment.r_Defenders");
	ByteProperty_TgGame_TgObjectiveAssignment_r_eState = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgObjectiveAssignment.r_eState");
	BoolProperty_TgGame_TgPawn_r_bIsBot = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsBot");
	BoolProperty_TgGame_TgPawn_r_bIsHenchman = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsHenchman");
	BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bNeedPlaySpawnFx");
	FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fMakeVisibleIncreased");
	IntProperty_TgGame_TgPawn_r_nAllianceId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nAllianceId");
	IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nBodyMeshAsmId");
	IntProperty_TgGame_TgPawn_r_nBotRankValueId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nBotRankValueId");
	IntProperty_TgGame_TgPawn_r_nFlashEvent = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nFlashEvent");
	IntProperty_TgGame_TgPawn_r_nFlashFireInfo = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nFlashFireInfo");
	IntProperty_TgGame_TgPawn_r_nFlashQueIndex = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nFlashQueIndex");
	IntProperty_TgGame_TgPawn_r_nPawnId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nPawnId");
	IntProperty_TgGame_TgPawn_r_nPhysicalType = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nPhysicalType");
	IntProperty_TgGame_TgPawn_r_nPreyProfileType = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nPreyProfileType");
	IntProperty_TgGame_TgPawn_r_nProfileId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nProfileId");
	IntProperty_TgGame_TgPawn_r_nProfileTypeValueId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nProfileTypeValueId");
	IntProperty_TgGame_TgPawn_r_nSoundGroupId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nSoundGroupId");
	StructProperty_TgGame_TgPawn_r_vFlashLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_vFlashLocation");
	StructProperty_TgGame_TgPawn_r_vFlashRayDir = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_vFlashRayDir");
	FloatProperty_TgGame_TgPawn_r_vFlashRefireTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_vFlashRefireTime");
	IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_vFlashSituationalAttack");
	StructProperty_TgGame_TgPawn_r_EquipDeviceInfo = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_EquipDeviceInfo");
	BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bInitialIsEnemy");
	ByteProperty_TgGame_TgPawn_r_bMadeSound = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_bMadeSound");
	ByteProperty_TgGame_TgPawn_r_eDesiredInHand = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_eDesiredInHand");
	IntProperty_TgGame_TgPawn_r_eEquippedInHandMode = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_eEquippedInHandMode");
	IntProperty_TgGame_TgPawn_r_nReplicateHit = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nReplicateHit");
	ObjectProperty_TgGame_TgPawn_r_ControlPawn = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_ControlPawn");
	ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_CurrentOmegaVolume");
	ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_CurrentSubzoneBilboardVol");
	ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_CurrentSubzoneVol");
	StructProperty_TgGame_TgPawn_r_ScannerSettings = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_ScannerSettings");
	ByteProperty_TgGame_TgPawn_r_UIClockState = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_UIClockState");
	FloatProperty_TgGame_TgPawn_r_UIClockTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_UIClockTime");
	IntProperty_TgGame_TgPawn_r_UITextBox1MessageID = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_UITextBox1MessageID");
	ByteProperty_TgGame_TgPawn_r_UITextBox1Packet = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_UITextBox1Packet");
	FloatProperty_TgGame_TgPawn_r_UITextBox1Time = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_UITextBox1Time");
	IntProperty_TgGame_TgPawn_r_UITextBox2MessageID = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_UITextBox2MessageID");
	ByteProperty_TgGame_TgPawn_r_UITextBox2Packet = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_UITextBox2Packet");
	FloatProperty_TgGame_TgPawn_r_UITextBox2Time = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_UITextBox2Time");
	BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bAllowAddMoralePoints");
	BoolProperty_TgGame_TgPawn_r_bDisableAllDevices = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bDisableAllDevices");
	BoolProperty_TgGame_TgPawn_r_bEnableCrafting = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bEnableCrafting");
	BoolProperty_TgGame_TgPawn_r_bEnableEquip = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bEnableEquip");
	BoolProperty_TgGame_TgPawn_r_bEnableSkills = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bEnableSkills");
	BoolProperty_TgGame_TgPawn_r_bInCombatFlag = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bInCombatFlag");
	BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bInGlobalOffhandCooldown");
	FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fCurrentPowerPool");
	FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fCurrentServerMoralePoints");
	FloatProperty_TgGame_TgPawn_r_fMaxControlRange = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fMaxControlRange");
	FloatProperty_TgGame_TgPawn_r_fMaxPowerPool = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fMaxPowerPool");
	FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fMoraleRechargeRate");
	FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fRequiredMoralePoints");
	FloatProperty_TgGame_TgPawn_r_fSkillRating = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fSkillRating");
	IntProperty_TgGame_TgPawn_r_nCurrency = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nCurrency");
	IntProperty_TgGame_TgPawn_r_nHZPoints = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nHZPoints");
	IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nMoraleDeviceSlot");
	IntProperty_TgGame_TgPawn_r_nRestDeviceSlot = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nRestDeviceSlot");
	IntProperty_TgGame_TgPawn_r_nToken = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nToken");
	IntProperty_TgGame_TgPawn_r_nXp = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nXp");
	FloatProperty_TgGame_TgPawn_r_DistanceToPushback = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_DistanceToPushback");
	ObjectProperty_TgGame_TgPawn_r_EffectManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_EffectManager");
	FloatProperty_TgGame_TgPawn_r_FlightAcceleration = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_FlightAcceleration");
	StructProperty_TgGame_TgPawn_r_HangingRotation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_HangingRotation");
	ObjectProperty_TgGame_TgPawn_r_Owner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_Owner");
	ObjectProperty_TgGame_TgPawn_r_Pet = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_Pet");
	StructProperty_TgGame_TgPawn_r_PlayAnimation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_PlayAnimation");
	StructProperty_TgGame_TgPawn_r_PushbackDirection = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_PushbackDirection");
	ObjectProperty_TgGame_TgPawn_r_Target = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_Target");
	ObjectProperty_TgGame_TgPawn_r_TargetActor = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_TargetActor");
	ObjectProperty_TgGame_TgPawn_r_aDebugDestination = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_aDebugDestination");
	ObjectProperty_TgGame_TgPawn_r_aDebugNextNav = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_aDebugNextNav");
	ObjectProperty_TgGame_TgPawn_r_aDebugTarget = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn.r_aDebugTarget");
	ByteProperty_TgGame_TgPawn_r_bAimType = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_bAimType");
	BoolProperty_TgGame_TgPawn_r_bAimingMode = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bAimingMode");
	BoolProperty_TgGame_TgPawn_r_bCallingForHelp = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bCallingForHelp");
	BoolProperty_TgGame_TgPawn_r_bIsAFK = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsAFK");
	BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsAnimInStrafeMode");
	BoolProperty_TgGame_TgPawn_r_bIsCrafting = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsCrafting");
	BoolProperty_TgGame_TgPawn_r_bIsCrewing = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsCrewing");
	BoolProperty_TgGame_TgPawn_r_bIsDecoy = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsDecoy");
	BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsGrappleDismounting");
	BoolProperty_TgGame_TgPawn_r_bIsHacked = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsHacked");
	BoolProperty_TgGame_TgPawn_r_bIsHacking = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsHacking");
	BoolProperty_TgGame_TgPawn_r_bIsHanging = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsHanging");
	BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsHangingDismounting");
	BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsInSnipeScope");
	BoolProperty_TgGame_TgPawn_r_bIsRappelling = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsRappelling");
	BoolProperty_TgGame_TgPawn_r_bIsStealthed = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bIsStealthed");
	BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bJumpedFromHanging");
	BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bPostureIgnoreTransition");
	BoolProperty_TgGame_TgPawn_r_bResistTagging = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bResistTagging");
	BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bShouldKnockDownAnimFaceDown");
	BoolProperty_TgGame_TgPawn_r_bTagEnemy = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bTagEnemy");
	BoolProperty_TgGame_TgPawn_r_bUsingBinoculars = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn.r_bUsingBinoculars");
	ByteProperty_TgGame_TgPawn_r_eCurrentStunType = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_eCurrentStunType");
	ByteProperty_TgGame_TgPawn_r_eDeathReason = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_eDeathReason");
	IntProperty_TgGame_TgPawn_r_eEmoteLength = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_eEmoteLength");
	IntProperty_TgGame_TgPawn_r_eEmoteRepnotify = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_eEmoteRepnotify");
	IntProperty_TgGame_TgPawn_r_eEmoteUpdate = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_eEmoteUpdate");
	ByteProperty_TgGame_TgPawn_r_ePosture = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn.r_ePosture");
	FloatProperty_TgGame_TgPawn_r_fDeployRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fDeployRate");
	FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fFrictionMultiplier");
	FloatProperty_TgGame_TgPawn_r_fGravityZModifier = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fGravityZModifier");
	FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fKnockDownTimeRemaining");
	FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fMakeVisibleFadeRate");
	FloatProperty_TgGame_TgPawn_r_fPostureRateScale = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fPostureRateScale");
	FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fRappelGravityModifier");
	FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn.r_fStealthTransitionTime");
	IntProperty_TgGame_TgPawn_r_fWeightBonus = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_fWeightBonus");
	IntProperty_TgGame_TgPawn_r_iKnockDownFlash = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_iKnockDownFlash");
	IntProperty_TgGame_TgPawn_r_nApplyStealth = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nApplyStealth");
	IntProperty_TgGame_TgPawn_r_nBotSoundCueId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nBotSoundCueId");
	IntProperty_TgGame_TgPawn_r_nDebugAggroRange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nDebugAggroRange");
	IntProperty_TgGame_TgPawn_r_nDebugFOV = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nDebugFOV");
	IntProperty_TgGame_TgPawn_r_nDebugHearingRange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nDebugHearingRange");
	IntProperty_TgGame_TgPawn_r_nDebugSightRange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nDebugSightRange");
	IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nGenericAIEventIndex");
	IntProperty_TgGame_TgPawn_r_nHealthMaximum = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nHealthMaximum");
	IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nNumberTimesCrewed");
	IntProperty_TgGame_TgPawn_r_nPhase = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nPhase");
	IntProperty_TgGame_TgPawn_r_nPitchOffset = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nPitchOffset");
	IntProperty_TgGame_TgPawn_r_nReplicateDying = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nReplicateDying");
	IntProperty_TgGame_TgPawn_r_nResetCharacter = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nResetCharacter");
	IntProperty_TgGame_TgPawn_r_nSensorAlertLevel = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nSensorAlertLevel");
	IntProperty_TgGame_TgPawn_r_nShieldHealthMax = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nShieldHealthMax");
	IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nShieldHealthRemaining");
	IntProperty_TgGame_TgPawn_r_nSilentMode = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nSilentMode");
	IntProperty_TgGame_TgPawn_r_nStealthAggroRange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nStealthAggroRange");
	IntProperty_TgGame_TgPawn_r_nStealthDisabled = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nStealthDisabled");
	IntProperty_TgGame_TgPawn_r_nStealthSensorRange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nStealthSensorRange");
	IntProperty_TgGame_TgPawn_r_nStealthTypeCode = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nStealthTypeCode");
	IntProperty_TgGame_TgPawn_r_nYawOffset = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn.r_nYawOffset");
	StrProperty_TgGame_TgPawn_r_sDebugAction = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgPawn.r_sDebugAction");
	StrProperty_TgGame_TgPawn_r_sDebugName = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgPawn.r_sDebugName");
	StrProperty_TgGame_TgPawn_r_sFactory = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgPawn.r_sFactory");
	StructProperty_TgGame_TgPawn_r_vDown = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn.r_vDown");
	BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn_Ambush.r_bIsDeployed");
	ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn_AttackTransport.r_DeathType");
	StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn_CTR.r_CustomCharacterAssembly");
	ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn_CTR.r_PilotPawn");
	IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_CTR.r_nMaxMorphIndexSentFromServer");
	IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_CTR.r_nMorphSettings");
	StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgPawn_Character.r_CustomCharacterAssembly");
	ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn_Character.r_eAttachedMesh");
	IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nBoostTimeRemaining");
	IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nHeadMeshAsmId");
	IntProperty_TgGame_TgPawn_Character_r_nItemProfileId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nItemProfileId");
	IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nItemProfileNbr");
	IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nMaxMorphIndexSentFromServer");
	IntProperty_TgGame_TgPawn_Character_r_nMorphSettings = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nMorphSettings");
	ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgPawn_Character.r_CurrentVanityPet");
	FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Character.r_WallJumpUpperLineCheckOffset");
	FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Character.r_WallJumpZ");
	BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn_Character.r_bElfGogglesEquipped");
	IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nDeviceSlotUnlockGrpId");
	IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_Character.r_nSkillGroupSetId");
	BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn_DuneCommander.r_bDoCrashLanding");
	ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn_Iris.r_nStartNewScan");
	FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Reaper.r_fBatteryPct");
	ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPawn_Siege.r_AccelDirection");
	BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPawn_Turret.r_bIsDeployed");
	FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Turret.r_fInitDeployTime");
	FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Turret.r_fTimeToDeploySecs");
	FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Turret.r_fCurrentDeployTime");
	FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgPawn_Turret.r_fDeployMaxHealthPCT");
	IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgPawn_VanityPet.r_nSpawningItemId");
	ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgPlayerController.r_WatchOtherPlayer");
	BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPlayerController.r_bEDDebugEffects");
	BoolProperty_TgGame_TgPlayerController_r_bGMInvisible = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPlayerController.r_bGMInvisible");
	BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPlayerController.r_bIsHackingABot");
	BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPlayerController.r_bLockYawRotation");
	BoolProperty_TgGame_TgPlayerController_r_bRove = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgPlayerController.r_bRove");
	StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgProj_Grapple.r_vTargetLocation");
	ObjectProperty_TgGame_TgProj_Missile_r_aSeeking = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgProj_Missile.r_aSeeking");
	StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgProj_Missile.r_vTargetWorldLocation");
	IntProperty_TgGame_TgProj_Missile_r_nNumBounces = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgProj_Missile.r_nNumBounces");
	ByteProperty_TgGame_TgProj_Rocket_FlockIndex = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgProj_Rocket.FlockIndex");
	BoolProperty_TgGame_TgProj_Rocket_bCurl = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgProj_Rocket.bCurl");
	ObjectProperty_TgGame_TgProjectile_r_Owner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgProjectile.r_Owner");
	FloatProperty_TgGame_TgProjectile_r_fAccelRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgProjectile.r_fAccelRate");
	FloatProperty_TgGame_TgProjectile_r_fDuration = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgProjectile.r_fDuration");
	FloatProperty_TgGame_TgProjectile_r_fRange = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgProjectile.r_fRange");
	IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgProjectile.r_nOwnerFireModeId");
	IntProperty_TgGame_TgProjectile_r_nProjectileId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgProjectile.r_nProjectileId");
	StructProperty_TgGame_TgProjectile_r_vSpawnLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgProjectile.r_vSpawnLocation");
	BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Beacon.r_bDeployed");
	StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Beacon.r_vLoc");
	StrProperty_TgGame_TgRepInfo_Beacon_r_nName = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgRepInfo_Beacon.r_nName");
	ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Deployable.r_InstigatorInfo");
	ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Deployable.r_TaskforceInfo");
	BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Deployable.r_bOwnedByTaskforce");
	IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Deployable.r_nHealthCurrent");
	ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Deployable.r_DeployableOwner");
	FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgRepInfo_Deployable.r_fDeployMaxHealthPCT");
	IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Deployable.r_nDeployableId");
	IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Deployable.r_nHealthMaximum");
	IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Deployable.r_nUniqueDeployableId");
	StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Game.r_MiniMapInfo");
	BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bActiveCombat");
	BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bAllowBuildMorale");
	BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bAllowPlayerRelease");
	BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bDefenseAlarm");
	BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bInOverTime");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsTutorialMap");
	FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgRepInfo_Game.r_fGameSpeedModifier");
	FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgRepInfo_Game.r_fMissionRemainingTime");
	FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgRepInfo_Game.r_fServerTimeLastUpdate");
	IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nMaxRoundNumber");
	ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgRepInfo_Game.r_nMissionTimerState");
	IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nMissionTimerStateChange");
	IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nRaidAttackerRespawnBonus");
	IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nRaidDefenderRespawnBonus");
	IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nReleaseDelay");
	IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nRoundNumber");
	IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nSecsToAutoReleaseAttackers");
	IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nSecsToAutoReleaseDefenders");
	ByteProperty_TgGame_TgRepInfo_Game_r_GameType = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgRepInfo_Game.r_GameType");
	StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Game.r_MapLogoResIds");
	ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Game.r_Objectives");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsArena");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsMatch");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsMission");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsPVP");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsRaid");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsTerritoryMap");
	BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Game.r_bIsTraining");
	IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nAutoKickTimeout");
	IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nPointsToWin");
	IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Game.r_nVictoryBonusLives");
	IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_GameOpenWorld.r_GameTickets");
	StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Player.r_ApproxLocation");
	StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Player.r_CustomCharacterAssembly");
	StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Player.r_EquipDeviceInfo");
	ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Player.r_MasterPrep");
	ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Player.r_PawnOwner");
	IntProperty_TgGame_TgRepInfo_Player_r_Scores = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_Scores");
	ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_Player.r_TaskForce");
	BoolProperty_TgGame_TgRepInfo_Player_r_bDropped = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_Player.r_bDropped");
	IntProperty_TgGame_TgRepInfo_Player_r_eBonusType = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_eBonusType");
	IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nCharacterId");
	IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nHealthCurrent");
	IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nHealthMaximum");
	IntProperty_TgGame_TgRepInfo_Player_r_nLevel = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nLevel");
	IntProperty_TgGame_TgRepInfo_Player_r_nProfileId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nProfileId");
	IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_Player.r_nTitleMsgId");
	StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgRepInfo_Player.r_sAgencyName");
	StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgRepInfo_Player.r_sAllianceName");
	StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName = (UProperty*)ClassPreloader::GetObject("StrProperty TgGame.TgRepInfo_Player.r_sOrigPlayerName");
	StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgRepInfo_Player.r_DeviceStats");
	ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_TaskForce.r_BeaconManager");
	ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_TaskForce.r_CurrActiveObjective");
	ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgRepInfo_TaskForce.r_ObjectiveAssignment");
	BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned = (UProperty*)ClassPreloader::GetObject("BoolProperty TgGame.TgRepInfo_TaskForce.r_bBotOwned");
	ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgRepInfo_TaskForce.r_eCoalition");
	IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_TaskForce.r_nCurrentPointCount");
	IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_TaskForce.r_nLeaderCharId");
	FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgRepInfo_TaskForce.r_nLookingForMembers");
	IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_TaskForce.r_nNumDeaths");
	ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgRepInfo_TaskForce.r_nTaskForce");
	IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgRepInfo_TaskForce.r_nTeamId");
	FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgSkydiveTarget.m_LandRadius");
	FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgSkydivingVolume.r_PawnGravityModifier");
	FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgSkydivingVolume.r_PawnLaunchForce");
	FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgSkydivingVolume.r_PawnUpForce");
	ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgSkydivingVolume.r_SkydiveTarget");
	ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgTeamBeaconManager.r_Beacon");
	IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgTeamBeaconManager.r_BeaconDestroyed");
	ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgTeamBeaconManager.r_BeaconHolder");
	ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgTeamBeaconManager.r_BeaconInfo");
	ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgTeamBeaconManager.r_BeaconStatus");
	ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgTeamBeaconManager.r_TaskForce");
	ByteProperty_TgGame_TgTimerManager_r_byEventQue = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgTimerManager.r_byEventQue");
	ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgTimerManager.r_byEventQueIndex");
	FloatProperty_TgGame_TgTimerManager_r_fRemaining = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgTimerManager.r_fRemaining");
	FloatProperty_TgGame_TgTimerManager_r_fStartTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgTimerManager.r_fStartTime");
}


int* __fastcall Actor__GetOptimizedRepList::Call(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5) {
	param_3 = CallOriginal(thisxx, edx_dummy, param_1, param_2, param_3, param_4, param_5);
	if (!bRepListCached) {
		bRepListCached = true;
		ResolveRepListProperties();
	}

	AActor* actor = (AActor*)thisxx;
	AActor* recent = (AActor*)param_1;
	int repindex = 0;
	char* classname = actor->Class->GetFullName();
	if (
		strcmp(classname, "Class Engine.Actor") == 0

		|| strcmp(classname, "Class Engine.WorldInfo") == 0

		|| strcmp(classname, "Class Engine.Vehicle") == 0
		|| strcmp(classname, "Class Engine.SVehicle") == 0
		|| strcmp(classname, "Class Engine.Teleporter") == 0
		|| strcmp(classname, "Class Engine.RB_RadialImpulseActor") == 0
		|| strcmp(classname, "Class Engine.RB_RadialForceActor") == 0
		|| strcmp(classname, "Class Engine.RB_LineImpulseActor") == 0
		|| strcmp(classname, "Class Engine.RB_CylindricalForceActor") == 0
		|| strcmp(classname, "Class Engine.PostProcessVolume") == 0

		|| strcmp(classname, "Class Engine.TeamInfo") == 0
		|| strcmp(classname, "Class Engine.GameReplicationInfo") == 0
		|| strcmp(classname, "Class Engine.PlayerReplicationInfo") == 0
		|| strcmp(classname, "Class Engine.PlayerController") == 0
		|| strcmp(classname, "Class Engine.PickupFactory") == 0
		|| strcmp(classname, "Class Engine.PhysXEmitterSpawnable") == 0
		|| strcmp(classname, "Class Engine.NxForceField") == 0
		|| strcmp(classname, "Class Engine.MatineeActor") == 0
		|| strcmp(classname, "Class Engine.Light") == 0
		|| strcmp(classname, "Class Engine.LensFlareSource") == 0
		|| strcmp(classname, "Class Engine.KAsset") == 0
		|| strcmp(classname, "Class Engine.KActor") == 0
		|| strcmp(classname, "Class Engine.InventoryManager") == 0
		|| strcmp(classname, "Class Engine.Inventory") == 0
		|| strcmp(classname, "Class Engine.HeightFog") == 0
		|| strcmp(classname, "Class Engine.FogVolumeDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FluidInfluenceActor") == 0
		|| strcmp(classname, "Class Engine.EmitterSpawnable") == 0
		|| strcmp(classname, "Class Engine.Emitter") == 0
		|| strcmp(classname, "Class Engine.DroppedPickup") == 0
		|| strcmp(classname, "Class Engine.CrowdReplicationActor") == 0
		|| strcmp(classname, "Class Engine.CrowdAttractor") == 0
		|| strcmp(classname, "Class Engine.Controller") == 0
		|| strcmp(classname, "Class Engine.CameraActor") == 0
		|| strcmp(classname, "Class Engine.AmbientSoundSimpleToggleable") == 0

		|| strcmp(classname, "Class Engine.DynamicSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgObjectiveAttachActor") == 0
		|| strcmp(classname, "Class TgGame.TgInterpActor") == 0
		|| strcmp(classname, "Class TgGame.TgHexLandMarkActor") == 0
		|| strcmp(classname, "Class TgGame.TgFracturedStaticMeshActor") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicDestructible") == 0


		|| strcmp(classname, "Class TgGame.TgFlagCaptureVolume") == 0

		|| strcmp(classname, "Class TgGame.TgEmitter") == 0

		|| strcmp(classname, "Class TgGame.TgStaticMeshActor") == 0

		|| strcmp(classname, "Class TgGame.TgRandomSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgRandomSMManager") == 0

		|| strcmp(classname, "Class TgGame.TgEffectManager") == 0

		|| strcmp(classname, "Class TgGame.TgInventoryManager") == 0

		|| strcmp(classname, "Class TgGame.TgDroppedItem") == 0

		|| strcmp(classname, "Class TgGame.TgChestActor") == 0

		|| strcmp(classname, "Class TgGame.TgDevice") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_HitPulse") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Morale") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewMelee") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_MeleeDualWield") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewRange") == 0

		|| strcmp(classname, "Class TgGame.TgDeployable") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Beacon") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_BeaconEntrance") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_DestructibleCover") == 0

		|| strcmp(classname, "Class TgGame.TgMissionObjective") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Bot") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Escort") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Proximity") == 0

		|| strcmp(classname, "Class Engine.Pawn") == 0
		|| strcmp(classname, "Class TgGame.TgPawn") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AVCompositeWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Ambush") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AndroidMinion") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AttackTransport") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss_Destroyer") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Brawler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_CTR") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Character") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ColonyEye") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Destructible") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Detonator") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Dismantler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_DuneCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Alchemist") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Assassin") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_EscortRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_FlyingBoss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_GroundPetA") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Guardian") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_HoverShieldSphere") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hunter") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Inquisitor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Interact_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Iris") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Juggernaut") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Marauder") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NewWasp") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Raptor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Reaper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_RecursiveSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Remote") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Robot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Scanner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ScannerRecursive") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Siege") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeBarrage") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeHover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeRapidFire") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Sniper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SonoranCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SupportForeman") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Switchblade") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Tentacle") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ThinkTank") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TreadRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Turret") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVAFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVARocket") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlame") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretPlasma") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_UberWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vanguard") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_VanityPet") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vulcan") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Warlord") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_WaspSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Widow") == 0
		
		|| strcmp(classname, "Class TgGame.TgObjectiveAssignment") == 0

		|| strcmp(classname, "Class TgGame.TgRepInfo_Deployable") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_Beacon") == 0

		|| strcmp(classname, "Class TgGame.TgRepInfo_Game") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_GameOpenWorld") == 0

		|| strcmp(classname, "Class TgGame.TgRepInfo_Player") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_TaskForce") == 0

		|| strcmp(classname, "Class Engine.KAsset") == 0

		|| strcmp(classname, "Class Engine.StaticMeshActor") == 0

		|| strcmp(classname, "Class Engine.Projectile") == 0
		|| strcmp(classname, "Class TgGame.TgProjectile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Teleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_StraightTeleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Rocket") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Net") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Mortar") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Missile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grapple") == 0
		|| strcmp(classname, "Class TgGame.TgProj_FreeGrenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Deployable") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bounce") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bot") == 0

		|| strcmp(classname, "Class TgGame.TgDoorMarker") == 0
		|| strcmp(classname, "Class TgGame.TgDoor") == 0


		|| strcmp(classname, "Class Engine.SkeletalMeshActor") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorGenericUIPreview") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorNPC") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorNPCVendor") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_CharacterBuilder") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_CharacterBuilderSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_Composite") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_EquipScreen") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_MeleePreVis") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor") == 0

		|| strcmp(classname, "Class TgGame.TgSkydiveTarget") == 0

		|| strcmp(classname, "Class TgGame.TgSkydivingVolume") == 0

		|| strcmp(classname, "Class TgGame.TgTeamBeaconManager") == 0

		|| strcmp(classname, "Class TgGame.TgTimerManager") == 0
		
		|| strcmp(classname, "Class TgGame.TgLevelCamera") == 0

		|| strcmp(classname, "Class TgGame.TgKismetTestActor") == 0

		|| strcmp(classname, "Class Engine.AIController") == 0
		|| strcmp(classname, "Class Engine.Admin") == 0
		|| strcmp(classname, "Class Engine.DebugCameraController") == 0
		|| strcmp(classname, "Class Engine.DynamicCameraActor") == 0
		|| strcmp(classname, "Class Engine.AnimatedCamera") == 0
		|| strcmp(classname, "Class Engine.EmitterCameraLensEffectBase") == 0
		|| strcmp(classname, "Class Engine.EmitterPool") == 0
		|| strcmp(classname, "Class Engine.FogVolumeConeDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeConstantDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeLinearHalfspaceDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeSphericalDensityInfo") == 0
		|| strcmp(classname, "Class Engine.DynamicSMActor_Spawnable") == 0
		|| strcmp(classname, "Class Engine.InterpActor") == 0
		|| strcmp(classname, "Class Engine.KActorSpawnable") == 0
		|| strcmp(classname, "Class Engine.KAssetSpawnable") == 0
		|| strcmp(classname, "Class Engine.Scout") == 0
		|| strcmp(classname, "Class Engine.DirectionalLight") == 0
		|| strcmp(classname, "Class Engine.DirectionalLightToggleable") == 0
		|| strcmp(classname, "Class Engine.PointLight") == 0
		|| strcmp(classname, "Class Engine.PointLightMovable") == 0
		|| strcmp(classname, "Class Engine.PointLightToggleable") == 0
		|| strcmp(classname, "Class Engine.SkyLight") == 0
		|| strcmp(classname, "Class Engine.SkyLightToggleable") == 0
		|| strcmp(classname, "Class Engine.SpotLight") == 0
		|| strcmp(classname, "Class Engine.SpotLightMovable") == 0
		|| strcmp(classname, "Class Engine.SpotLightToggleable") == 0
		|| strcmp(classname, "Class Engine.StaticLightCollectionActor") == 0
		|| strcmp(classname, "Class Engine.NxCylindricalForceField") == 0
		|| strcmp(classname, "Class Engine.NxCylindricalForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldGeneric") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldRadial") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldTornado") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceField") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceFieldBox") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxRadialCustomForceField") == 0
		|| strcmp(classname, "Class Engine.NxRadialForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoAngularForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoAngularForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxTornadoForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.Admin") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorBasedOnExtremeContent") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorMAT") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorMATSpawnable") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorSpawnable") == 0
		|| strcmp(classname, "Class Engine.Weapon") == 0
		|| strcmp(classname, "Class GameFramework.GameAIController") == 0
		|| strcmp(classname, "Class GameFramework.GamePawn") == 0
		|| strcmp(classname, "Class GameFramework.GamePlayerController") == 0
		|| strcmp(classname, "Class GameFramework.GameProjectile") == 0
		|| strcmp(classname, "Class GameFramework.GameVehicle") == 0
		|| strcmp(classname, "Class GameFramework.GameWeapon") == 0
		|| strcmp(classname, "Class TgGame.TgAIController") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_CTFBot") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_KOTH") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Kismet") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Artillery") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_ForceField") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Sensor") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_SweepSensor") == 0
		|| strcmp(classname, "Class TgGame.TgDebugCameraController") == 0
		|| strcmp(classname, "Class TgGame.TgEmitterSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgEmitterCrashlanding") == 0
		|| strcmp(classname, "Class TgGame.TgInterpolatingCameraActor") == 0
		|| strcmp(classname, "Class TgGame.TgTeleporter") == 0
		|| strcmp(classname, "Class TgGame.TgPostProcessVolume") == 0
		|| strcmp(classname, "Class TgGame.TgPickupFactory") == 0
		|| strcmp(classname, "Class TgGame.TgPickupFactory_Item") == 0
		|| strcmp(classname, "Class TgGame.TgKActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKAssetSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKAsset_ClientSideSim") == 0
		|| strcmp(classname, "Class TgGame.TgNavRouteIndicator") == 0
		|| strcmp(classname, "Class TgGame.TgCharacterBuilderLight") == 0
		|| strcmp(classname, "Class Engine.AccessControl") == 0
		|| strcmp(classname, "Class Engine.AmbientSound") == 0
		|| strcmp(classname, "Class Engine.AmbientSoundMovable") == 0
		|| strcmp(classname, "Class Engine.AmbientSoundNonLoop") == 0
		|| strcmp(classname, "Class Engine.AmbientSoundSimple") == 0
		|| strcmp(classname, "Class Engine.AutoLadder") == 0
		|| strcmp(classname, "Class Engine.BlockingVolume") == 0
		|| strcmp(classname, "Class Engine.BroadcastHandler") == 0
		|| strcmp(classname, "Class Engine.Brush") == 0
		|| strcmp(classname, "Class Engine.Camera") == 0
		|| strcmp(classname, "Class Engine.ClipMarker") == 0
		|| strcmp(classname, "Class Engine.ColorScaleVolume") == 0
		|| strcmp(classname, "Class Engine.CoverGroup") == 0
		|| strcmp(classname, "Class Engine.CoverLink") == 0
		|| strcmp(classname, "Class Engine.CoverReplicator") == 0
		|| strcmp(classname, "Class Engine.CoverSlotMarker") == 0
		|| strcmp(classname, "Class Engine.CrowdAgent") == 0
		|| strcmp(classname, "Class Engine.CullDistanceVolume") == 0
		|| strcmp(classname, "Class Engine.DebugCameraHUD") == 0
		|| strcmp(classname, "Class Engine.DecalActor") == 0
		|| strcmp(classname, "Class Engine.DecalActorBase") == 0
		|| strcmp(classname, "Class Engine.DecalActorMovable") == 0
		|| strcmp(classname, "Class Engine.DecalManager") == 0
		|| strcmp(classname, "Class Engine.DefaultPhysicsVolume") == 0
		|| strcmp(classname, "Class Engine.DoorMarker") == 0
		|| strcmp(classname, "Class Engine.DynamicAnchor") == 0
		|| strcmp(classname, "Class Engine.DynamicBlockingVolume") == 0
		|| strcmp(classname, "Class Engine.DynamicPhysicsVolume") == 0
		|| strcmp(classname, "Class Engine.DynamicTriggerVolume") == 0
		|| strcmp(classname, "Class Engine.FileLog") == 0
		|| strcmp(classname, "Class Engine.FileWriter") == 0
		|| strcmp(classname, "Class Engine.FluidSurfaceActor") == 0
		|| strcmp(classname, "Class Engine.FluidSurfaceActorMovable") == 0
		|| strcmp(classname, "Class Engine.FoliageFactory") == 0
		|| strcmp(classname, "Class Engine.FractureManager") == 0
		|| strcmp(classname, "Class Engine.FracturedStaticMeshActor") == 0
		|| strcmp(classname, "Class Engine.FracturedStaticMeshPart") == 0
		|| strcmp(classname, "Class Engine.GameInfo") == 0
		|| strcmp(classname, "Class Engine.GravityVolume") == 0
		|| strcmp(classname, "Class Engine.HUD") == 0
		|| strcmp(classname, "Class Engine.Info") == 0
		|| strcmp(classname, "Class Engine.InternetInfo") == 0
		|| strcmp(classname, "Class Engine.Keypoint") == 0
		|| strcmp(classname, "Class Engine.Ladder") == 0
		|| strcmp(classname, "Class Engine.LadderVolume") == 0
		|| strcmp(classname, "Class Engine.LevelStreamingVolume") == 0
		|| strcmp(classname, "Class Engine.LiftCenter") == 0
		|| strcmp(classname, "Class Engine.LiftExit") == 0
		|| strcmp(classname, "Class Engine.LightVolume") == 0
		|| strcmp(classname, "Class Engine.MantleMarker") == 0
		|| strcmp(classname, "Class Engine.MaterialInstanceActor") == 0
		|| strcmp(classname, "Class Engine.Mutator") == 0
		|| strcmp(classname, "Class Engine.NavigationPoint") == 0
		|| strcmp(classname, "Class Engine.Note") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceFieldBrush") == 0
		|| strcmp(classname, "Class Engine.Objective") == 0
		|| strcmp(classname, "Class Engine.PathBlockingVolume") == 0
		|| strcmp(classname, "Class Engine.PathNode") == 0
		|| strcmp(classname, "Class Engine.PathNode_Dynamic") == 0
		|| strcmp(classname, "Class Engine.PhysXDestructibleActor") == 0
		|| strcmp(classname, "Class Engine.PhysXDestructiblePart") == 0
		|| strcmp(classname, "Class Engine.PhysicsVolume") == 0
		|| strcmp(classname, "Class Engine.PlayerStart") == 0
		|| strcmp(classname, "Class Engine.PolyMarker") == 0
		|| strcmp(classname, "Class Engine.PortalMarker") == 0
		|| strcmp(classname, "Class Engine.PortalTeleporter") == 0
		|| strcmp(classname, "Class Engine.PortalVolume") == 0
		|| strcmp(classname, "Class Engine.PotentialClimbWatcher") == 0
		|| strcmp(classname, "Class Engine.PrefabInstance") == 0
		|| strcmp(classname, "Class Engine.RB_BSJointActor") == 0
		|| strcmp(classname, "Class Engine.RB_ConstraintActor") == 0
		|| strcmp(classname, "Class Engine.RB_ConstraintActorSpawnable") == 0
		|| strcmp(classname, "Class Engine.RB_ForceFieldExcludeVolume") == 0
		|| strcmp(classname, "Class Engine.RB_HingeActor") == 0
		|| strcmp(classname, "Class Engine.RB_PrismaticActor") == 0
		|| strcmp(classname, "Class Engine.RB_PulleyJointActor") == 0
		|| strcmp(classname, "Class Engine.RB_Thruster") == 0
		|| strcmp(classname, "Class Engine.ReplicationInfo") == 0
		|| strcmp(classname, "Class Engine.ReverbVolume") == 0
		|| strcmp(classname, "Class Engine.Route") == 0
		|| strcmp(classname, "Class Engine.SceneCapture2DActor") == 0
		|| strcmp(classname, "Class Engine.SceneCaptureActor") == 0
		|| strcmp(classname, "Class Engine.SceneCaptureCubeMapActor") == 0
		|| strcmp(classname, "Class Engine.SceneCapturePortalActor") == 0
		|| strcmp(classname, "Class Engine.SceneCaptureReflectActor") == 0
		|| strcmp(classname, "Class Engine.ScoreBoard") == 0
		|| strcmp(classname, "Class Engine.SpeedTreeActor") == 0
		|| strcmp(classname, "Class Engine.StaticMeshActorBase") == 0
		|| strcmp(classname, "Class Engine.StaticMeshActorBasedOnExtremeContent") == 0
		|| strcmp(classname, "Class Engine.StaticMeshCollectionActor") == 0
		|| strcmp(classname, "Class Engine.TargetPoint") == 0
		|| strcmp(classname, "Class Engine.Terrain") == 0
		|| strcmp(classname, "Class Engine.Trigger") == 0
		|| strcmp(classname, "Class Engine.TriggerStreamingLevel") == 0
		|| strcmp(classname, "Class Engine.TriggerVolume") == 0
		|| strcmp(classname, "Class Engine.Trigger_Dynamic") == 0
		|| strcmp(classname, "Class Engine.Trigger_LOS") == 0
		|| strcmp(classname, "Class Engine.TriggeredPath") == 0
		|| strcmp(classname, "Class Engine.Volume") == 0
		|| strcmp(classname, "Class Engine.VolumePathNode") == 0
		|| strcmp(classname, "Class Engine.VolumeTimer") == 0
		|| strcmp(classname, "Class Engine.WaterVolume") == 0
		|| strcmp(classname, "Class Engine.WindDirectionalSource") == 0
		|| strcmp(classname, "Class Engine.ZoneInfo") == 0
		|| strcmp(classname, "Class GameFramework.GameExplosionActor") == 0
		|| strcmp(classname, "Class GameFramework.GameHUD") == 0
		|| strcmp(classname, "Class TgGame.TgActionPoint") == 0
		|| strcmp(classname, "Class TgGame.TgActorFactory") == 0
		|| strcmp(classname, "Class TgGame.TgAlarmPoint") == 0
		|| strcmp(classname, "Class TgGame.TgAnnouncer") == 0
		|| strcmp(classname, "Class TgGame.TgBeaconFactory") == 0
		|| strcmp(classname, "Class TgGame.TgBotEncounterVolume") == 0
		|| strcmp(classname, "Class TgGame.TgBotFactory") == 0
		|| strcmp(classname, "Class TgGame.TgBotFactorySpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgBotStart") == 0
		|| strcmp(classname, "Class TgGame.TgCollisionProxy") == 0
		|| strcmp(classname, "Class TgGame.TgCollisionProxy_Vortex") == 0
		|| strcmp(classname, "Class TgGame.TgCoverPoint") == 0
		|| strcmp(classname, "Class TgGame.TgDecalActor_Logo") == 0
		|| strcmp(classname, "Class TgGame.TgDefensePoint") == 0
		|| strcmp(classname, "Class TgGame.TgDeployableFactory") == 0
		|| strcmp(classname, "Class TgGame.TgDestructibleFactory") == 0
		|| strcmp(classname, "Class TgGame.TgDeviceVolume") == 0
		|| strcmp(classname, "Class TgGame.TgDeviceVolumeInfo") == 0
		|| strcmp(classname, "Class TgGame.TgDummyActor") == 0
		|| strcmp(classname, "Class TgGame.TgElevatingVolume") == 0
		|| strcmp(classname, "Class TgGame.TgGame") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Arena") == 0
		|| strcmp(classname, "Class TgGame.TgGame_CTF") == 0
		|| strcmp(classname, "Class TgGame.TgGame_City") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Control") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Defense") == 0
		|| strcmp(classname, "Class TgGame.TgGame_DualCTF") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Escort") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Mission") == 0
		|| strcmp(classname, "Class TgGame.TgGame_OpenWorld") == 0
		|| strcmp(classname, "Class TgGame.TgGame_OpenWorldPVE") == 0
		|| strcmp(classname, "Class TgGame.TgGame_OpenWorldPVP") == 0
		|| strcmp(classname, "Class TgGame.TgGame_PointRotation") == 0
		|| strcmp(classname, "Class TgGame.TgGame_Ticket") == 0
		|| strcmp(classname, "Class TgGame.TgHUD") == 0
		|| strcmp(classname, "Class TgGame.TgHeightFog") == 0
		|| strcmp(classname, "Class TgGame.TgHelpAlertVolume") == 0
		|| strcmp(classname, "Class TgGame.TgHexItemFactory") == 0
		|| strcmp(classname, "Class TgGame.TgHitDisplayActor") == 0
		|| strcmp(classname, "Class TgGame.TgHoldSpot") == 0
		|| strcmp(classname, "Class TgGame.TgMeshAssembly") == 0
		|| strcmp(classname, "Class TgGame.TgMiniMapActor") == 0
		|| strcmp(classname, "Class TgGame.TgMissionListVolume") == 0
		|| strcmp(classname, "Class TgGame.TgModifyPawnPropertiesVolume") == 0
		|| strcmp(classname, "Class TgGame.TgMorphFX") == 0
		|| strcmp(classname, "Class TgGame.TgNavigationPoint") == 0
		|| strcmp(classname, "Class TgGame.TgNavigationPointSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgNewsStand") == 0
		|| strcmp(classname, "Class TgGame.TgOmegaVolume") == 0
		|| strcmp(classname, "Class TgGame.TgPhysAnimTestActor") == 0
		|| strcmp(classname, "Class TgGame.TgPlayerController") == 0
		|| strcmp(classname, "Class TgGame.TgPlayerCountVolume") == 0
		|| strcmp(classname, "Class TgGame.TgPointOfInterest") == 0
		|| strcmp(classname, "Class TgGame.TgQueuedAnnouncement") == 0
		|| strcmp(classname, "Class TgGame.TgReferenceArray") == 0
		|| strcmp(classname, "Class TgGame.TgScoreboard") == 0
		|| strcmp(classname, "Class TgGame.TgSoundInsulationVolume") == 0
		|| strcmp(classname, "Class TgGame.TgStartPoint") == 0
		|| strcmp(classname, "Class TgGame.TgStartpointPortalNetwork") == 0
		|| strcmp(classname, "Class TgGame.TgStaticMeshActor_Logo") == 0
		|| strcmp(classname, "Class TgGame.TgTeamBlocker") == 0
		|| strcmp(classname, "Class TgGame.TgTeamMarker") == 0
		|| strcmp(classname, "Class TgGame.TgTeamPlayerStart") == 0
		|| strcmp(classname, "Class TgGame.TgTeamScoreboard") == 0
		|| strcmp(classname, "Class TgGame.TgTeleportPlayerVolume") == 0
		|| strcmp(classname, "Class TgGame.TgTrigger_Instance") == 0
		|| strcmp(classname, "Class TgGame.TgTrigger_Use") == 0
		|| strcmp(classname, "Class TgGame.TgVolumePathNode") == 0
		|| strcmp(classname, "Class TgGame.TgWaterVolume") == 0
		|| strcmp(classname, "Class TgGame.TgWindManager") == 0
	) {
		if (actor->RemoteRole == 1) {
			if (((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->bReplicateMovement) && actor->RemoteRole == 1) {
				DO_REP(AActor, Base, ObjectProperty_Engine_Actor_Base);
			}
			if (((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->bReplicateMovement) && (actor->RemoteRole == 1) && actor->bNetInitial || actor->bUpdateSimulatedPosition) {
				DO_REP(AActor, Physics, ByteProperty_Engine_Actor_Physics);
				DO_REP(AActor, Velocity, StructProperty_Engine_Actor_Velocity);
			}
			if ((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) {
				DO_REP(AActor, RemoteRole, ByteProperty_Engine_Actor_RemoteRole);
				DO_REP(AActor, Role, ByteProperty_Engine_Actor_Role);
				DO_REP(AActor, bNetOwner, BoolProperty_Engine_Actor_bNetOwner);
				DO_REP(AActor, bTearOff, BoolProperty_Engine_Actor_bTearOff);
			}
			if (((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) {
				DO_REP(AActor, DrawScale, FloatProperty_Engine_Actor_DrawScale);
				DO_REP(AActor, ReplicatedCollisionType, ByteProperty_Engine_Actor_ReplicatedCollisionType);
				DO_REP(AActor, bCollideActors, BoolProperty_Engine_Actor_bCollideActors);
				DO_REP(AActor, bCollideWorld, BoolProperty_Engine_Actor_bCollideWorld);
			}
			if ((((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) && actor->bCollideActors || actor->bCollideWorld) {
				DO_REP(AActor, bBlockActors, BoolProperty_Engine_Actor_bBlockActors);
				DO_REP(AActor, bProjTarget, BoolProperty_Engine_Actor_bProjTarget);
			}
			if ((((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) && actor->bReplicateInstigator) {
				DO_REP(AActor, Instigator, ObjectProperty_Engine_Actor_Instigator);
			}
			if (((actor->bNetOwner && !actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) {
				DO_REP(AActor, Owner, ObjectProperty_Engine_Actor_Owner);
			}
		}
	}
	if (strcmp(classname, "Class Engine.AmbientSoundSimpleToggleable") == 0) {
		if (actor->Role == 3) {
			DO_REP(AAmbientSoundSimpleToggleable, bCurrentlyPlaying, BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying);
		}
	}
	if (
		strcmp(classname, "Class Engine.CameraActor") == 0
		|| strcmp(classname, "Class Engine.DynamicCameraActor") == 0
		|| strcmp(classname, "Class TgGame.TgInterpolatingCameraActor") == 0
		|| strcmp(classname, "Class TgGame.TgLevelCamera") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ACameraActor, AspectRatio, FloatProperty_Engine_CameraActor_AspectRatio);
			DO_REP(ACameraActor, FOVAngle, FloatProperty_Engine_CameraActor_FOVAngle);
		}
	}
	if (
		strcmp(classname, "Class Engine.Controller") == 0
		|| strcmp(classname, "Class Engine.AIController") == 0
		|| strcmp(classname, "Class Engine.Admin") == 0
		|| strcmp(classname, "Class Engine.DebugCameraController") == 0
		|| strcmp(classname, "Class Engine.PlayerController") == 0
		|| strcmp(classname, "Class GameFramework.GameAIController") == 0
		|| strcmp(classname, "Class GameFramework.GamePlayerController") == 0
		|| strcmp(classname, "Class TgGame.TgAIController") == 0
		|| strcmp(classname, "Class TgGame.TgDebugCameraController") == 0
		|| strcmp(classname, "Class TgGame.TgPlayerController") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AController, Pawn, ObjectProperty_Engine_Controller_Pawn);
			DO_REP(AController, PlayerReplicationInfo, ObjectProperty_Engine_Controller_PlayerReplicationInfo);
		}
	}
	if (strcmp(classname, "Class Engine.CrowdAttractor") == 0) {
		if (actor->bNoDelete) {
			DO_REP(ACrowdAttractor, bAttractorEnabled, BoolProperty_Engine_CrowdAttractor_bAttractorEnabled);
		}
	}
	if (strcmp(classname, "Class Engine.CrowdReplicationActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ACrowdReplicationActor, DestroyAllCount, IntProperty_Engine_CrowdReplicationActor_DestroyAllCount);
			DO_REP(ACrowdReplicationActor, Spawner, ObjectProperty_Engine_CrowdReplicationActor_Spawner);
			DO_REP(ACrowdReplicationActor, bSpawningActive, BoolProperty_Engine_CrowdReplicationActor_bSpawningActive);
		}
	}
	if (strcmp(classname, "Class Engine.DroppedPickup") == 0) {
		if (actor->Role == 3) {
			DO_REP(ADroppedPickup, InventoryClass, ClassProperty_Engine_DroppedPickup_InventoryClass);
			DO_REP(ADroppedPickup, bFadeOut, BoolProperty_Engine_DroppedPickup_bFadeOut);
		}
	}
	if (
		strcmp(classname, "Class Engine.DynamicSMActor") == 0
		|| strcmp(classname, "Class Engine.DynamicSMActor_Spawnable") == 0
		|| strcmp(classname, "Class Engine.InterpActor") == 0
		|| strcmp(classname, "Class Engine.KActor") == 0
		|| strcmp(classname, "Class Engine.KActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgDoor") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicDestructible") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgInterpActor") == 0
		|| strcmp(classname, "Class TgGame.TgKActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKismetTestActor") == 0
		|| strcmp(classname, "Class TgGame.TgObjectiveAttachActor") == 0
	) {
		if (actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ADynamicSMActor, ReplicatedMaterial, ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial);
			DO_REP(ADynamicSMActor, ReplicatedMesh, ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh);
			DO_REP(ADynamicSMActor, ReplicatedMeshRotation, StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation);
			DO_REP(ADynamicSMActor, ReplicatedMeshScale3D, StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D);
			DO_REP(ADynamicSMActor, ReplicatedMeshTranslation, StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation);
			DO_REP(ADynamicSMActor, bForceStaticDecals, BoolProperty_Engine_DynamicSMActor_bForceStaticDecals);
		}
	}
	if (
		strcmp(classname, "Class Engine.Emitter") == 0
		|| strcmp(classname, "Class Engine.EmitterCameraLensEffectBase") == 0
		|| strcmp(classname, "Class Engine.EmitterSpawnable") == 0
		|| strcmp(classname, "Class Engine.PhysXEmitterSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgEmitter") == 0
		|| strcmp(classname, "Class TgGame.TgEmitterCrashlanding") == 0
		|| strcmp(classname, "Class TgGame.TgEmitterSpawnable") == 0
	) {
		if (actor->bNoDelete) {
			DO_REP(AEmitter, bCurrentlyActive, BoolProperty_Engine_Emitter_bCurrentlyActive);
		}
	}
	if (strcmp(classname, "Class Engine.EmitterSpawnable") == 0) {
		if (actor->bNetInitial) {
			DO_REP(AEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate);
		}
	}
	if (strcmp(classname, "Class Engine.FluidInfluenceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(AFluidInfluenceActor, bActive, BoolProperty_Engine_FluidInfluenceActor_bActive);
			DO_REP(AFluidInfluenceActor, bToggled, BoolProperty_Engine_FluidInfluenceActor_bToggled);
		}
	}
	if (
		strcmp(classname, "Class Engine.FogVolumeDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeConeDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeConstantDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeLinearHalfspaceDensityInfo") == 0
		|| strcmp(classname, "Class Engine.FogVolumeSphericalDensityInfo") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(AFogVolumeDensityInfo, bEnabled, BoolProperty_Engine_FogVolumeDensityInfo_bEnabled);
		}
	}
	if (
		strcmp(classname, "Class Engine.GameReplicationInfo") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_Game") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_GameOpenWorld") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AGameReplicationInfo, MatchID, IntProperty_Engine_GameReplicationInfo_MatchID);
			DO_REP(AGameReplicationInfo, Winner, ObjectProperty_Engine_GameReplicationInfo_Winner);
			DO_REP(AGameReplicationInfo, bMatchHasBegun, BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun);
			DO_REP(AGameReplicationInfo, bMatchIsOver, BoolProperty_Engine_GameReplicationInfo_bMatchIsOver);
			DO_REP(AGameReplicationInfo, bStopCountDown, BoolProperty_Engine_GameReplicationInfo_bStopCountDown);
		}
		if ((!actor->bNetInitial && actor->bNetDirty) && actor->Role == 3) {
			DO_REP(AGameReplicationInfo, RemainingMinute, IntProperty_Engine_GameReplicationInfo_RemainingMinute);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(AGameReplicationInfo, AdminEmail, StrProperty_Engine_GameReplicationInfo_AdminEmail);
			DO_REP(AGameReplicationInfo, AdminName, StrProperty_Engine_GameReplicationInfo_AdminName);
			DO_REP(AGameReplicationInfo, ElapsedTime, IntProperty_Engine_GameReplicationInfo_ElapsedTime);
			DO_REP(AGameReplicationInfo, GameClass, ClassProperty_Engine_GameReplicationInfo_GameClass);
			DO_REP(AGameReplicationInfo, GoalScore, IntProperty_Engine_GameReplicationInfo_GoalScore);
			DO_REP(AGameReplicationInfo, MaxLives, IntProperty_Engine_GameReplicationInfo_MaxLives);
			DO_REP(AGameReplicationInfo, MessageOfTheDay, StrProperty_Engine_GameReplicationInfo_MessageOfTheDay);
			DO_REP(AGameReplicationInfo, RemainingTime, IntProperty_Engine_GameReplicationInfo_RemainingTime);
			DO_REP(AGameReplicationInfo, ServerName, StrProperty_Engine_GameReplicationInfo_ServerName);
			DO_REP(AGameReplicationInfo, ServerRegion, IntProperty_Engine_GameReplicationInfo_ServerRegion);
			DO_REP(AGameReplicationInfo, ShortName, StrProperty_Engine_GameReplicationInfo_ShortName);
			DO_REP(AGameReplicationInfo, TimeLimit, IntProperty_Engine_GameReplicationInfo_TimeLimit);
			DO_REP(AGameReplicationInfo, bIsArbitrated, BoolProperty_Engine_GameReplicationInfo_bIsArbitrated);
			DO_REP(AGameReplicationInfo, bTrackStats, BoolProperty_Engine_GameReplicationInfo_bTrackStats);
		}
	}
	if (strcmp(classname, "Class Engine.HeightFog") == 0) {
		if (actor->Role == 3) {
			DO_REP(AHeightFog, bEnabled, BoolProperty_Engine_HeightFog_bEnabled);
		}
	}
	if (
		strcmp(classname, "Class Engine.Inventory") == 0
		|| strcmp(classname, "Class Engine.Weapon") == 0
		|| strcmp(classname, "Class GameFramework.GameWeapon") == 0
		|| strcmp(classname, "Class TgGame.TgDevice") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_HitPulse") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_MeleeDualWield") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Morale") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewMelee") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewRange") == 0
	) {
		if (((actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventory, InvManager, ObjectProperty_Engine_Inventory_InvManager);
			DO_REP(AInventory, Inventory, ObjectProperty_Engine_Inventory_Inventory);
		}
	}
	if (
		strcmp(classname, "Class Engine.InventoryManager") == 0
		|| strcmp(classname, "Class TgGame.TgInventoryManager") == 0
	) {
		if ((((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventoryManager, InventoryChain, ObjectProperty_Engine_InventoryManager_InventoryChain);
		}
	}
	if (
		strcmp(classname, "Class Engine.KActor") == 0
		|| strcmp(classname, "Class Engine.KActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKActorSpawnable") == 0
	) {
		if (!((AKActor*)actor)->bNeedsRBStateReplication && actor->Role == 3) {
			DO_REP(AKActor, RBState, StructProperty_Engine_KActor_RBState);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(AKActor, ReplicatedDrawScale3D, StructProperty_Engine_KActor_ReplicatedDrawScale3D);
			DO_REP(AKActor, bWakeOnLevelStart, BoolProperty_Engine_KActor_bWakeOnLevelStart);
		}
	}
	if (
		strcmp(classname, "Class Engine.KAsset") == 0
		|| strcmp(classname, "Class Engine.KAssetSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKAssetSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgKAsset_ClientSideSim") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(AKAsset, ReplicatedMesh, ObjectProperty_Engine_KAsset_ReplicatedMesh);
			DO_REP(AKAsset, ReplicatedPhysAsset, ObjectProperty_Engine_KAsset_ReplicatedPhysAsset);
		}
	}
	if (strcmp(classname, "Class Engine.LensFlareSource") == 0) {
		if (actor->bNoDelete) {
			DO_REP(ALensFlareSource, bCurrentlyActive, BoolProperty_Engine_LensFlareSource_bCurrentlyActive);
		}
	}
	if (
		strcmp(classname, "Class Engine.Light") == 0
		|| strcmp(classname, "Class Engine.DirectionalLight") == 0
		|| strcmp(classname, "Class Engine.DirectionalLightToggleable") == 0
		|| strcmp(classname, "Class Engine.PointLight") == 0
		|| strcmp(classname, "Class Engine.PointLightMovable") == 0
		|| strcmp(classname, "Class Engine.PointLightToggleable") == 0
		|| strcmp(classname, "Class Engine.SkyLight") == 0
		|| strcmp(classname, "Class Engine.SkyLightToggleable") == 0
		|| strcmp(classname, "Class Engine.SpotLight") == 0
		|| strcmp(classname, "Class Engine.SpotLightMovable") == 0
		|| strcmp(classname, "Class Engine.SpotLightToggleable") == 0
		|| strcmp(classname, "Class Engine.StaticLightCollectionActor") == 0
		|| strcmp(classname, "Class TgGame.TgCharacterBuilderLight") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ALight, bEnabled, BoolProperty_Engine_Light_bEnabled);
		}
	}
	if (strcmp(classname, "Class Engine.MatineeActor") == 0) {
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(AMatineeActor, InterpAction, ObjectProperty_Engine_MatineeActor_InterpAction);
		}
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AMatineeActor, PlayRate, FloatProperty_Engine_MatineeActor_PlayRate);
			DO_REP(AMatineeActor, Position, FloatProperty_Engine_MatineeActor_Position);
			DO_REP(AMatineeActor, bIsPlaying, BoolProperty_Engine_MatineeActor_bIsPlaying);
			DO_REP(AMatineeActor, bPaused, BoolProperty_Engine_MatineeActor_bPaused);
			DO_REP(AMatineeActor, bReversePlayback, BoolProperty_Engine_MatineeActor_bReversePlayback);
		}
	}
	if (
		strcmp(classname, "Class Engine.NxForceField") == 0
		|| strcmp(classname, "Class Engine.NxCylindricalForceField") == 0
		|| strcmp(classname, "Class Engine.NxCylindricalForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldGeneric") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldRadial") == 0
		|| strcmp(classname, "Class Engine.NxForceFieldTornado") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceField") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceFieldBox") == 0
		|| strcmp(classname, "Class Engine.NxGenericForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxRadialCustomForceField") == 0
		|| strcmp(classname, "Class Engine.NxRadialForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoAngularForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoAngularForceFieldCapsule") == 0
		|| strcmp(classname, "Class Engine.NxTornadoForceField") == 0
		|| strcmp(classname, "Class Engine.NxTornadoForceFieldCapsule") == 0
	) {
		if (actor->bNetDirty) {
			DO_REP(ANxForceField, bForceActive, BoolProperty_Engine_NxForceField_bForceActive);
		}
	}



	if (
		strcmp(classname, "Class Engine.Pawn") == 0
		|| strcmp(classname, "Class TgGame.TgPawn") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AVCompositeWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Ambush") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AndroidMinion") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AttackTransport") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss_Destroyer") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Brawler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_CTR") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Character") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ColonyEye") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Destructible") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Detonator") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Dismantler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_DuneCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Alchemist") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Assassin") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_EscortRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_FlyingBoss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_GroundPetA") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Guardian") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_HoverShieldSphere") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hunter") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Inquisitor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Interact_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Iris") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Juggernaut") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Marauder") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NewWasp") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Raptor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Reaper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_RecursiveSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Remote") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Robot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Scanner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ScannerRecursive") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Siege") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeBarrage") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeHover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeRapidFire") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Sniper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SonoranCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SupportForeman") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Switchblade") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Tentacle") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ThinkTank") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TreadRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Turret") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVAFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVARocket") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlame") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretPlasma") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_UberWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vanguard") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_VanityPet") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vulcan") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Warlord") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_WaspSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Widow") == 0
		|| strcmp(classname, "Class Engine.Scout") == 0
		|| strcmp(classname, "Class Engine.Vehicle") == 0
		|| strcmp(classname, "Class Engine.SVehicle") == 0
		|| strcmp(classname, "Class GameFramework.GamePawn") == 0
		|| strcmp(classname, "Class GameFramework.GameVehicle") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(APawn, DrivenVehicle, ObjectProperty_Engine_Pawn_DrivenVehicle);
			DO_REP(APawn, FlashLocation, StructProperty_Engine_Pawn_FlashLocation);
			DO_REP(APawn, Health, IntProperty_Engine_Pawn_Health);
			DO_REP(APawn, HitDamageType, ClassProperty_Engine_Pawn_HitDamageType);
			DO_REP(APawn, PlayerReplicationInfo, ObjectProperty_Engine_Pawn_PlayerReplicationInfo);
			DO_REP(APawn, TakeHitLocation, StructProperty_Engine_Pawn_TakeHitLocation);
			DO_REP(APawn, bIsWalking, BoolProperty_Engine_Pawn_bIsWalking);
			DO_REP(APawn, bSimulateGravity, BoolProperty_Engine_Pawn_bSimulateGravity);
		}
		if ((actor->bNetDirty && actor->bNetOwner) && actor->Role == 3) {
			DO_REP(APawn, AccelRate, FloatProperty_Engine_Pawn_AccelRate);
			DO_REP(APawn, AirControl, FloatProperty_Engine_Pawn_AirControl);
			DO_REP(APawn, AirSpeed, FloatProperty_Engine_Pawn_AirSpeed);
			DO_REP(APawn, Controller, ObjectProperty_Engine_Pawn_Controller);
			DO_REP(APawn, GroundSpeed, FloatProperty_Engine_Pawn_GroundSpeed);
			DO_REP(APawn, InvManager, ObjectProperty_Engine_Pawn_InvManager);
			DO_REP(APawn, JumpZ, FloatProperty_Engine_Pawn_JumpZ);
			DO_REP(APawn, WaterSpeed, FloatProperty_Engine_Pawn_WaterSpeed);
		}
		if ((actor->bNetDirty && !actor->bNetOwner || FALSE) && actor->Role == 3) {
			DO_REP(APawn, FiringMode, ByteProperty_Engine_Pawn_FiringMode);
			DO_REP(APawn, FlashCount, ByteProperty_Engine_Pawn_FlashCount);
			DO_REP(APawn, bIsCrouched, BoolProperty_Engine_Pawn_bIsCrouched);
		}
		if ((actor->bTearOff && actor->bNetDirty) && actor->Role == 3) {
			DO_REP(APawn, TearOffMomentum, StructProperty_Engine_Pawn_TearOffMomentum);
		}
		if ((!actor->bNetOwner || FALSE) && actor->Role == 3) {
			DO_REP(APawn, RemoteViewPitch, ByteProperty_Engine_Pawn_RemoteViewPitch);
		}
	}
	if (strcmp(classname, "Class Engine.PhysXEmitterSpawnable") == 0) {
		if (actor->bNetInitial) {
			DO_REP(APhysXEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate);
		}
	}
	if (
		strcmp(classname, "Class Engine.PickupFactory") == 0
		|| strcmp(classname, "Class TgGame.TgPickupFactory") == 0
		|| strcmp(classname, "Class TgGame.TgPickupFactory_Item") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(APickupFactory, bPickupHidden, BoolProperty_Engine_PickupFactory_bPickupHidden);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(APickupFactory, InventoryType, ClassProperty_Engine_PickupFactory_InventoryType);
		}
	}
	if (
		strcmp(classname, "Class Engine.PlayerController") == 0
		|| strcmp(classname, "Class Engine.Admin") == 0
		|| strcmp(classname, "Class Engine.DebugCameraController") == 0
		|| strcmp(classname, "Class GameFramework.GamePlayerController") == 0
		|| strcmp(classname, "Class TgGame.TgDebugCameraController") == 0
		|| strcmp(classname, "Class TgGame.TgPlayerController") == 0
	) {
		if (((actor->bNetOwner && actor->Role == 3) && ((APlayerController*)actor)->ViewTarget != ((APlayerController*)actor)->Pawn) && ((APlayerController*)actor)->ViewTarget != NULL) {
			DO_REP(APlayerController, TargetEyeHeight, FloatProperty_Engine_PlayerController_TargetEyeHeight);
			DO_REP(APlayerController, TargetViewRotation, StructProperty_Engine_PlayerController_TargetViewRotation);
		}
	}
	if (
		strcmp(classname, "Class Engine.PlayerReplicationInfo") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_Player") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(APlayerReplicationInfo, Deaths, FloatProperty_Engine_PlayerReplicationInfo_Deaths);
			DO_REP(APlayerReplicationInfo, PlayerAlias, StrProperty_Engine_PlayerReplicationInfo_PlayerAlias);
			DO_REP(APlayerReplicationInfo, PlayerLocationHint, ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint);
			DO_REP(APlayerReplicationInfo, PlayerName, StrProperty_Engine_PlayerReplicationInfo_PlayerName);
			DO_REP(APlayerReplicationInfo, PlayerSkill, IntProperty_Engine_PlayerReplicationInfo_PlayerSkill);
			DO_REP(APlayerReplicationInfo, Score, FloatProperty_Engine_PlayerReplicationInfo_Score);
			DO_REP(APlayerReplicationInfo, StartTime, IntProperty_Engine_PlayerReplicationInfo_StartTime);
			DO_REP(APlayerReplicationInfo, Team, ObjectProperty_Engine_PlayerReplicationInfo_Team);
			DO_REP(APlayerReplicationInfo, UniqueId, StructProperty_Engine_PlayerReplicationInfo_UniqueId);
			DO_REP(APlayerReplicationInfo, bAdmin, BoolProperty_Engine_PlayerReplicationInfo_bAdmin);
			DO_REP(APlayerReplicationInfo, bHasFlag, BoolProperty_Engine_PlayerReplicationInfo_bHasFlag);
			DO_REP(APlayerReplicationInfo, bIsFemale, BoolProperty_Engine_PlayerReplicationInfo_bIsFemale);
			DO_REP(APlayerReplicationInfo, bIsSpectator, BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator);
			DO_REP(APlayerReplicationInfo, bOnlySpectator, BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator);
			DO_REP(APlayerReplicationInfo, bOutOfLives, BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives);
			DO_REP(APlayerReplicationInfo, bReadyToPlay, BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay);
			DO_REP(APlayerReplicationInfo, bWaitingPlayer, BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer);
		}
		if ((actor->bNetDirty && actor->Role == 3) && !actor->bNetOwner) {
			DO_REP(APlayerReplicationInfo, PacketLoss, ByteProperty_Engine_PlayerReplicationInfo_PacketLoss);
			DO_REP(APlayerReplicationInfo, Ping, ByteProperty_Engine_PlayerReplicationInfo_Ping);
			DO_REP(APlayerReplicationInfo, SplitscreenIndex, IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(APlayerReplicationInfo, PlayerID, IntProperty_Engine_PlayerReplicationInfo_PlayerID);
			DO_REP(APlayerReplicationInfo, bBot, BoolProperty_Engine_PlayerReplicationInfo_bBot);
			DO_REP(APlayerReplicationInfo, bIsInactive, BoolProperty_Engine_PlayerReplicationInfo_bIsInactive);
		}
	}
	if (
		strcmp(classname, "Class Engine.PostProcessVolume") == 0
		|| strcmp(classname, "Class TgGame.TgPostProcessVolume") == 0
	) {
		if (actor->bNetDirty) {
			DO_REP(APostProcessVolume, bEnabled, BoolProperty_Engine_PostProcessVolume_bEnabled);
		}
	}
	if (
		strcmp(classname, "Class Engine.Projectile") == 0
		|| strcmp(classname, "Class GameFramework.GameProjectile") == 0
		|| strcmp(classname, "Class TgGame.TgProjectile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Teleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_StraightTeleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Rocket") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Net") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Mortar") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Missile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grapple") == 0
		|| strcmp(classname, "Class TgGame.TgProj_FreeGrenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Deployable") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bounce") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bot") == 0
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(AProjectile, MaxSpeed, FloatProperty_Engine_Projectile_MaxSpeed);
			DO_REP(AProjectile, Speed, FloatProperty_Engine_Projectile_Speed);
		}
	}
	if (strcmp(classname, "Class Engine.RB_CylindricalForceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_CylindricalForceActor, bForceActive, BoolProperty_Engine_RB_CylindricalForceActor_bForceActive);
		}
	}
	if (strcmp(classname, "Class Engine.RB_LineImpulseActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_LineImpulseActor, ImpulseCount, ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount);
		}
	}
	if (strcmp(classname, "Class Engine.RB_RadialForceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialForceActor, bForceActive, BoolProperty_Engine_RB_RadialForceActor_bForceActive);
		}
	}
	if (strcmp(classname, "Class Engine.RB_RadialImpulseActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialImpulseActor, ImpulseCount, ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount);
		}
	}
	if (
		strcmp(classname, "Class Engine.SVehicle") == 0
		|| strcmp(classname, "Class GameFramework.GameVehicle") == 0
	) {
		if (actor->Physics == 10) {
			DO_REP(ASVehicle, MaxSpeed, FloatProperty_Engine_SVehicle_MaxSpeed);
			DO_REP(ASVehicle, VState, StructProperty_Engine_SVehicle_VState);
		}
	}
	if (
		strcmp(classname, "Class Engine.SkeletalMeshActor") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorBasedOnExtremeContent") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorMAT") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorMATSpawnable") == 0
		|| strcmp(classname, "Class Engine.SkeletalMeshActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgNavRouteIndicator") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorGenericUIPreview") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorNPC") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorNPCVendor") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActorSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_CharacterBuilder") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_CharacterBuilderSpawnable") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_Composite") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_EquipScreen") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor_MeleePreVis") == 0
		|| strcmp(classname, "Class TgGame.TgSkeletalMeshActor") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ASkeletalMeshActor, ReplicatedMaterial, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial);
			DO_REP(ASkeletalMeshActor, ReplicatedMesh, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh);
		}
	}
	if (
		strcmp(classname, "Class Engine.TeamInfo") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_TaskForce") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATeamInfo, Score, FloatProperty_Engine_TeamInfo_Score);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATeamInfo, TeamIndex, IntProperty_Engine_TeamInfo_TeamIndex);
			DO_REP(ATeamInfo, TeamName, StrProperty_Engine_TeamInfo_TeamName);
		}
	}
	if (
		strcmp(classname, "Class Engine.Teleporter") == 0
		|| strcmp(classname, "Class TgGame.TgTeleporter") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATeleporter, URL, StrProperty_Engine_Teleporter_URL);
			DO_REP(ATeleporter, bEnabled, BoolProperty_Engine_Teleporter_bEnabled);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATeleporter, TargetVelocity, StructProperty_Engine_Teleporter_TargetVelocity);
			DO_REP(ATeleporter, bChangesVelocity, BoolProperty_Engine_Teleporter_bChangesVelocity);
			DO_REP(ATeleporter, bChangesYaw, BoolProperty_Engine_Teleporter_bChangesYaw);
			DO_REP(ATeleporter, bReversesX, BoolProperty_Engine_Teleporter_bReversesX);
			DO_REP(ATeleporter, bReversesY, BoolProperty_Engine_Teleporter_bReversesY);
			DO_REP(ATeleporter, bReversesZ, BoolProperty_Engine_Teleporter_bReversesZ);
		}
	}
	if (
		strcmp(classname, "Class Engine.Vehicle") == 0
		|| strcmp(classname, "Class Engine.SVehicle") == 0
		|| strcmp(classname, "Class GameFramework.GameVehicle") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AVehicle, bDriving, BoolProperty_Engine_Vehicle_bDriving);
		}
		if ((actor->bNetDirty && (actor->bNetOwner || ((AVehicle*)actor)->Driver == NULL) || !((AVehicle*)actor)->Driver->bHidden) && actor->Role == 3) {
			DO_REP(AVehicle, Driver, ObjectProperty_Engine_Vehicle_Driver);
		}
	}
	if (strcmp(classname, "Class Engine.WorldInfo") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AWorldInfo, Pauser, ObjectProperty_Engine_WorldInfo_Pauser);
			DO_REP(AWorldInfo, ReplicatedMusicTrack, StructProperty_Engine_WorldInfo_ReplicatedMusicTrack);
			DO_REP(AWorldInfo, TimeDilation, FloatProperty_Engine_WorldInfo_TimeDilation);
			DO_REP(AWorldInfo, WorldGravityZ, FloatProperty_Engine_WorldInfo_WorldGravityZ);
			DO_REP(AWorldInfo, bHighPriorityLoading, BoolProperty_Engine_WorldInfo_bHighPriorityLoading);
		}
	}
	if (strcmp(classname, "Class TgGame.TgChestActor") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgChestActor, r_eChestState, ByteProperty_TgGame_TgChestActor_r_eChestState);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDeploy_BeaconEntrance") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_BeaconEntrance, r_bActive, BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDeploy_DestructibleCover") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_DestructibleCover, r_bHasFired, BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDeploy_Sensor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_Sensor, r_nSensorAudioWarning, IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning);
			DO_REP(ATgDeploy_Sensor, r_nTouchedPlayerCount, IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDeployable") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Artillery") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Beacon") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_BeaconEntrance") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_DestructibleCover") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_ForceField") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_Sensor") == 0
		|| strcmp(classname, "Class TgGame.TgDeploy_SweepSensor") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgDeployable, r_bDelayDeployed, BoolProperty_TgGame_TgDeployable_r_bDelayDeployed);
			DO_REP(ATgDeployable, r_nReplicateDestroyIt, IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDeployable, r_DRI, ObjectProperty_TgGame_TgDeployable_r_DRI);
			DO_REP(ATgDeployable, r_bInitialIsEnemy, BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy);
			DO_REP(ATgDeployable, r_bTakeDamage, BoolProperty_TgGame_TgDeployable_r_bTakeDamage);
			DO_REP(ATgDeployable, r_fClientProximityRadius, FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius);
			DO_REP(ATgDeployable, r_fCurrentDeployTime, FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime);
			DO_REP(ATgDeployable, r_nDeployableId, IntProperty_TgGame_TgDeployable_r_nDeployableId);
			DO_REP(ATgDeployable, r_nPhysicalType, IntProperty_TgGame_TgDeployable_r_nPhysicalType);
			DO_REP(ATgDeployable, r_nTickingTime, IntProperty_TgGame_TgDeployable_r_nTickingTime);
		}
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgDeployable, r_Owner, ObjectProperty_TgGame_TgDeployable_r_Owner);
			DO_REP(ATgDeployable, r_nOwnerFireMode, IntProperty_TgGame_TgDeployable_r_nOwnerFireMode);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgDevice") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_HitPulse") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_Morale") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewMelee") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_MeleeDualWield") == 0
		|| strcmp(classname, "Class TgGame.TgDevice_NewRange") == 0
	) {
		if (((actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventory, InvManager, ObjectProperty_Engine_Inventory_InvManager);
			DO_REP(AInventory, Inventory, ObjectProperty_Engine_Inventory_Inventory);
		}
		if (actor->Role == 3) {
			DO_REP(ATgDevice, CurrentFireMode, ByteProperty_TgGame_TgDevice_CurrentFireMode);
			DO_REP(ATgDevice, r_bIsStealthDevice, BoolProperty_TgGame_TgDevice_r_bIsStealthDevice);
			DO_REP(ATgDevice, r_eEquippedAt, ByteProperty_TgGame_TgDevice_r_eEquippedAt);
			DO_REP(ATgDevice, r_nInventoryId, IntProperty_TgGame_TgDevice_r_nInventoryId);
			DO_REP(ATgDevice, r_nMeleeComboSeed, IntProperty_TgGame_TgDevice_r_nMeleeComboSeed);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDevice, r_bConsumedOnDeath, BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath);
			DO_REP(ATgDevice, r_bConsumedOnUse, BoolProperty_TgGame_TgDevice_r_bConsumedOnUse);
			DO_REP(ATgDevice, r_nDeviceId, IntProperty_TgGame_TgDevice_r_nDeviceId);
			DO_REP(ATgDevice, r_nDeviceInstanceId, IntProperty_TgGame_TgDevice_r_nDeviceInstanceId);
			DO_REP(ATgDevice, r_nQualityValueId, IntProperty_TgGame_TgDevice_r_nQualityValueId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDevice_Morale") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDevice_Morale, r_bIsActivelyFiring, BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDoor") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDoor, r_bOpen, BoolProperty_TgGame_TgDoor_r_bOpen);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDoorMarker") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDoorMarker, r_eStatus, ByteProperty_TgGame_TgDoorMarker_r_eStatus);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDroppedItem") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDroppedItem, r_nItemId, IntProperty_TgGame_TgDroppedItem_r_nItemId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgDynamicDestructible") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicDestructible, r_nDestructibleId, IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId);
			DO_REP(ATgDynamicDestructible, r_pFactory, ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgDynamicSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicDestructible") == 0
		|| strcmp(classname, "Class TgGame.TgObjectiveAttachActor") == 0
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicSMActor, m_sAssembly, StrProperty_TgGame_TgDynamicSMActor_m_sAssembly);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgDynamicSMActor") == 0
		|| strcmp(classname, "Class TgGame.TgDynamicDestructible") == 0
		|| strcmp(classname, "Class TgGame.TgObjectiveAttachActor") == 0
	) {

		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicSMActor, r_EffectManager, ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager);
		}
		if (actor->Role == 3) {
			DO_REP(ATgDynamicSMActor, r_nHealth, IntProperty_TgGame_TgDynamicSMActor_r_nHealth);
		}
	}
	if (strcmp(classname, "Class TgGame.TgEffectManager") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgEffectManager, r_EventQueue, StructProperty_TgGame_TgEffectManager_r_EventQueue);
			DO_REP_ARRAY(0x10, ATgEffectManager, r_ManagedEffectList, StructProperty_TgGame_TgEffectManager_r_ManagedEffectList);
			DO_REP(ATgEffectManager, r_Owner, ObjectProperty_TgGame_TgEffectManager_r_Owner);
			DO_REP(ATgEffectManager, r_bRelevancyNotify, BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify);
			DO_REP(ATgEffectManager, r_nInvulnerableCount, IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount);
			DO_REP(ATgEffectManager, r_nNextQueueIndex, IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex);
		}
	}
	if (strcmp(classname, "Class TgGame.TgEmitter") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgEmitter, BoneName, NameProperty_TgGame_TgEmitter_BoneName);
		}
	}
	if (strcmp(classname, "Class TgGame.TgFlagCaptureVolume") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgFlagCaptureVolume, r_eCoalition, ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition);
			DO_REP(ATgFlagCaptureVolume, r_nTaskForce, ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce);
		}
	}
	if (strcmp(classname, "Class TgGame.TgFracturedStaticMeshActor") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgFracturedStaticMeshActor, r_EffectManager, ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager);
			DO_REP(ATgFracturedStaticMeshActor, r_TakeHitNotifier, IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier);
		}
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgFracturedStaticMeshActor, r_DamageRadius, FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius);
			DO_REP(ATgFracturedStaticMeshActor, r_HitDamageType, ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType);
			DO_REP(ATgFracturedStaticMeshActor, r_HitInfo, StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo);
			DO_REP(ATgFracturedStaticMeshActor, r_vTakeHitLocation, StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation);
			DO_REP(ATgFracturedStaticMeshActor, r_vTakeHitMomentum, StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum);
		}
	}
	if (strcmp(classname, "Class TgGame.TgHexLandMarkActor") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgHexLandMarkActor, r_nMeshAsmId, IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgInterpActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgInterpActor, r_sCurrState, StrProperty_TgGame_TgInterpActor_r_sCurrState);
		}
	}
	if (strcmp(classname, "Class TgGame.TgInventoryManager") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgInventoryManager, r_ItemCount, IntProperty_TgGame_TgInventoryManager_r_ItemCount);
		}
	}
	if (strcmp(classname, "Class TgGame.TgKismetTestActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgKismetTestActor, r_nCurrentTest, IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest);
			DO_REP(ATgKismetTestActor, r_nFailCount, IntProperty_TgGame_TgKismetTestActor_r_nFailCount);
			DO_REP(ATgKismetTestActor, r_nPassCount, IntProperty_TgGame_TgKismetTestActor_r_nPassCount);
		}
	}
	if (strcmp(classname, "Class TgGame.TgLevelCamera") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgLevelCamera, r_bEnabled, BoolProperty_TgGame_TgLevelCamera_r_bEnabled);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgMissionObjective") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_CTFBot") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_KOTH") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Bot") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Escort") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Kismet") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Proximity") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective, r_ObjectiveAssignment, ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment);
			DO_REP(ATgMissionObjective, r_bHasBeenCapturedOnce, BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce);
			DO_REP(ATgMissionObjective, r_bIsActive, BoolProperty_TgGame_TgMissionObjective_r_bIsActive);
			DO_REP(ATgMissionObjective, r_bIsLocked, BoolProperty_TgGame_TgMissionObjective_r_bIsLocked);
			DO_REP(ATgMissionObjective, r_bIsPending, BoolProperty_TgGame_TgMissionObjective_r_bIsPending);
			DO_REP(ATgMissionObjective, r_eOwningCoalition, ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition);
			DO_REP(ATgMissionObjective, r_eStatus, ByteProperty_TgGame_TgMissionObjective_r_eStatus);
			DO_REP(ATgMissionObjective, r_fCurrCaptureTime, FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime);
			DO_REP(ATgMissionObjective, r_fLastCompletedTime, FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime);
			DO_REP(ATgMissionObjective, r_nOwnerTaskForce, IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgMissionObjective, nObjectiveId, IntProperty_TgGame_TgMissionObjective_nObjectiveId);
			DO_REP(ATgMissionObjective, nPriority, IntProperty_TgGame_TgMissionObjective_nPriority);
			DO_REP(ATgMissionObjective, r_OpenWorldPlayerDefaultRole, ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole);
			DO_REP(ATgMissionObjective, r_bUsePendingState, BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState);
			DO_REP(ATgMissionObjective, r_eDefaultCoalition, ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgMissionObjective_Bot") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_CTFBot") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBot, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot);
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBotInfo, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo);
		}
	}
	if (strcmp(classname, "Class TgGame.TgMissionObjective_Escort") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Escort, r_AttachedActor, ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgMissionObjective_Proximity") == 0
		|| strcmp(classname, "Class TgGame.TgBaseObjective_KOTH") == 0
		|| strcmp(classname, "Class TgGame.TgMissionObjective_Escort") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Proximity, r_fCaptureRate, FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate);
		}
	}
	if (strcmp(classname, "Class TgGame.TgObjectiveAssignment") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgObjectiveAssignment, r_AssignedObjective, ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective);
			DO_REP(ATgObjectiveAssignment, r_Attackers, ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers);
			DO_REP(ATgObjectiveAssignment, r_Bots, ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots);
			DO_REP(ATgObjectiveAssignment, r_Defenders, ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders);
			DO_REP(ATgObjectiveAssignment, r_eState, ByteProperty_TgGame_TgObjectiveAssignment_r_eState);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgPawn") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AVCompositeWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Ambush") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AndroidMinion") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AttackTransport") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss_Destroyer") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Brawler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_CTR") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Character") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ColonyEye") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Destructible") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Detonator") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Dismantler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_DuneCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Alchemist") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Assassin") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_EscortRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_FlyingBoss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_GroundPetA") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Guardian") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_HoverShieldSphere") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hunter") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Inquisitor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Interact_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Iris") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Juggernaut") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Marauder") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NewWasp") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Raptor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Reaper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_RecursiveSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Remote") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Robot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Scanner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ScannerRecursive") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Siege") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeBarrage") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeHover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeRapidFire") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Sniper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SonoranCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SupportForeman") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Switchblade") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Tentacle") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ThinkTank") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TreadRobot") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Turret") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVAFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVARocket") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlame") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretPlasma") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_UberWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vanguard") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_VanityPet") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vulcan") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Warlord") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_WaspSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Widow") == 0
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgPawn, r_bIsBot, BoolProperty_TgGame_TgPawn_r_bIsBot);
			DO_REP(ATgPawn, r_bIsHenchman, BoolProperty_TgGame_TgPawn_r_bIsHenchman);
			DO_REP(ATgPawn, r_bNeedPlaySpawnFx, BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx);
			DO_REP(ATgPawn, r_fMakeVisibleIncreased, FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased);
			DO_REP(ATgPawn, r_nAllianceId, IntProperty_TgGame_TgPawn_r_nAllianceId);
			DO_REP(ATgPawn, r_nBodyMeshAsmId, IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId);
			DO_REP(ATgPawn, r_nBotRankValueId, IntProperty_TgGame_TgPawn_r_nBotRankValueId);
			DO_REP(ATgPawn, r_nPawnId, IntProperty_TgGame_TgPawn_r_nPawnId);
			DO_REP(ATgPawn, r_nPhysicalType, IntProperty_TgGame_TgPawn_r_nPhysicalType);
			DO_REP(ATgPawn, r_nPreyProfileType, IntProperty_TgGame_TgPawn_r_nPreyProfileType);
			DO_REP(ATgPawn, r_nProfileId, IntProperty_TgGame_TgPawn_r_nProfileId);
			DO_REP(ATgPawn, r_nProfileTypeValueId, IntProperty_TgGame_TgPawn_r_nProfileTypeValueId);
			DO_REP(ATgPawn, r_nSoundGroupId, IntProperty_TgGame_TgPawn_r_nSoundGroupId);
		}
		if ((actor->Role == 3) && !actor->bNetOwner) {
			DO_REP(ATgPawn, r_bInitialIsEnemy, BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy);
			DO_REP(ATgPawn, r_bMadeSound, ByteProperty_TgGame_TgPawn_r_bMadeSound);
			DO_REP(ATgPawn, r_eDesiredInHand, ByteProperty_TgGame_TgPawn_r_eDesiredInHand);
			DO_REP(ATgPawn, r_eEquippedInHandMode, IntProperty_TgGame_TgPawn_r_eEquippedInHandMode);
			DO_REP(ATgPawn, r_nReplicateHit, IntProperty_TgGame_TgPawn_r_nReplicateHit);
			DO_REP_ARRAY(25, ATgPawn, r_EquipDeviceInfo, StructProperty_TgGame_TgPawn_r_EquipDeviceInfo);
		}
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgPawn, r_ControlPawn, ObjectProperty_TgGame_TgPawn_r_ControlPawn);
			DO_REP(ATgPawn, r_CurrentOmegaVolume, ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume);
			DO_REP(ATgPawn, r_CurrentSubzoneBilboardVol, ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol);
			DO_REP(ATgPawn, r_CurrentSubzoneVol, ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol);
			DO_REP_ARRAY(0x2, ATgPawn, r_ScannerSettings, StructProperty_TgGame_TgPawn_r_ScannerSettings);
			DO_REP(ATgPawn, r_UIClockState, ByteProperty_TgGame_TgPawn_r_UIClockState);
			DO_REP(ATgPawn, r_UIClockTime, FloatProperty_TgGame_TgPawn_r_UIClockTime);
			DO_REP(ATgPawn, r_UITextBox1MessageID, IntProperty_TgGame_TgPawn_r_UITextBox1MessageID);
			DO_REP(ATgPawn, r_UITextBox1Packet, ByteProperty_TgGame_TgPawn_r_UITextBox1Packet);
			DO_REP(ATgPawn, r_UITextBox1Time, FloatProperty_TgGame_TgPawn_r_UITextBox1Time);
			DO_REP(ATgPawn, r_UITextBox2MessageID, IntProperty_TgGame_TgPawn_r_UITextBox2MessageID);
			DO_REP(ATgPawn, r_UITextBox2Packet, ByteProperty_TgGame_TgPawn_r_UITextBox2Packet);
			DO_REP(ATgPawn, r_UITextBox2Time, FloatProperty_TgGame_TgPawn_r_UITextBox2Time);
			DO_REP(ATgPawn, r_bAllowAddMoralePoints, BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints);
			DO_REP(ATgPawn, r_bDisableAllDevices, BoolProperty_TgGame_TgPawn_r_bDisableAllDevices);
			DO_REP(ATgPawn, r_bEnableCrafting, BoolProperty_TgGame_TgPawn_r_bEnableCrafting);
			DO_REP(ATgPawn, r_bEnableEquip, BoolProperty_TgGame_TgPawn_r_bEnableEquip);
			DO_REP(ATgPawn, r_bEnableSkills, BoolProperty_TgGame_TgPawn_r_bEnableSkills);
			DO_REP(ATgPawn, r_bInCombatFlag, BoolProperty_TgGame_TgPawn_r_bInCombatFlag);
			DO_REP(ATgPawn, r_bInGlobalOffhandCooldown, BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown);
			DO_REP(ATgPawn, r_fCurrentPowerPool, FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool);
			DO_REP(ATgPawn, r_fCurrentServerMoralePoints, FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints);
			DO_REP(ATgPawn, r_fMaxControlRange, FloatProperty_TgGame_TgPawn_r_fMaxControlRange);
			DO_REP(ATgPawn, r_fMaxPowerPool, FloatProperty_TgGame_TgPawn_r_fMaxPowerPool);
			DO_REP(ATgPawn, r_fMoraleRechargeRate, FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate);
			DO_REP(ATgPawn, r_fRequiredMoralePoints, FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints);
			DO_REP(ATgPawn, r_fSkillRating, FloatProperty_TgGame_TgPawn_r_fSkillRating);
			DO_REP(ATgPawn, r_nCurrency, IntProperty_TgGame_TgPawn_r_nCurrency);
			DO_REP(ATgPawn, r_nHZPoints, IntProperty_TgGame_TgPawn_r_nHZPoints);
			DO_REP(ATgPawn, r_nMoraleDeviceSlot, IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot);
			DO_REP(ATgPawn, r_nRestDeviceSlot, IntProperty_TgGame_TgPawn_r_nRestDeviceSlot);
			DO_REP(ATgPawn, r_nToken, IntProperty_TgGame_TgPawn_r_nToken);
			DO_REP(ATgPawn, r_nXp, IntProperty_TgGame_TgPawn_r_nXp);
		}
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP_ARRAY(0x20, ATgPawn, r_nFlashEvent, IntProperty_TgGame_TgPawn_r_nFlashEvent);
			DO_REP_ARRAY(0x20, ATgPawn, r_nFlashFireInfo, IntProperty_TgGame_TgPawn_r_nFlashFireInfo);
			DO_REP(ATgPawn, r_nFlashQueIndex, IntProperty_TgGame_TgPawn_r_nFlashQueIndex);
			DO_REP_ARRAY(0x20, ATgPawn, r_vFlashLocation, StructProperty_TgGame_TgPawn_r_vFlashLocation);
			DO_REP_ARRAY(0x20, ATgPawn, r_vFlashRayDir, StructProperty_TgGame_TgPawn_r_vFlashRayDir);
			DO_REP_ARRAY(0x20, ATgPawn, r_vFlashRefireTime, FloatProperty_TgGame_TgPawn_r_vFlashRefireTime);
			DO_REP_ARRAY(0x20, ATgPawn, r_vFlashSituationalAttack, IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack);

			DO_REP(ATgPawn, r_DistanceToPushback, FloatProperty_TgGame_TgPawn_r_DistanceToPushback);
			DO_REP(ATgPawn, r_EffectManager, ObjectProperty_TgGame_TgPawn_r_EffectManager);
			DO_REP(ATgPawn, r_FlightAcceleration, FloatProperty_TgGame_TgPawn_r_FlightAcceleration);
			DO_REP(ATgPawn, r_HangingRotation, StructProperty_TgGame_TgPawn_r_HangingRotation);
			DO_REP(ATgPawn, r_Owner, ObjectProperty_TgGame_TgPawn_r_Owner);
			DO_REP(ATgPawn, r_Pet, ObjectProperty_TgGame_TgPawn_r_Pet);
			DO_REP(ATgPawn, r_PlayAnimation, StructProperty_TgGame_TgPawn_r_PlayAnimation);
			DO_REP(ATgPawn, r_PushbackDirection, StructProperty_TgGame_TgPawn_r_PushbackDirection);
			DO_REP(ATgPawn, r_Target, ObjectProperty_TgGame_TgPawn_r_Target);
			DO_REP(ATgPawn, r_TargetActor, ObjectProperty_TgGame_TgPawn_r_TargetActor);
			// DO_REP(ATgPawn, r_aDebugDestination, ObjectProperty_TgGame_TgPawn_r_aDebugDestination);
			// DO_REP(ATgPawn, r_aDebugNextNav, ObjectProperty_TgGame_TgPawn_r_aDebugNextNav);
			// DO_REP(ATgPawn, r_aDebugTarget, ObjectProperty_TgGame_TgPawn_r_aDebugTarget);
			DO_REP(ATgPawn, r_bAimType, ByteProperty_TgGame_TgPawn_r_bAimType);
			DO_REP(ATgPawn, r_bAimingMode, BoolProperty_TgGame_TgPawn_r_bAimingMode);
			DO_REP(ATgPawn, r_bCallingForHelp, BoolProperty_TgGame_TgPawn_r_bCallingForHelp);
			DO_REP(ATgPawn, r_bIsAFK, BoolProperty_TgGame_TgPawn_r_bIsAFK);
			DO_REP(ATgPawn, r_bIsAnimInStrafeMode, BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode);
			DO_REP(ATgPawn, r_bIsCrafting, BoolProperty_TgGame_TgPawn_r_bIsCrafting);
			DO_REP(ATgPawn, r_bIsCrewing, BoolProperty_TgGame_TgPawn_r_bIsCrewing);
			DO_REP(ATgPawn, r_bIsDecoy, BoolProperty_TgGame_TgPawn_r_bIsDecoy);
			DO_REP(ATgPawn, r_bIsGrappleDismounting, BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting);
			DO_REP(ATgPawn, r_bIsHacked, BoolProperty_TgGame_TgPawn_r_bIsHacked);
			DO_REP(ATgPawn, r_bIsHacking, BoolProperty_TgGame_TgPawn_r_bIsHacking);
			DO_REP(ATgPawn, r_bIsHanging, BoolProperty_TgGame_TgPawn_r_bIsHanging);
			DO_REP(ATgPawn, r_bIsHangingDismounting, BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting);
			DO_REP(ATgPawn, r_bIsInSnipeScope, BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope);
			DO_REP(ATgPawn, r_bIsRappelling, BoolProperty_TgGame_TgPawn_r_bIsRappelling);
			DO_REP(ATgPawn, r_bIsStealthed, BoolProperty_TgGame_TgPawn_r_bIsStealthed);
			DO_REP(ATgPawn, r_bJumpedFromHanging, BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging);
			DO_REP(ATgPawn, r_bPostureIgnoreTransition, BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition);
			DO_REP(ATgPawn, r_bResistTagging, BoolProperty_TgGame_TgPawn_r_bResistTagging);
			DO_REP(ATgPawn, r_bShouldKnockDownAnimFaceDown, BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown);
			DO_REP(ATgPawn, r_bTagEnemy, BoolProperty_TgGame_TgPawn_r_bTagEnemy);
			DO_REP(ATgPawn, r_bUsingBinoculars, BoolProperty_TgGame_TgPawn_r_bUsingBinoculars);
			DO_REP(ATgPawn, r_eCurrentStunType, ByteProperty_TgGame_TgPawn_r_eCurrentStunType);
			DO_REP(ATgPawn, r_eDeathReason, ByteProperty_TgGame_TgPawn_r_eDeathReason);
			DO_REP(ATgPawn, r_eEmoteLength, IntProperty_TgGame_TgPawn_r_eEmoteLength);
			DO_REP(ATgPawn, r_eEmoteRepnotify, IntProperty_TgGame_TgPawn_r_eEmoteRepnotify);
			DO_REP(ATgPawn, r_eEmoteUpdate, IntProperty_TgGame_TgPawn_r_eEmoteUpdate);
			DO_REP(ATgPawn, r_ePosture, ByteProperty_TgGame_TgPawn_r_ePosture);
			DO_REP(ATgPawn, r_fDeployRate, FloatProperty_TgGame_TgPawn_r_fDeployRate);
			DO_REP(ATgPawn, r_fFrictionMultiplier, FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier);
			DO_REP(ATgPawn, r_fGravityZModifier, FloatProperty_TgGame_TgPawn_r_fGravityZModifier);
			DO_REP(ATgPawn, r_fKnockDownTimeRemaining, FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining);
			DO_REP(ATgPawn, r_fMakeVisibleFadeRate, FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate);
			DO_REP(ATgPawn, r_fPostureRateScale, FloatProperty_TgGame_TgPawn_r_fPostureRateScale);
			DO_REP(ATgPawn, r_fRappelGravityModifier, FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier);
			DO_REP(ATgPawn, r_fStealthTransitionTime, FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime);
			DO_REP(ATgPawn, r_fWeightBonus, IntProperty_TgGame_TgPawn_r_fWeightBonus);
			DO_REP(ATgPawn, r_iKnockDownFlash, IntProperty_TgGame_TgPawn_r_iKnockDownFlash);
			DO_REP(ATgPawn, r_nApplyStealth, IntProperty_TgGame_TgPawn_r_nApplyStealth);
			DO_REP(ATgPawn, r_nBotSoundCueId, IntProperty_TgGame_TgPawn_r_nBotSoundCueId);
			// DO_REP(ATgPawn, r_nDebugAggroRange, IntProperty_TgGame_TgPawn_r_nDebugAggroRange);
			// DO_REP(ATgPawn, r_nDebugFOV, IntProperty_TgGame_TgPawn_r_nDebugFOV);
			// DO_REP(ATgPawn, r_nDebugHearingRange, IntProperty_TgGame_TgPawn_r_nDebugHearingRange);
			// DO_REP(ATgPawn, r_nDebugSightRange, IntProperty_TgGame_TgPawn_r_nDebugSightRange);
			DO_REP(ATgPawn, r_nGenericAIEventIndex, IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex);
			DO_REP(ATgPawn, r_nHealthMaximum, IntProperty_TgGame_TgPawn_r_nHealthMaximum);
			DO_REP(ATgPawn, r_nNumberTimesCrewed, IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed);
			DO_REP(ATgPawn, r_nPhase, IntProperty_TgGame_TgPawn_r_nPhase);
			DO_REP(ATgPawn, r_nPitchOffset, IntProperty_TgGame_TgPawn_r_nPitchOffset);
			DO_REP(ATgPawn, r_nReplicateDying, IntProperty_TgGame_TgPawn_r_nReplicateDying);
			DO_REP(ATgPawn, r_nResetCharacter, IntProperty_TgGame_TgPawn_r_nResetCharacter);
			DO_REP(ATgPawn, r_nSensorAlertLevel, IntProperty_TgGame_TgPawn_r_nSensorAlertLevel);
			DO_REP(ATgPawn, r_nShieldHealthMax, IntProperty_TgGame_TgPawn_r_nShieldHealthMax);
			DO_REP(ATgPawn, r_nShieldHealthRemaining, IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining);
			DO_REP(ATgPawn, r_nSilentMode, IntProperty_TgGame_TgPawn_r_nSilentMode);
			DO_REP(ATgPawn, r_nStealthAggroRange, IntProperty_TgGame_TgPawn_r_nStealthAggroRange);
			DO_REP(ATgPawn, r_nStealthDisabled, IntProperty_TgGame_TgPawn_r_nStealthDisabled);
			DO_REP(ATgPawn, r_nStealthSensorRange, IntProperty_TgGame_TgPawn_r_nStealthSensorRange);
			DO_REP(ATgPawn, r_nStealthTypeCode, IntProperty_TgGame_TgPawn_r_nStealthTypeCode);
			DO_REP(ATgPawn, r_nYawOffset, IntProperty_TgGame_TgPawn_r_nYawOffset);
			// DO_REP(ATgPawn, r_sDebugAction, StrProperty_TgGame_TgPawn_r_sDebugAction);
			// DO_REP(ATgPawn, r_sDebugName, StrProperty_TgGame_TgPawn_r_sDebugName);
			DO_REP(ATgPawn, r_sFactory, StrProperty_TgGame_TgPawn_r_sFactory);
			DO_REP(ATgPawn, r_vDown, StructProperty_TgGame_TgPawn_r_vDown);
		}

		// DO_REP_ARRAY(25, ATgPawn, r_EquipDeviceInfo, StructProperty_TgGame_TgPawn_r_EquipDeviceInfo); // todo: investigate why it only works here
	}
	if (
		strcmp(classname, "Class TgGame.TgPawn_Ambush") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Tentacle") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Ambush, r_bIsDeployed, BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_AttackTransport") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_AttackTransport, r_DeathType, ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_CTR") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_CTR, r_CustomCharacterAssembly, StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly);
			DO_REP(ATgPawn_CTR, r_PilotPawn, ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn);
			DO_REP(ATgPawn_CTR, r_nMaxMorphIndexSentFromServer, IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer);
			DO_REP_ARRAY(0xFF, ATgPawn_CTR, r_nMorphSettings, IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgPawn_Character") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_AndroidMinion") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Boss_Destroyer") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Brawler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_CTR") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ColonyEye") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Dismantler") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_DuneCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Alchemist") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Elite_Assassin") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_FlyingBoss") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Guardian") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Hunter") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Inquisitor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Interact_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Juggernaut") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Marauder") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_NPC") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Raptor") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Reaper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_RecursiveSpawner") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Sniper") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SonoranCommander") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Switchblade") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_ThinkTank") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Turret") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVAFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVARocket") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlame") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretPlasma") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_UberWalker") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vanguard") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Vulcan") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_Warlord") == 0
	) {


		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_Character, r_CustomCharacterAssembly, StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly);
			DO_REP(ATgPawn_Character, r_eAttachedMesh, ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh);
			DO_REP(ATgPawn_Character, r_nBoostTimeRemaining, IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining);
			DO_REP(ATgPawn_Character, r_nHeadMeshAsmId, IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId);
			DO_REP(ATgPawn_Character, r_nItemProfileId, IntProperty_TgGame_TgPawn_Character_r_nItemProfileId);
			DO_REP(ATgPawn_Character, r_nItemProfileNbr, IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr);
			DO_REP(ATgPawn_Character, r_nMaxMorphIndexSentFromServer, IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer);
			DO_REP_ARRAY(0xFF, ATgPawn_Character, r_nMorphSettings, IntProperty_TgGame_TgPawn_Character_r_nMorphSettings);
		}
		if (((actor->Role == 3) && actor->bNetOwner) && actor->bNetInitial || actor->bNetDirty) {
			DO_REP(ATgPawn_Character, r_CurrentVanityPet, ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet);
			DO_REP(ATgPawn_Character, r_WallJumpUpperLineCheckOffset, FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset);
			DO_REP(ATgPawn_Character, r_WallJumpZ, FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ);
			DO_REP(ATgPawn_Character, r_bElfGogglesEquipped, BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped);
			DO_REP(ATgPawn_Character, r_nDeviceSlotUnlockGrpId, IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId);
			DO_REP(ATgPawn_Character, r_nSkillGroupSetId, IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_DuneCommander") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_DuneCommander, r_bDoCrashLanding, BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_Iris") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Iris, r_nStartNewScan, ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_Reaper") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Reaper, r_fBatteryPct, FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgPawn_Siege") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeBarrage") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeHover") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_SiegeRapidFire") == 0
	) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Siege, r_AccelDirection, ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgPawn_Turret") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVAFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretAVARocket") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlak") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretFlame") == 0
		|| strcmp(classname, "Class TgGame.TgPawn_TurretPlasma") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Turret, r_bIsDeployed, BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed);
			DO_REP(ATgPawn_Turret, r_fInitDeployTime, FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime);
			DO_REP(ATgPawn_Turret, r_fTimeToDeploySecs, FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgPawn_Turret, r_fCurrentDeployTime, FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime);
			DO_REP(ATgPawn_Turret, r_fDeployMaxHealthPCT, FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPawn_VanityPet") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_VanityPet, r_nSpawningItemId, IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgPlayerController") == 0) {
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgPlayerController, r_WatchOtherPlayer, ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer);
			// DO_REP(ATgPlayerController, r_bEDDebugEffects, BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects);
			DO_REP(ATgPlayerController, r_bGMInvisible, BoolProperty_TgGame_TgPlayerController_r_bGMInvisible);
			DO_REP(ATgPlayerController, r_bIsHackingABot, BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot);
			DO_REP(ATgPlayerController, r_bLockYawRotation, BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation);
			DO_REP(ATgPlayerController, r_bRove, BoolProperty_TgGame_TgPlayerController_r_bRove);
		}
	}
	if (strcmp(classname, "Class TgGame.TgProj_Grapple") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Grapple, r_vTargetLocation, StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation);
		}
	}
	if (strcmp(classname, "Class TgGame.TgProj_Missile") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Missile, r_aSeeking, ObjectProperty_TgGame_TgProj_Missile_r_aSeeking);
			DO_REP(ATgProj_Missile, r_vTargetWorldLocation, StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgProj_Missile, r_nNumBounces, IntProperty_TgGame_TgProj_Missile_r_nNumBounces);
		}
	}
	if (strcmp(classname, "Class TgGame.TgProj_Rocket") == 0) {
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgProj_Rocket, FlockIndex, ByteProperty_TgGame_TgProj_Rocket_FlockIndex);
			DO_REP(ATgProj_Rocket, bCurl, BoolProperty_TgGame_TgProj_Rocket_bCurl);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgProjectile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Teleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_StraightTeleporter") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Rocket") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Net") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Mortar") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Missile") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Grapple") == 0
		|| strcmp(classname, "Class TgGame.TgProj_FreeGrenade") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Deployable") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bounce") == 0
		|| strcmp(classname, "Class TgGame.TgProj_Bot") == 0
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgProjectile, r_Owner, ObjectProperty_TgGame_TgProjectile_r_Owner);
			DO_REP(ATgProjectile, r_fAccelRate, FloatProperty_TgGame_TgProjectile_r_fAccelRate);
			DO_REP(ATgProjectile, r_fDuration, FloatProperty_TgGame_TgProjectile_r_fDuration);
			DO_REP(ATgProjectile, r_fRange, FloatProperty_TgGame_TgProjectile_r_fRange);
			DO_REP(ATgProjectile, r_nOwnerFireModeId, IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId);
			DO_REP(ATgProjectile, r_nProjectileId, IntProperty_TgGame_TgProjectile_r_nProjectileId);
			DO_REP(ATgProjectile, r_vSpawnLocation, StructProperty_TgGame_TgProjectile_r_vSpawnLocation);
		}
	}
	if (strcmp(classname, "Class TgGame.TgRepInfo_Beacon") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_bDeployed, BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed);
			DO_REP(ATgRepInfo_Beacon, r_vLoc, StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_nName, StrProperty_TgGame_TgRepInfo_Beacon_r_nName);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgRepInfo_Deployable") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_Beacon") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Deployable, r_InstigatorInfo, ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo);
			DO_REP(ATgRepInfo_Deployable, r_TaskforceInfo, ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo);
			DO_REP(ATgRepInfo_Deployable, r_bOwnedByTaskforce, BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce);
			DO_REP(ATgRepInfo_Deployable, r_nHealthCurrent, IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgRepInfo_Deployable, r_DeployableOwner, ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner);
			DO_REP(ATgRepInfo_Deployable, r_fDeployMaxHealthPCT, FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT);
			DO_REP(ATgRepInfo_Deployable, r_nDeployableId, IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId);
			DO_REP(ATgRepInfo_Deployable, r_nHealthMaximum, IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum);
			DO_REP(ATgRepInfo_Deployable, r_nUniqueDeployableId, IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgRepInfo_Game") == 0
		|| strcmp(classname, "Class TgGame.TgRepInfo_GameOpenWorld") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgRepInfo_Game, r_MiniMapInfo, StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo);
			DO_REP(ATgRepInfo_Game, r_bActiveCombat, BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat);
			DO_REP(ATgRepInfo_Game, r_bAllowBuildMorale, BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale);
			DO_REP(ATgRepInfo_Game, r_bAllowPlayerRelease, BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease);
			DO_REP(ATgRepInfo_Game, r_bDefenseAlarm, BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm);
			DO_REP(ATgRepInfo_Game, r_bInOverTime, BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime);
			DO_REP(ATgRepInfo_Game, r_bIsTutorialMap, BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap);
			DO_REP(ATgRepInfo_Game, r_fGameSpeedModifier, FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier);
			DO_REP(ATgRepInfo_Game, r_fMissionRemainingTime, FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime);
			DO_REP(ATgRepInfo_Game, r_fServerTimeLastUpdate, FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate);
			DO_REP(ATgRepInfo_Game, r_nMaxRoundNumber, IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber);
			DO_REP(ATgRepInfo_Game, r_nMissionTimerState, ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState);
			DO_REP(ATgRepInfo_Game, r_nMissionTimerStateChange, IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange);
			DO_REP(ATgRepInfo_Game, r_nRaidAttackerRespawnBonus, IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus);
			DO_REP(ATgRepInfo_Game, r_nRaidDefenderRespawnBonus, IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus);
			DO_REP(ATgRepInfo_Game, r_nReleaseDelay, IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay);
			DO_REP(ATgRepInfo_Game, r_nRoundNumber, IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber);
			DO_REP(ATgRepInfo_Game, r_nSecsToAutoReleaseAttackers, IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers);
			DO_REP(ATgRepInfo_Game, r_nSecsToAutoReleaseDefenders, IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders);

			DO_REP_ARRAY(0x4B, ATgRepInfo_Game, r_Objectives, ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives); // moved from bNetInitial
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgRepInfo_Game, r_GameType, ByteProperty_TgGame_TgRepInfo_Game_r_GameType);
			DO_REP(ATgRepInfo_Game, r_MapLogoResIds, StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds);
			DO_REP(ATgRepInfo_Game, r_bIsArena, BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena);
			DO_REP(ATgRepInfo_Game, r_bIsMatch, BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch);
			DO_REP(ATgRepInfo_Game, r_bIsMission, BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission);
			DO_REP(ATgRepInfo_Game, r_bIsPVP, BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP);
			DO_REP(ATgRepInfo_Game, r_bIsRaid, BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid);
			DO_REP(ATgRepInfo_Game, r_bIsTerritoryMap, BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap);
			DO_REP(ATgRepInfo_Game, r_bIsTraining, BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining);
			DO_REP(ATgRepInfo_Game, r_nAutoKickTimeout, IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout);
			DO_REP(ATgRepInfo_Game, r_nPointsToWin, IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin);
			DO_REP(ATgRepInfo_Game, r_nVictoryBonusLives, IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives);

			DO_REP(ATgRepInfo_Game, r_nMissionTimerState, ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState);
		}
	}
	if (strcmp(classname, "Class TgGame.TgRepInfo_GameOpenWorld") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x3, ATgRepInfo_GameOpenWorld, r_GameTickets, IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets);
		}
	}
	if (strcmp(classname, "Class TgGame.TgRepInfo_Player") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Player, r_ApproxLocation, StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation);
			DO_REP(ATgRepInfo_Player, r_CustomCharacterAssembly, StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly);
			DO_REP_ARRAY(25, ATgRepInfo_Player, r_EquipDeviceInfo, StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo);
			DO_REP(ATgRepInfo_Player, r_MasterPrep, ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep);
			DO_REP(ATgRepInfo_Player, r_PawnOwner, ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner);
			DO_REP_ARRAY(0xB, ATgRepInfo_Player, r_Scores, IntProperty_TgGame_TgRepInfo_Player_r_Scores);
			DO_REP(ATgRepInfo_Player, r_TaskForce, ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce);
			DO_REP(ATgRepInfo_Player, r_bDropped, BoolProperty_TgGame_TgRepInfo_Player_r_bDropped);
			DO_REP(ATgRepInfo_Player, r_eBonusType, IntProperty_TgGame_TgRepInfo_Player_r_eBonusType);
			DO_REP(ATgRepInfo_Player, r_nCharacterId, IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId);
			DO_REP(ATgRepInfo_Player, r_nHealthCurrent, IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent);
			DO_REP(ATgRepInfo_Player, r_nHealthMaximum, IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum);
			DO_REP(ATgRepInfo_Player, r_nLevel, IntProperty_TgGame_TgRepInfo_Player_r_nLevel);
			DO_REP(ATgRepInfo_Player, r_nProfileId, IntProperty_TgGame_TgRepInfo_Player_r_nProfileId);
			DO_REP(ATgRepInfo_Player, r_nTitleMsgId, IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId);
			DO_REP(ATgRepInfo_Player, r_sAgencyName, StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName);
			DO_REP(ATgRepInfo_Player, r_sAllianceName, StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName);
			DO_REP(ATgRepInfo_Player, r_sOrigPlayerName, StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName);
		}
		if (actor->bNetOwner && actor->Role == 3) {
			DO_REP_ARRAY(0x9, ATgRepInfo_Player, r_DeviceStats, StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats);
		}
	}
	if (
		strcmp(classname, "Class TgGame.TgRepInfo_TaskForce") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ATgRepInfo_TaskForce, r_BeaconManager, ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager);
			DO_REP(ATgRepInfo_TaskForce, r_CurrActiveObjective, ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective);
			DO_REP(ATgRepInfo_TaskForce, r_ObjectiveAssignment, ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment);
			DO_REP(ATgRepInfo_TaskForce, r_bBotOwned, BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned);
			DO_REP(ATgRepInfo_TaskForce, r_eCoalition, ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition);
			DO_REP(ATgRepInfo_TaskForce, r_nCurrentPointCount, IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount);
			DO_REP(ATgRepInfo_TaskForce, r_nLeaderCharId, IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId);
			DO_REP(ATgRepInfo_TaskForce, r_nLookingForMembers, FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers);
			DO_REP(ATgRepInfo_TaskForce, r_nNumDeaths, IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths);

		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgRepInfo_TaskForce, r_nTaskForce, ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce);
			DO_REP(ATgRepInfo_TaskForce, r_nTeamId, IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId);
		}
	}
	if (strcmp(classname, "Class TgGame.TgSkydiveTarget") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydiveTarget, m_LandRadius, FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius);
		}
	}
	if (strcmp(classname, "Class TgGame.TgSkydivingVolume") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydivingVolume, r_PawnGravityModifier, FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier);
			DO_REP(ATgSkydivingVolume, r_PawnLaunchForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce);
			DO_REP(ATgSkydivingVolume, r_PawnUpForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce);
			DO_REP(ATgSkydivingVolume, r_SkydiveTarget, ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget);
		}
	}
	if (strcmp(classname, "Class TgGame.TgTeamBeaconManager") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgTeamBeaconManager, r_Beacon, ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon);
			DO_REP(ATgTeamBeaconManager, r_BeaconDestroyed, IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed);
			DO_REP(ATgTeamBeaconManager, r_BeaconHolder, ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder);
			DO_REP(ATgTeamBeaconManager, r_BeaconInfo, ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo);
			DO_REP(ATgTeamBeaconManager, r_BeaconStatus, ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus);

			DO_REP(ATgTeamBeaconManager, r_TaskForce, ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
		}
	}
	if (strcmp(classname, "Class TgGame.TgTimerManager") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgTimerManager, r_byEventQue, ByteProperty_TgGame_TgTimerManager_r_byEventQue);
			DO_REP(ATgTimerManager, r_byEventQueIndex, ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fRemaining, FloatProperty_TgGame_TgTimerManager_r_fRemaining);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fStartTime, FloatProperty_TgGame_TgTimerManager_r_fStartTime);
		}
	}

	return param_3;
}

