#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/Utils/Macros.hpp"
#include "src/Utils/Logger/Logger.hpp"

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


int* __fastcall Actor__GetOptimizedRepList::Call(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5) {
	param_3 = CallOriginal(thisxx, edx_dummy, param_1, param_2, param_3, param_4, param_5);
	if (!bRepListCached) {
		bRepListCached = true;
		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (obj) {
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Actor.Base") == 0) {
					ObjectProperty_Engine_Actor_Base = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Actor.Physics") == 0) {
					ByteProperty_Engine_Actor_Physics = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Actor.Instigator") == 0) {
					ObjectProperty_Engine_Actor_Instigator = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Controller.Pawn") == 0) {
					ObjectProperty_Engine_Controller_Pawn = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.EmitterSpawnable.ParticleTemplate") == 0) {
					ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Inventory.Inventory") == 0) {
					ObjectProperty_Engine_Inventory_Inventory = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "StructProperty Engine.Pawn.FlashLocation") == 0) {
					StructProperty_Engine_Pawn_FlashLocation = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "IntProperty Engine.Pawn.Health") == 0) {
					IntProperty_Engine_Pawn_Health = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Pawn.Controller") == 0) {
					ObjectProperty_Engine_Pawn_Controller = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Pawn.FiringMode") == 0) {
					ByteProperty_Engine_Pawn_FiringMode = (UProperty*)obj;
					continue;
				}
				if (strcmp(obj->GetFullName(), "ByteProperty Engine.Pawn.FlashCount") == 0) {
					ByteProperty_Engine_Pawn_FlashCount = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty Engine.Vehicle.Driver") == 0) {
					ObjectProperty_Engine_Vehicle_Driver = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "BoolProperty TgGame.TgPawn.r_bIsHanging") == 0) {
					BoolProperty_TgGame_TgPawn_r_bIsHanging = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgRepInfo_Game.r_nMissionTimerState") == 0) {
					ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ObjectProperty TgGame.TgTeamBeaconManager.r_Beacon") == 0) {
					ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon = (UProperty*)obj;
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
				if (strcmp(obj->GetFullName(), "ByteProperty TgGame.TgTimerManager.r_byEventQue") == 0) {
					ByteProperty_TgGame_TgTimerManager_r_byEventQue = (UProperty*)obj;
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

	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.Actor") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgRandomSMActor") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0
	) {
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.AmbientSoundSimpleToggleable") == 0) {
		if (actor->Role == 3) {
			DO_REP(AAmbientSoundSimpleToggleable, bCurrentlyPlaying, BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.CameraActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ACameraActor, AspectRatio, FloatProperty_Engine_CameraActor_AspectRatio);
			DO_REP(ACameraActor, FOVAngle, FloatProperty_Engine_CameraActor_FOVAngle);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.Controller") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgPlayerController") == 0
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AController, Pawn, ObjectProperty_Engine_Controller_Pawn);
			DO_REP(AController, PlayerReplicationInfo, ObjectProperty_Engine_Controller_PlayerReplicationInfo);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.CrowdAttractor") == 0) {
		if (actor->bNoDelete) {
			DO_REP(ACrowdAttractor, bAttractorEnabled, BoolProperty_Engine_CrowdAttractor_bAttractorEnabled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.CrowdReplicationActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ACrowdReplicationActor, DestroyAllCount, IntProperty_Engine_CrowdReplicationActor_DestroyAllCount);
			DO_REP(ACrowdReplicationActor, Spawner, ObjectProperty_Engine_CrowdReplicationActor_Spawner);
			DO_REP(ACrowdReplicationActor, bSpawningActive, BoolProperty_Engine_CrowdReplicationActor_bSpawningActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.DroppedPickup") == 0) {
		if (actor->Role == 3) {
			DO_REP(ADroppedPickup, InventoryClass, ClassProperty_Engine_DroppedPickup_InventoryClass);
			DO_REP(ADroppedPickup, bFadeOut, BoolProperty_Engine_DroppedPickup_bFadeOut);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.DynamicSMActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ADynamicSMActor, ReplicatedMaterial, ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial);
			DO_REP(ADynamicSMActor, ReplicatedMesh, ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh);
			DO_REP(ADynamicSMActor, ReplicatedMeshRotation, StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation);
			DO_REP(ADynamicSMActor, ReplicatedMeshScale3D, StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D);
			DO_REP(ADynamicSMActor, ReplicatedMeshTranslation, StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation);
			DO_REP(ADynamicSMActor, bForceStaticDecals, BoolProperty_Engine_DynamicSMActor_bForceStaticDecals);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Emitter") == 0) {
		if (actor->bNoDelete) {
			DO_REP(AEmitter, bCurrentlyActive, BoolProperty_Engine_Emitter_bCurrentlyActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.EmitterSpawnable") == 0) {
		if (actor->bNetInitial) {
			DO_REP(AEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.FluidInfluenceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(AFluidInfluenceActor, bActive, BoolProperty_Engine_FluidInfluenceActor_bActive);
			DO_REP(AFluidInfluenceActor, bToggled, BoolProperty_Engine_FluidInfluenceActor_bToggled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.FogVolumeDensityInfo") == 0) {
		if (actor->Role == 3) {
			DO_REP(AFogVolumeDensityInfo, bEnabled, BoolProperty_Engine_FogVolumeDensityInfo_bEnabled);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.GameReplicationInfo") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Game") == 0
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.HeightFog") == 0) {
		if (actor->Role == 3) {
			DO_REP(AHeightFog, bEnabled, BoolProperty_Engine_HeightFog_bEnabled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Inventory") == 0) {
		if (((actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventory, InvManager, ObjectProperty_Engine_Inventory_InvManager);
			DO_REP(AInventory, Inventory, ObjectProperty_Engine_Inventory_Inventory);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.InventoryManager") == 0) {
		if ((((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventoryManager, InventoryChain, ObjectProperty_Engine_InventoryManager_InventoryChain);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.KActor") == 0) {
		if (!((AKActor*)actor)->bNeedsRBStateReplication && actor->Role == 3) {
			DO_REP(AKActor, RBState, StructProperty_Engine_KActor_RBState);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(AKActor, ReplicatedDrawScale3D, StructProperty_Engine_KActor_ReplicatedDrawScale3D);
			DO_REP(AKActor, bWakeOnLevelStart, BoolProperty_Engine_KActor_bWakeOnLevelStart);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.KAsset") == 0) {
		if (actor->Role == 3) {
			DO_REP(AKAsset, ReplicatedMesh, ObjectProperty_Engine_KAsset_ReplicatedMesh);
			DO_REP(AKAsset, ReplicatedPhysAsset, ObjectProperty_Engine_KAsset_ReplicatedPhysAsset);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.LensFlareSource") == 0) {
		if (actor->bNoDelete) {
			DO_REP(ALensFlareSource, bCurrentlyActive, BoolProperty_Engine_LensFlareSource_bCurrentlyActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Light") == 0) {
		if (actor->Role == 3) {
			DO_REP(ALight, bEnabled, BoolProperty_Engine_Light_bEnabled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.MatineeActor") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.NxForceField") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ANxForceField, bForceActive, BoolProperty_Engine_NxForceField_bForceActive);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.Pawn") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Character") == 0
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.PhysXEmitterSpawnable") == 0) {
		if (actor->bNetInitial) {
			DO_REP(APhysXEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.PickupFactory") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(APickupFactory, bPickupHidden, BoolProperty_Engine_PickupFactory_bPickupHidden);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(APickupFactory, InventoryType, ClassProperty_Engine_PickupFactory_InventoryType);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.PlayerController") == 0) {
		if (((actor->bNetOwner && actor->Role == 3) && ((APlayerController*)actor)->ViewTarget != ((APlayerController*)actor)->Pawn) && ((APlayerController*)actor)->ViewTarget != NULL) {
			DO_REP(APlayerController, TargetEyeHeight, FloatProperty_Engine_PlayerController_TargetEyeHeight);
			DO_REP(APlayerController, TargetViewRotation, StructProperty_Engine_PlayerController_TargetViewRotation);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.PlayerReplicationInfo") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Player") == 0
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.PostProcessVolume") == 0) {
		if (actor->bNetDirty) {
			DO_REP(APostProcessVolume, bEnabled, BoolProperty_Engine_PostProcessVolume_bEnabled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Projectile") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(AProjectile, MaxSpeed, FloatProperty_Engine_Projectile_MaxSpeed);
			DO_REP(AProjectile, Speed, FloatProperty_Engine_Projectile_Speed);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.RB_CylindricalForceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_CylindricalForceActor, bForceActive, BoolProperty_Engine_RB_CylindricalForceActor_bForceActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.RB_LineImpulseActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_LineImpulseActor, ImpulseCount, ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.RB_RadialForceActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialForceActor, bForceActive, BoolProperty_Engine_RB_RadialForceActor_bForceActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.RB_RadialImpulseActor") == 0) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialImpulseActor, ImpulseCount, ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.SVehicle") == 0) {
		if (actor->Physics == 10) {
			DO_REP(ASVehicle, MaxSpeed, FloatProperty_Engine_SVehicle_MaxSpeed);
			DO_REP(ASVehicle, VState, StructProperty_Engine_SVehicle_VState);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class Engine.SkeletalMeshActor") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActorGenericUIPreview") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActorNPC") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActorNPCVendor") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActorSpawnable") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor_CharacterBuilder") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor_CharacterBuilderSpawnable") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor_Composite") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor_EquipScreen") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor_MeleePreVis") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkeletalMeshActor") == 0
	) {
		if (actor->Role == 3) {
			DO_REP(ASkeletalMeshActor, ReplicatedMaterial, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial);
			DO_REP(ASkeletalMeshActor, ReplicatedMesh, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.TeamInfo") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATeamInfo, Score, FloatProperty_Engine_TeamInfo_Score);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATeamInfo, TeamIndex, IntProperty_Engine_TeamInfo_TeamIndex);
			DO_REP(ATeamInfo, TeamName, StrProperty_Engine_TeamInfo_TeamName);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Teleporter") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class Engine.Vehicle") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AVehicle, bDriving, BoolProperty_Engine_Vehicle_bDriving);
		}
		if ((actor->bNetDirty && (actor->bNetOwner || ((AVehicle*)actor)->Driver == NULL) || !((AVehicle*)actor)->Driver->bHidden) && actor->Role == 3) {
			DO_REP(AVehicle, Driver, ObjectProperty_Engine_Vehicle_Driver);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class Engine.WorldInfo") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AWorldInfo, Pauser, ObjectProperty_Engine_WorldInfo_Pauser);
			DO_REP(AWorldInfo, ReplicatedMusicTrack, StructProperty_Engine_WorldInfo_ReplicatedMusicTrack);
			DO_REP(AWorldInfo, TimeDilation, FloatProperty_Engine_WorldInfo_TimeDilation);
			DO_REP(AWorldInfo, WorldGravityZ, FloatProperty_Engine_WorldInfo_WorldGravityZ);
			DO_REP(AWorldInfo, bHighPriorityLoading, BoolProperty_Engine_WorldInfo_bHighPriorityLoading);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgChestActor") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgChestActor, r_eChestState, ByteProperty_TgGame_TgChestActor_r_eChestState);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeploy_BeaconEntrance") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_BeaconEntrance, r_bActive, BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeploy_DestructibleCover") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_DestructibleCover, r_bHasFired, BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeploy_Sensor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_Sensor, r_nSensorAudioWarning, IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning);
			DO_REP(ATgDeploy_Sensor, r_nTouchedPlayerCount, IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeployable") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeploy_Beacon") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgDeploy_BeaconEntrance") == 0
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDevice") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDevice_Morale") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDevice_Morale, r_bIsActivelyFiring, BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDoor") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDoor, r_bOpen, BoolProperty_TgGame_TgDoor_r_bOpen);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDoorMarker") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgDoorMarker, r_eStatus, ByteProperty_TgGame_TgDoorMarker_r_eStatus);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDroppedItem") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDroppedItem, r_nItemId, IntProperty_TgGame_TgDroppedItem_r_nItemId);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDynamicDestructible") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicDestructible, r_nDestructibleId, IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId);
			DO_REP(ATgDynamicDestructible, r_pFactory, ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgDynamicSMActor") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicSMActor, m_sAssembly, StrProperty_TgGame_TgDynamicSMActor_m_sAssembly);
			DO_REP(ATgDynamicSMActor, r_EffectManager, ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager);
		}
		if (actor->Role == 3) {
			DO_REP(ATgDynamicSMActor, r_nHealth, IntProperty_TgGame_TgDynamicSMActor_r_nHealth);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgEffectManager") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgEffectManager, r_EventQueue, StructProperty_TgGame_TgEffectManager_r_EventQueue);
			DO_REP_ARRAY(0x10, ATgEffectManager, r_ManagedEffectList, StructProperty_TgGame_TgEffectManager_r_ManagedEffectList);
			DO_REP(ATgEffectManager, r_Owner, ObjectProperty_TgGame_TgEffectManager_r_Owner);
			DO_REP(ATgEffectManager, r_bRelevancyNotify, BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify);
			DO_REP(ATgEffectManager, r_nInvulnerableCount, IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount);
			DO_REP(ATgEffectManager, r_nNextQueueIndex, IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgEmitter") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgEmitter, BoneName, NameProperty_TgGame_TgEmitter_BoneName);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgFlagCaptureVolume") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgFlagCaptureVolume, r_eCoalition, ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition);
			DO_REP(ATgFlagCaptureVolume, r_nTaskForce, ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgFracturedStaticMeshActor") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgHexLandMarkActor") == 0) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgHexLandMarkActor, r_nMeshAsmId, IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgInterpActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgInterpActor, r_sCurrState, StrProperty_TgGame_TgInterpActor_r_sCurrState);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgInventoryManager") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgInventoryManager, r_ItemCount, IntProperty_TgGame_TgInventoryManager_r_ItemCount);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgKismetTestActor") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgKismetTestActor, r_nCurrentTest, IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest);
			DO_REP(ATgKismetTestActor, r_nFailCount, IntProperty_TgGame_TgKismetTestActor_r_nFailCount);
			DO_REP(ATgKismetTestActor, r_nPassCount, IntProperty_TgGame_TgKismetTestActor_r_nPassCount);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgLevelCamera") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgLevelCamera, r_bEnabled, BoolProperty_TgGame_TgLevelCamera_r_bEnabled);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgMissionObjective") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgMissionObjective_Bot") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBot, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot);
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBotInfo, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgMissionObjective_Escort") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Escort, r_AttachedActor, ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgMissionObjective_Proximity") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Proximity, r_fCaptureRate, FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgObjectiveAssignment") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgObjectiveAssignment, r_AssignedObjective, ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective);
			DO_REP(ATgObjectiveAssignment, r_Attackers, ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers);
			DO_REP(ATgObjectiveAssignment, r_Bots, ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots);
			DO_REP(ATgObjectiveAssignment, r_Defenders, ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders);
			DO_REP(ATgObjectiveAssignment, r_eState, ByteProperty_TgGame_TgObjectiveAssignment_r_eState);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Character") == 0
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
		}
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgPawn, r_ControlPawn, ObjectProperty_TgGame_TgPawn_r_ControlPawn);
			DO_REP(ATgPawn, r_CurrentOmegaVolume, ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume);
			DO_REP(ATgPawn, r_CurrentSubzoneBilboardVol, ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol);
			DO_REP(ATgPawn, r_CurrentSubzoneVol, ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol);
			DO_REP_ARRAY(0x2, ATgPawn, r_ScannerSettings, StructProperty_TgGame_TgPawn_r_ScannerSettings);
			DO_REP_ARRAY(25, ATgPawn, r_EquipDeviceInfo, StructProperty_TgGame_TgPawn_r_EquipDeviceInfo);
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
			// DO_REP(ATgPawn, r_bCallingForHelp, BoolProperty_TgGame_TgPawn_r_bCallingForHelp);
			// DO_REP(ATgPawn, r_bIsAFK, BoolProperty_TgGame_TgPawn_r_bIsAFK);
			DO_REP(ATgPawn, r_bIsAnimInStrafeMode, BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode);
			DO_REP(ATgPawn, r_bIsCrafting, BoolProperty_TgGame_TgPawn_r_bIsCrafting);
			// DO_REP(ATgPawn, r_bIsCrewing, BoolProperty_TgGame_TgPawn_r_bIsCrewing);
			DO_REP(ATgPawn, r_bIsDecoy, BoolProperty_TgGame_TgPawn_r_bIsDecoy);
			DO_REP(ATgPawn, r_bIsGrappleDismounting, BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting);
			// DO_REP(ATgPawn, r_bIsHacked, BoolProperty_TgGame_TgPawn_r_bIsHacked);
			// DO_REP(ATgPawn, r_bIsHacking, BoolProperty_TgGame_TgPawn_r_bIsHacking);
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
			// DO_REP(ATgPawn, r_nSensorAlertLevel, IntProperty_TgGame_TgPawn_r_nSensorAlertLevel);
			// DO_REP(ATgPawn, r_nShieldHealthMax, IntProperty_TgGame_TgPawn_r_nShieldHealthMax);
			// DO_REP(ATgPawn, r_nShieldHealthRemaining, IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining);
			// DO_REP(ATgPawn, r_nSilentMode, IntProperty_TgGame_TgPawn_r_nSilentMode);
			// DO_REP(ATgPawn, r_nStealthAggroRange, IntProperty_TgGame_TgPawn_r_nStealthAggroRange);
			// DO_REP(ATgPawn, r_nStealthDisabled, IntProperty_TgGame_TgPawn_r_nStealthDisabled);
			// DO_REP(ATgPawn, r_nStealthSensorRange, IntProperty_TgGame_TgPawn_r_nStealthSensorRange);
			// DO_REP(ATgPawn, r_nStealthTypeCode, IntProperty_TgGame_TgPawn_r_nStealthTypeCode);
			DO_REP(ATgPawn, r_nYawOffset, IntProperty_TgGame_TgPawn_r_nYawOffset);
			// DO_REP(ATgPawn, r_sDebugAction, StrProperty_TgGame_TgPawn_r_sDebugAction);
			// DO_REP(ATgPawn, r_sDebugName, StrProperty_TgGame_TgPawn_r_sDebugName);
			// DO_REP(ATgPawn, r_sFactory, StrProperty_TgGame_TgPawn_r_sFactory);
			DO_REP(ATgPawn, r_vDown, StructProperty_TgGame_TgPawn_r_vDown);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Ambush") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Ambush, r_bIsDeployed, BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_AttackTransport") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_AttackTransport, r_DeathType, ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_CTR") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_CTR, r_CustomCharacterAssembly, StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly);
			DO_REP(ATgPawn_CTR, r_PilotPawn, ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn);
			DO_REP(ATgPawn_CTR, r_nMaxMorphIndexSentFromServer, IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer);
			DO_REP_ARRAY(0xFF, ATgPawn_CTR, r_nMorphSettings, IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Character") == 0) {


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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_DuneCommander") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_DuneCommander, r_bDoCrashLanding, BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Iris") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Iris, r_nStartNewScan, ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Reaper") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Reaper, r_fBatteryPct, FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Siege") == 0) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Siege, r_AccelDirection, ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_Turret") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPawn_VanityPet") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_VanityPet, r_nSpawningItemId, IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgPlayerController") == 0) {
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgPlayerController, r_WatchOtherPlayer, ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer);
			// DO_REP(ATgPlayerController, r_bEDDebugEffects, BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects);
			DO_REP(ATgPlayerController, r_bGMInvisible, BoolProperty_TgGame_TgPlayerController_r_bGMInvisible);
			DO_REP(ATgPlayerController, r_bIsHackingABot, BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot);
			DO_REP(ATgPlayerController, r_bLockYawRotation, BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation);
			DO_REP(ATgPlayerController, r_bRove, BoolProperty_TgGame_TgPlayerController_r_bRove);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgProj_Grapple") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Grapple, r_vTargetLocation, StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgProj_Missile") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Missile, r_aSeeking, ObjectProperty_TgGame_TgProj_Missile_r_aSeeking);
			DO_REP(ATgProj_Missile, r_vTargetWorldLocation, StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgProj_Missile, r_nNumBounces, IntProperty_TgGame_TgProj_Missile_r_nNumBounces);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgProj_Rocket") == 0) {
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgProj_Rocket, FlockIndex, ByteProperty_TgGame_TgProj_Rocket_FlockIndex);
			DO_REP(ATgProj_Rocket, bCurl, BoolProperty_TgGame_TgProj_Rocket_bCurl);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgProjectile") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Beacon") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_bDeployed, BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed);
			DO_REP(ATgRepInfo_Beacon, r_vLoc, StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_nName, StrProperty_TgGame_TgRepInfo_Beacon_r_nName);
		}
	}
	if (
		strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Deployable") == 0
		|| strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Beacon") == 0
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Game") == 0) {
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
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgRepInfo_Game, r_GameType, ByteProperty_TgGame_TgRepInfo_Game_r_GameType);
			DO_REP(ATgRepInfo_Game, r_MapLogoResIds, StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds);
			DO_REP_ARRAY(0x4B, ATgRepInfo_Game, r_Objectives, ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives);
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
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_GameOpenWorld") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x3, ATgRepInfo_GameOpenWorld, r_GameTickets, IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_Player") == 0) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Player, r_ApproxLocation, StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation);
			DO_REP(ATgRepInfo_Player, r_CustomCharacterAssembly, StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly);
			DO_REP_ARRAY(25, ATgRepInfo_Player, r_EquipDeviceInfo, StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo);
			DO_REP(ATgRepInfo_Player, r_MasterPrep, ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep);
			DO_REP(ATgRepInfo_Player, r_PawnOwner, ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner);
			// DO_REP_ARRAY(0xB, ATgRepInfo_Player, r_Scores, IntProperty_TgGame_TgRepInfo_Player_r_Scores);
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgRepInfo_TaskForce") == 0) {
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
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkydiveTarget") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydiveTarget, m_LandRadius, FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgSkydivingVolume") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydivingVolume, r_PawnGravityModifier, FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier);
			DO_REP(ATgSkydivingVolume, r_PawnLaunchForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce);
			DO_REP(ATgSkydivingVolume, r_PawnUpForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce);
			DO_REP(ATgSkydivingVolume, r_SkydiveTarget, ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgTeamBeaconManager") == 0) {
		if (actor->Role == 3) {
			DO_REP(ATgTeamBeaconManager, r_Beacon, ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon);
			DO_REP(ATgTeamBeaconManager, r_BeaconDestroyed, IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed);
			DO_REP(ATgTeamBeaconManager, r_BeaconHolder, ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder);
			DO_REP(ATgTeamBeaconManager, r_BeaconInfo, ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo);
			DO_REP(ATgTeamBeaconManager, r_BeaconStatus, ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgTeamBeaconManager, r_TaskForce, ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce);
		}
	}
	if (strcmp(actor->Class->GetFullName(), "Class TgGame.TgTimerManager") == 0) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgTimerManager, r_byEventQue, ByteProperty_TgGame_TgTimerManager_r_byEventQue);
			DO_REP(ATgTimerManager, r_byEventQueIndex, ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fRemaining, FloatProperty_TgGame_TgTimerManager_r_fRemaining);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fStartTime, FloatProperty_TgGame_TgTimerManager_r_fStartTime);
		}
	}

	return param_3;
}

