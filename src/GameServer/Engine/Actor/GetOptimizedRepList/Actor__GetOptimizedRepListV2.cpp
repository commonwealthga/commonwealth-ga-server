#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/Utils/Macros.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

bool Actor__GetOptimizedRepList::bRepListCached = false;

// Per-class UClass* pointers used for O(1) class-identity dispatch in Call().
// Resolved once at first invocation by ResolveRepListProperties().
// Replaces the per-call strcmp(actor->Class->GetFullName(), "Class X") cascade.
static UClass* Class_Engine_AIController = nullptr;
static UClass* Class_Engine_AccessControl = nullptr;
static UClass* Class_Engine_Actor = nullptr;
static UClass* Class_Engine_Admin = nullptr;
static UClass* Class_Engine_AmbientSound = nullptr;
static UClass* Class_Engine_AmbientSoundMovable = nullptr;
static UClass* Class_Engine_AmbientSoundNonLoop = nullptr;
static UClass* Class_Engine_AmbientSoundSimple = nullptr;
static UClass* Class_Engine_AmbientSoundSimpleToggleable = nullptr;
static UClass* Class_Engine_AnimatedCamera = nullptr;
static UClass* Class_Engine_AutoLadder = nullptr;
static UClass* Class_Engine_BlockingVolume = nullptr;
static UClass* Class_Engine_BroadcastHandler = nullptr;
static UClass* Class_Engine_Brush = nullptr;
static UClass* Class_Engine_Camera = nullptr;
static UClass* Class_Engine_CameraActor = nullptr;
static UClass* Class_Engine_ClipMarker = nullptr;
static UClass* Class_Engine_ColorScaleVolume = nullptr;
static UClass* Class_Engine_Controller = nullptr;
static UClass* Class_Engine_CoverGroup = nullptr;
static UClass* Class_Engine_CoverLink = nullptr;
static UClass* Class_Engine_CoverReplicator = nullptr;
static UClass* Class_Engine_CoverSlotMarker = nullptr;
static UClass* Class_Engine_CrowdAgent = nullptr;
static UClass* Class_Engine_CrowdAttractor = nullptr;
static UClass* Class_Engine_CrowdReplicationActor = nullptr;
static UClass* Class_Engine_CullDistanceVolume = nullptr;
static UClass* Class_Engine_DebugCameraController = nullptr;
static UClass* Class_Engine_DebugCameraHUD = nullptr;
static UClass* Class_Engine_DecalActor = nullptr;
static UClass* Class_Engine_DecalActorBase = nullptr;
static UClass* Class_Engine_DecalActorMovable = nullptr;
static UClass* Class_Engine_DecalManager = nullptr;
static UClass* Class_Engine_DefaultPhysicsVolume = nullptr;
static UClass* Class_Engine_DirectionalLight = nullptr;
static UClass* Class_Engine_DirectionalLightToggleable = nullptr;
static UClass* Class_Engine_DoorMarker = nullptr;
static UClass* Class_Engine_DroppedPickup = nullptr;
static UClass* Class_Engine_DynamicAnchor = nullptr;
static UClass* Class_Engine_DynamicBlockingVolume = nullptr;
static UClass* Class_Engine_DynamicCameraActor = nullptr;
static UClass* Class_Engine_DynamicPhysicsVolume = nullptr;
static UClass* Class_Engine_DynamicSMActor = nullptr;
static UClass* Class_Engine_DynamicSMActor_Spawnable = nullptr;
static UClass* Class_Engine_DynamicTriggerVolume = nullptr;
static UClass* Class_Engine_Emitter = nullptr;
static UClass* Class_Engine_EmitterCameraLensEffectBase = nullptr;
static UClass* Class_Engine_EmitterPool = nullptr;
static UClass* Class_Engine_EmitterSpawnable = nullptr;
static UClass* Class_Engine_FileLog = nullptr;
static UClass* Class_Engine_FileWriter = nullptr;
static UClass* Class_Engine_FluidInfluenceActor = nullptr;
static UClass* Class_Engine_FluidSurfaceActor = nullptr;
static UClass* Class_Engine_FluidSurfaceActorMovable = nullptr;
static UClass* Class_Engine_FogVolumeConeDensityInfo = nullptr;
static UClass* Class_Engine_FogVolumeConstantDensityInfo = nullptr;
static UClass* Class_Engine_FogVolumeDensityInfo = nullptr;
static UClass* Class_Engine_FogVolumeLinearHalfspaceDensityInfo = nullptr;
static UClass* Class_Engine_FogVolumeSphericalDensityInfo = nullptr;
static UClass* Class_Engine_FoliageFactory = nullptr;
static UClass* Class_Engine_FractureManager = nullptr;
static UClass* Class_Engine_FracturedStaticMeshActor = nullptr;
static UClass* Class_Engine_FracturedStaticMeshPart = nullptr;
static UClass* Class_Engine_GameInfo = nullptr;
static UClass* Class_Engine_GameReplicationInfo = nullptr;
static UClass* Class_Engine_GravityVolume = nullptr;
static UClass* Class_Engine_HUD = nullptr;
static UClass* Class_Engine_HeightFog = nullptr;
static UClass* Class_Engine_Info = nullptr;
static UClass* Class_Engine_InternetInfo = nullptr;
static UClass* Class_Engine_InterpActor = nullptr;
static UClass* Class_Engine_Inventory = nullptr;
static UClass* Class_Engine_InventoryManager = nullptr;
static UClass* Class_Engine_KActor = nullptr;
static UClass* Class_Engine_KActorSpawnable = nullptr;
static UClass* Class_Engine_KAsset = nullptr;
static UClass* Class_Engine_KAssetSpawnable = nullptr;
static UClass* Class_Engine_Keypoint = nullptr;
static UClass* Class_Engine_Ladder = nullptr;
static UClass* Class_Engine_LadderVolume = nullptr;
static UClass* Class_Engine_LensFlareSource = nullptr;
static UClass* Class_Engine_LevelStreamingVolume = nullptr;
static UClass* Class_Engine_LiftCenter = nullptr;
static UClass* Class_Engine_LiftExit = nullptr;
static UClass* Class_Engine_Light = nullptr;
static UClass* Class_Engine_LightVolume = nullptr;
static UClass* Class_Engine_MantleMarker = nullptr;
static UClass* Class_Engine_MaterialInstanceActor = nullptr;
static UClass* Class_Engine_MatineeActor = nullptr;
static UClass* Class_Engine_Mutator = nullptr;
static UClass* Class_Engine_NavigationPoint = nullptr;
static UClass* Class_Engine_Note = nullptr;
static UClass* Class_Engine_NxCylindricalForceField = nullptr;
static UClass* Class_Engine_NxCylindricalForceFieldCapsule = nullptr;
static UClass* Class_Engine_NxForceField = nullptr;
static UClass* Class_Engine_NxForceFieldGeneric = nullptr;
static UClass* Class_Engine_NxForceFieldRadial = nullptr;
static UClass* Class_Engine_NxForceFieldTornado = nullptr;
static UClass* Class_Engine_NxGenericForceField = nullptr;
static UClass* Class_Engine_NxGenericForceFieldBox = nullptr;
static UClass* Class_Engine_NxGenericForceFieldBrush = nullptr;
static UClass* Class_Engine_NxGenericForceFieldCapsule = nullptr;
static UClass* Class_Engine_NxRadialCustomForceField = nullptr;
static UClass* Class_Engine_NxRadialForceField = nullptr;
static UClass* Class_Engine_NxTornadoAngularForceField = nullptr;
static UClass* Class_Engine_NxTornadoAngularForceFieldCapsule = nullptr;
static UClass* Class_Engine_NxTornadoForceField = nullptr;
static UClass* Class_Engine_NxTornadoForceFieldCapsule = nullptr;
static UClass* Class_Engine_Objective = nullptr;
static UClass* Class_Engine_PathBlockingVolume = nullptr;
static UClass* Class_Engine_PathNode = nullptr;
static UClass* Class_Engine_PathNode_Dynamic = nullptr;
static UClass* Class_Engine_Pawn = nullptr;
static UClass* Class_Engine_PhysXDestructibleActor = nullptr;
static UClass* Class_Engine_PhysXDestructiblePart = nullptr;
static UClass* Class_Engine_PhysXEmitterSpawnable = nullptr;
static UClass* Class_Engine_PhysicsVolume = nullptr;
static UClass* Class_Engine_PickupFactory = nullptr;
static UClass* Class_Engine_PlayerController = nullptr;
static UClass* Class_Engine_PlayerReplicationInfo = nullptr;
static UClass* Class_Engine_PlayerStart = nullptr;
static UClass* Class_Engine_PointLight = nullptr;
static UClass* Class_Engine_PointLightMovable = nullptr;
static UClass* Class_Engine_PointLightToggleable = nullptr;
static UClass* Class_Engine_PolyMarker = nullptr;
static UClass* Class_Engine_PortalMarker = nullptr;
static UClass* Class_Engine_PortalTeleporter = nullptr;
static UClass* Class_Engine_PortalVolume = nullptr;
static UClass* Class_Engine_PostProcessVolume = nullptr;
static UClass* Class_Engine_PotentialClimbWatcher = nullptr;
static UClass* Class_Engine_PrefabInstance = nullptr;
static UClass* Class_Engine_Projectile = nullptr;
static UClass* Class_Engine_RB_BSJointActor = nullptr;
static UClass* Class_Engine_RB_ConstraintActor = nullptr;
static UClass* Class_Engine_RB_ConstraintActorSpawnable = nullptr;
static UClass* Class_Engine_RB_CylindricalForceActor = nullptr;
static UClass* Class_Engine_RB_ForceFieldExcludeVolume = nullptr;
static UClass* Class_Engine_RB_HingeActor = nullptr;
static UClass* Class_Engine_RB_LineImpulseActor = nullptr;
static UClass* Class_Engine_RB_PrismaticActor = nullptr;
static UClass* Class_Engine_RB_PulleyJointActor = nullptr;
static UClass* Class_Engine_RB_RadialForceActor = nullptr;
static UClass* Class_Engine_RB_RadialImpulseActor = nullptr;
static UClass* Class_Engine_RB_Thruster = nullptr;
static UClass* Class_Engine_ReplicationInfo = nullptr;
static UClass* Class_Engine_ReverbVolume = nullptr;
static UClass* Class_Engine_Route = nullptr;
static UClass* Class_Engine_SVehicle = nullptr;
static UClass* Class_Engine_SceneCapture2DActor = nullptr;
static UClass* Class_Engine_SceneCaptureActor = nullptr;
static UClass* Class_Engine_SceneCaptureCubeMapActor = nullptr;
static UClass* Class_Engine_SceneCapturePortalActor = nullptr;
static UClass* Class_Engine_SceneCaptureReflectActor = nullptr;
static UClass* Class_Engine_ScoreBoard = nullptr;
static UClass* Class_Engine_Scout = nullptr;
static UClass* Class_Engine_SkeletalMeshActor = nullptr;
static UClass* Class_Engine_SkeletalMeshActorBasedOnExtremeContent = nullptr;
static UClass* Class_Engine_SkeletalMeshActorMAT = nullptr;
static UClass* Class_Engine_SkeletalMeshActorMATSpawnable = nullptr;
static UClass* Class_Engine_SkeletalMeshActorSpawnable = nullptr;
static UClass* Class_Engine_SkyLight = nullptr;
static UClass* Class_Engine_SkyLightToggleable = nullptr;
static UClass* Class_Engine_SpeedTreeActor = nullptr;
static UClass* Class_Engine_SpotLight = nullptr;
static UClass* Class_Engine_SpotLightMovable = nullptr;
static UClass* Class_Engine_SpotLightToggleable = nullptr;
static UClass* Class_Engine_StaticLightCollectionActor = nullptr;
static UClass* Class_Engine_StaticMeshActor = nullptr;
static UClass* Class_Engine_StaticMeshActorBase = nullptr;
static UClass* Class_Engine_StaticMeshActorBasedOnExtremeContent = nullptr;
static UClass* Class_Engine_StaticMeshCollectionActor = nullptr;
static UClass* Class_Engine_TargetPoint = nullptr;
static UClass* Class_Engine_TeamInfo = nullptr;
static UClass* Class_Engine_Teleporter = nullptr;
static UClass* Class_Engine_Terrain = nullptr;
static UClass* Class_Engine_Trigger = nullptr;
static UClass* Class_Engine_TriggerStreamingLevel = nullptr;
static UClass* Class_Engine_TriggerVolume = nullptr;
static UClass* Class_Engine_Trigger_Dynamic = nullptr;
static UClass* Class_Engine_Trigger_LOS = nullptr;
static UClass* Class_Engine_TriggeredPath = nullptr;
static UClass* Class_Engine_Vehicle = nullptr;
static UClass* Class_Engine_Volume = nullptr;
static UClass* Class_Engine_VolumePathNode = nullptr;
static UClass* Class_Engine_VolumeTimer = nullptr;
static UClass* Class_Engine_WaterVolume = nullptr;
static UClass* Class_Engine_Weapon = nullptr;
static UClass* Class_Engine_WindDirectionalSource = nullptr;
static UClass* Class_Engine_WorldInfo = nullptr;
static UClass* Class_Engine_ZoneInfo = nullptr;
static UClass* Class_GameFramework_GameAIController = nullptr;
static UClass* Class_GameFramework_GameExplosionActor = nullptr;
static UClass* Class_GameFramework_GameHUD = nullptr;
static UClass* Class_GameFramework_GamePawn = nullptr;
static UClass* Class_GameFramework_GamePlayerController = nullptr;
static UClass* Class_GameFramework_GameProjectile = nullptr;
static UClass* Class_GameFramework_GameVehicle = nullptr;
static UClass* Class_GameFramework_GameWeapon = nullptr;
static UClass* Class_TgGame_TgAIController = nullptr;
static UClass* Class_TgGame_TgActionPoint = nullptr;
static UClass* Class_TgGame_TgActorFactory = nullptr;
static UClass* Class_TgGame_TgAlarmPoint = nullptr;
static UClass* Class_TgGame_TgAnnouncer = nullptr;
static UClass* Class_TgGame_TgBaseObjective_CTFBot = nullptr;
static UClass* Class_TgGame_TgBaseObjective_KOTH = nullptr;
static UClass* Class_TgGame_TgBeaconFactory = nullptr;
static UClass* Class_TgGame_TgBotEncounterVolume = nullptr;
static UClass* Class_TgGame_TgBotFactory = nullptr;
static UClass* Class_TgGame_TgBotFactorySpawnable = nullptr;
static UClass* Class_TgGame_TgBotStart = nullptr;
static UClass* Class_TgGame_TgCharacterBuilderLight = nullptr;
static UClass* Class_TgGame_TgChestActor = nullptr;
static UClass* Class_TgGame_TgCollisionProxy = nullptr;
static UClass* Class_TgGame_TgCollisionProxy_Vortex = nullptr;
static UClass* Class_TgGame_TgCoverPoint = nullptr;
static UClass* Class_TgGame_TgDebugCameraController = nullptr;
static UClass* Class_TgGame_TgDecalActor_Logo = nullptr;
static UClass* Class_TgGame_TgDefensePoint = nullptr;
static UClass* Class_TgGame_TgDeploy_Artillery = nullptr;
static UClass* Class_TgGame_TgDeploy_Beacon = nullptr;
static UClass* Class_TgGame_TgDeploy_BeaconEntrance = nullptr;
static UClass* Class_TgGame_TgDeploy_DestructibleCover = nullptr;
static UClass* Class_TgGame_TgDeploy_ForceField = nullptr;
static UClass* Class_TgGame_TgDeploy_Sensor = nullptr;
static UClass* Class_TgGame_TgDeploy_SweepSensor = nullptr;
static UClass* Class_TgGame_TgDeployable = nullptr;
static UClass* Class_TgGame_TgDeployableFactory = nullptr;
static UClass* Class_TgGame_TgDestructibleFactory = nullptr;
static UClass* Class_TgGame_TgDevice = nullptr;
static UClass* Class_TgGame_TgDeviceVolume = nullptr;
static UClass* Class_TgGame_TgDeviceVolumeInfo = nullptr;
static UClass* Class_TgGame_TgDevice_Grenade = nullptr;
static UClass* Class_TgGame_TgDevice_HitPulse = nullptr;
static UClass* Class_TgGame_TgDevice_MeleeDualWield = nullptr;
static UClass* Class_TgGame_TgDevice_Morale = nullptr;
static UClass* Class_TgGame_TgDevice_NewMelee = nullptr;
static UClass* Class_TgGame_TgDevice_NewRange = nullptr;
static UClass* Class_TgGame_TgDoor = nullptr;
static UClass* Class_TgGame_TgDoorMarker = nullptr;
static UClass* Class_TgGame_TgDroppedItem = nullptr;
static UClass* Class_TgGame_TgDummyActor = nullptr;
static UClass* Class_TgGame_TgDynamicDestructible = nullptr;
static UClass* Class_TgGame_TgDynamicSMActor = nullptr;
static UClass* Class_TgGame_TgEffectManager = nullptr;
static UClass* Class_TgGame_TgElevatingVolume = nullptr;
static UClass* Class_TgGame_TgEmitter = nullptr;
static UClass* Class_TgGame_TgEmitterCrashlanding = nullptr;
static UClass* Class_TgGame_TgEmitterSpawnable = nullptr;
static UClass* Class_TgGame_TgFlagCaptureVolume = nullptr;
static UClass* Class_TgGame_TgFracturedStaticMeshActor = nullptr;
static UClass* Class_TgGame_TgGame = nullptr;
static UClass* Class_TgGame_TgGame_Arena = nullptr;
static UClass* Class_TgGame_TgGame_CTF = nullptr;
static UClass* Class_TgGame_TgGame_City = nullptr;
static UClass* Class_TgGame_TgGame_Control = nullptr;
static UClass* Class_TgGame_TgGame_Defense = nullptr;
static UClass* Class_TgGame_TgGame_DualCTF = nullptr;
static UClass* Class_TgGame_TgGame_Escort = nullptr;
static UClass* Class_TgGame_TgGame_Mission = nullptr;
static UClass* Class_TgGame_TgGame_OpenWorld = nullptr;
static UClass* Class_TgGame_TgGame_OpenWorldPVE = nullptr;
static UClass* Class_TgGame_TgGame_OpenWorldPVP = nullptr;
static UClass* Class_TgGame_TgGame_PointRotation = nullptr;
static UClass* Class_TgGame_TgGame_Ticket = nullptr;
static UClass* Class_TgGame_TgHUD = nullptr;
static UClass* Class_TgGame_TgHeightFog = nullptr;
static UClass* Class_TgGame_TgHelpAlertVolume = nullptr;
static UClass* Class_TgGame_TgHexItemFactory = nullptr;
static UClass* Class_TgGame_TgHexLandMarkActor = nullptr;
static UClass* Class_TgGame_TgHitDisplayActor = nullptr;
static UClass* Class_TgGame_TgHoldSpot = nullptr;
static UClass* Class_TgGame_TgInterpActor = nullptr;
static UClass* Class_TgGame_TgInterpolatingCameraActor = nullptr;
static UClass* Class_TgGame_TgInventoryManager = nullptr;
static UClass* Class_TgGame_TgKActorSpawnable = nullptr;
static UClass* Class_TgGame_TgKAssetSpawnable = nullptr;
static UClass* Class_TgGame_TgKAsset_ClientSideSim = nullptr;
static UClass* Class_TgGame_TgKismetTestActor = nullptr;
static UClass* Class_TgGame_TgLevelCamera = nullptr;
static UClass* Class_TgGame_TgMeshAssembly = nullptr;
static UClass* Class_TgGame_TgMiniMapActor = nullptr;
static UClass* Class_TgGame_TgMissionListVolume = nullptr;
static UClass* Class_TgGame_TgMissionObjective = nullptr;
static UClass* Class_TgGame_TgMissionObjective_Bot = nullptr;
static UClass* Class_TgGame_TgMissionObjective_Escort = nullptr;
static UClass* Class_TgGame_TgMissionObjective_Kismet = nullptr;
static UClass* Class_TgGame_TgMissionObjective_Proximity = nullptr;
static UClass* Class_TgGame_TgModifyPawnPropertiesVolume = nullptr;
static UClass* Class_TgGame_TgMorphFX = nullptr;
static UClass* Class_TgGame_TgNavRouteIndicator = nullptr;
static UClass* Class_TgGame_TgNavigationPoint = nullptr;
static UClass* Class_TgGame_TgNavigationPointSpawnable = nullptr;
static UClass* Class_TgGame_TgNewsStand = nullptr;
static UClass* Class_TgGame_TgObjectiveAssignment = nullptr;
static UClass* Class_TgGame_TgObjectiveAttachActor = nullptr;
static UClass* Class_TgGame_TgOmegaVolume = nullptr;
static UClass* Class_TgGame_TgPawn = nullptr;
static UClass* Class_TgGame_TgPawn_AVCompositeWalker = nullptr;
static UClass* Class_TgGame_TgPawn_Ambush = nullptr;
static UClass* Class_TgGame_TgPawn_AndroidMinion = nullptr;
static UClass* Class_TgGame_TgPawn_AttackTransport = nullptr;
static UClass* Class_TgGame_TgPawn_Boss = nullptr;
static UClass* Class_TgGame_TgPawn_Boss_Destroyer = nullptr;
static UClass* Class_TgGame_TgPawn_Brawler = nullptr;
static UClass* Class_TgGame_TgPawn_CTR = nullptr;
static UClass* Class_TgGame_TgPawn_Character = nullptr;
static UClass* Class_TgGame_TgPawn_ColonyEye = nullptr;
static UClass* Class_TgGame_TgPawn_Destructible = nullptr;
static UClass* Class_TgGame_TgPawn_Detonator = nullptr;
static UClass* Class_TgGame_TgPawn_Dismantler = nullptr;
static UClass* Class_TgGame_TgPawn_DuneCommander = nullptr;
static UClass* Class_TgGame_TgPawn_Elite_Alchemist = nullptr;
static UClass* Class_TgGame_TgPawn_Elite_Assassin = nullptr;
static UClass* Class_TgGame_TgPawn_EscortRobot = nullptr;
static UClass* Class_TgGame_TgPawn_FlyingBoss = nullptr;
static UClass* Class_TgGame_TgPawn_GroundPetA = nullptr;
static UClass* Class_TgGame_TgPawn_Guardian = nullptr;
static UClass* Class_TgGame_TgPawn_Hover = nullptr;
static UClass* Class_TgGame_TgPawn_HoverShieldSphere = nullptr;
static UClass* Class_TgGame_TgPawn_Hunter = nullptr;
static UClass* Class_TgGame_TgPawn_Inquisitor = nullptr;
static UClass* Class_TgGame_TgPawn_Interact_NPC = nullptr;
static UClass* Class_TgGame_TgPawn_Iris = nullptr;
static UClass* Class_TgGame_TgPawn_Juggernaut = nullptr;
static UClass* Class_TgGame_TgPawn_Marauder = nullptr;
static UClass* Class_TgGame_TgPawn_NPC = nullptr;
static UClass* Class_TgGame_TgPawn_NewWasp = nullptr;
static UClass* Class_TgGame_TgPawn_Raptor = nullptr;
static UClass* Class_TgGame_TgPawn_Reaper = nullptr;
static UClass* Class_TgGame_TgPawn_RecursiveSpawner = nullptr;
static UClass* Class_TgGame_TgPawn_Remote = nullptr;
static UClass* Class_TgGame_TgPawn_Robot = nullptr;
static UClass* Class_TgGame_TgPawn_Scanner = nullptr;
static UClass* Class_TgGame_TgPawn_ScannerRecursive = nullptr;
static UClass* Class_TgGame_TgPawn_Siege = nullptr;
static UClass* Class_TgGame_TgPawn_SiegeBarrage = nullptr;
static UClass* Class_TgGame_TgPawn_SiegeHover = nullptr;
static UClass* Class_TgGame_TgPawn_SiegeRapidFire = nullptr;
static UClass* Class_TgGame_TgPawn_Sniper = nullptr;
static UClass* Class_TgGame_TgPawn_SonoranCommander = nullptr;
static UClass* Class_TgGame_TgPawn_SupportForeman = nullptr;
static UClass* Class_TgGame_TgPawn_Switchblade = nullptr;
static UClass* Class_TgGame_TgPawn_Tentacle = nullptr;
static UClass* Class_TgGame_TgPawn_ThinkTank = nullptr;
static UClass* Class_TgGame_TgPawn_TreadRobot = nullptr;
static UClass* Class_TgGame_TgPawn_Turret = nullptr;
static UClass* Class_TgGame_TgPawn_TurretAVAFlak = nullptr;
static UClass* Class_TgGame_TgPawn_TurretAVARocket = nullptr;
static UClass* Class_TgGame_TgPawn_TurretFlak = nullptr;
static UClass* Class_TgGame_TgPawn_TurretFlame = nullptr;
static UClass* Class_TgGame_TgPawn_TurretPlasma = nullptr;
static UClass* Class_TgGame_TgPawn_UberWalker = nullptr;
static UClass* Class_TgGame_TgPawn_Vanguard = nullptr;
static UClass* Class_TgGame_TgPawn_VanityPet = nullptr;
static UClass* Class_TgGame_TgPawn_Vulcan = nullptr;
static UClass* Class_TgGame_TgPawn_Warlord = nullptr;
static UClass* Class_TgGame_TgPawn_WaspSpawner = nullptr;
static UClass* Class_TgGame_TgPawn_Widow = nullptr;
static UClass* Class_TgGame_TgPhysAnimTestActor = nullptr;
static UClass* Class_TgGame_TgPickupFactory = nullptr;
static UClass* Class_TgGame_TgPickupFactory_Item = nullptr;
static UClass* Class_TgGame_TgPlayerController = nullptr;
static UClass* Class_TgGame_TgPlayerCountVolume = nullptr;
static UClass* Class_TgGame_TgPointOfInterest = nullptr;
static UClass* Class_TgGame_TgPostProcessVolume = nullptr;
static UClass* Class_TgGame_TgProj_Bot = nullptr;
static UClass* Class_TgGame_TgProj_Bounce = nullptr;
static UClass* Class_TgGame_TgProj_Deployable = nullptr;
static UClass* Class_TgGame_TgProj_FreeGrenade = nullptr;
static UClass* Class_TgGame_TgProj_Grapple = nullptr;
static UClass* Class_TgGame_TgProj_Grenade = nullptr;
static UClass* Class_TgGame_TgProj_Missile = nullptr;
static UClass* Class_TgGame_TgProj_Mortar = nullptr;
static UClass* Class_TgGame_TgProj_Net = nullptr;
static UClass* Class_TgGame_TgProj_Rocket = nullptr;
static UClass* Class_TgGame_TgProj_StraightTeleporter = nullptr;
static UClass* Class_TgGame_TgProj_Teleporter = nullptr;
static UClass* Class_TgGame_TgProjectile = nullptr;
static UClass* Class_TgGame_TgQueuedAnnouncement = nullptr;
static UClass* Class_TgGame_TgRandomSMActor = nullptr;
static UClass* Class_TgGame_TgRandomSMManager = nullptr;
static UClass* Class_TgGame_TgReferenceArray = nullptr;
static UClass* Class_TgGame_TgRepInfo_Beacon = nullptr;
static UClass* Class_TgGame_TgRepInfo_Deployable = nullptr;
static UClass* Class_TgGame_TgRepInfo_Game = nullptr;
static UClass* Class_TgGame_TgRepInfo_GameOpenWorld = nullptr;
static UClass* Class_TgGame_TgRepInfo_Player = nullptr;
static UClass* Class_TgGame_TgRepInfo_TaskForce = nullptr;
static UClass* Class_TgGame_TgScoreboard = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActorGenericUIPreview = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActorNPC = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActorNPCVendor = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActorSpawnable = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor_CharacterBuilder = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor_CharacterBuilderSpawnable = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor_Composite = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor_EquipScreen = nullptr;
static UClass* Class_TgGame_TgSkeletalMeshActor_MeleePreVis = nullptr;
static UClass* Class_TgGame_TgSkydiveTarget = nullptr;
static UClass* Class_TgGame_TgSkydivingVolume = nullptr;
static UClass* Class_TgGame_TgSoundInsulationVolume = nullptr;
static UClass* Class_TgGame_TgStartPoint = nullptr;
static UClass* Class_TgGame_TgStartpointPortalNetwork = nullptr;
static UClass* Class_TgGame_TgStaticMeshActor = nullptr;
static UClass* Class_TgGame_TgStaticMeshActor_Logo = nullptr;
static UClass* Class_TgGame_TgTeamBeaconManager = nullptr;
static UClass* Class_TgGame_TgTeamBlocker = nullptr;
static UClass* Class_TgGame_TgTeamMarker = nullptr;
static UClass* Class_TgGame_TgTeamPlayerStart = nullptr;
static UClass* Class_TgGame_TgTeamScoreboard = nullptr;
static UClass* Class_TgGame_TgTeleportPlayerVolume = nullptr;
static UClass* Class_TgGame_TgTeleporter = nullptr;
static UClass* Class_TgGame_TgTimerManager = nullptr;
static UClass* Class_TgGame_TgTrigger_Instance = nullptr;
static UClass* Class_TgGame_TgTrigger_Use = nullptr;
static UClass* Class_TgGame_TgVolumePathNode = nullptr;
static UClass* Class_TgGame_TgWaterVolume = nullptr;
static UClass* Class_TgGame_TgWindManager = nullptr;

// Backing storage for the unified initial-rep flag (see DO_REP in Macros.hpp).
std::unordered_set<AActor*> g_RepListInitialDoneActors;
UProperty* ObjectProperty_Engine_Actor_Base = nullptr;
UProperty* ByteProperty_Engine_Actor_Physics = nullptr;
UProperty* StructProperty_Engine_Actor_Velocity = nullptr;
UProperty* ByteProperty_Engine_Actor_RemoteRole = nullptr;
UProperty* ByteProperty_Engine_Actor_Role = nullptr;
UProperty* BoolProperty_Engine_Actor_bNetOwner = nullptr;
UProperty* BoolProperty_Engine_Actor_bTearOff = nullptr;
UProperty* FloatProperty_Engine_Actor_DrawScale = nullptr;
UProperty* ByteProperty_Engine_Actor_ReplicatedCollisionType = nullptr;
UProperty* BoolProperty_Engine_Actor_bCollideActors = nullptr;
UProperty* BoolProperty_Engine_Actor_bCollideWorld = nullptr;
UProperty* BoolProperty_Engine_Actor_bBlockActors = nullptr;
UProperty* BoolProperty_Engine_Actor_bProjTarget = nullptr;
UProperty* ObjectProperty_Engine_Actor_Instigator = nullptr;
UProperty* ObjectProperty_Engine_Actor_Owner = nullptr;
UProperty* BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying = nullptr;
UProperty* FloatProperty_Engine_CameraActor_AspectRatio = nullptr;
UProperty* FloatProperty_Engine_CameraActor_FOVAngle = nullptr;
UProperty* ObjectProperty_Engine_Controller_Pawn = nullptr;
UProperty* ObjectProperty_Engine_Controller_PlayerReplicationInfo = nullptr;
UProperty* BoolProperty_Engine_CrowdAttractor_bAttractorEnabled = nullptr;
UProperty* IntProperty_Engine_CrowdReplicationActor_DestroyAllCount = nullptr;
UProperty* ObjectProperty_Engine_CrowdReplicationActor_Spawner = nullptr;
UProperty* BoolProperty_Engine_CrowdReplicationActor_bSpawningActive = nullptr;
UProperty* ClassProperty_Engine_DroppedPickup_InventoryClass = nullptr;
UProperty* BoolProperty_Engine_DroppedPickup_bFadeOut = nullptr;
UProperty* ObjectProperty_Engine_DynamicSMActor_ReplicatedMaterial = nullptr;
UProperty* ObjectProperty_Engine_DynamicSMActor_ReplicatedMesh = nullptr;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshRotation = nullptr;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshScale3D = nullptr;
UProperty* StructProperty_Engine_DynamicSMActor_ReplicatedMeshTranslation = nullptr;
UProperty* BoolProperty_Engine_DynamicSMActor_bForceStaticDecals = nullptr;
UProperty* BoolProperty_Engine_Emitter_bCurrentlyActive = nullptr;
UProperty* ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate = nullptr;
UProperty* BoolProperty_Engine_FluidInfluenceActor_bActive = nullptr;
UProperty* BoolProperty_Engine_FluidInfluenceActor_bToggled = nullptr;
UProperty* BoolProperty_Engine_FogVolumeDensityInfo_bEnabled = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_MatchID = nullptr;
UProperty* ObjectProperty_Engine_GameReplicationInfo_Winner = nullptr;
UProperty* BoolProperty_Engine_GameReplicationInfo_bMatchHasBegun = nullptr;
UProperty* BoolProperty_Engine_GameReplicationInfo_bMatchIsOver = nullptr;
UProperty* BoolProperty_Engine_GameReplicationInfo_bStopCountDown = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_RemainingMinute = nullptr;
UProperty* StrProperty_Engine_GameReplicationInfo_AdminEmail = nullptr;
UProperty* StrProperty_Engine_GameReplicationInfo_AdminName = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_ElapsedTime = nullptr;
UProperty* ClassProperty_Engine_GameReplicationInfo_GameClass = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_GoalScore = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_MaxLives = nullptr;
UProperty* StrProperty_Engine_GameReplicationInfo_MessageOfTheDay = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_RemainingTime = nullptr;
UProperty* StrProperty_Engine_GameReplicationInfo_ServerName = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_ServerRegion = nullptr;
UProperty* StrProperty_Engine_GameReplicationInfo_ShortName = nullptr;
UProperty* IntProperty_Engine_GameReplicationInfo_TimeLimit = nullptr;
UProperty* BoolProperty_Engine_GameReplicationInfo_bIsArbitrated = nullptr;
UProperty* BoolProperty_Engine_GameReplicationInfo_bTrackStats = nullptr;
UProperty* BoolProperty_Engine_HeightFog_bEnabled = nullptr;
UProperty* ObjectProperty_Engine_Inventory_InvManager = nullptr;
UProperty* ObjectProperty_Engine_Inventory_Inventory = nullptr;
UProperty* ObjectProperty_Engine_InventoryManager_InventoryChain = nullptr;
UProperty* StructProperty_Engine_KActor_RBState = nullptr;
UProperty* StructProperty_Engine_KActor_ReplicatedDrawScale3D = nullptr;
UProperty* BoolProperty_Engine_KActor_bWakeOnLevelStart = nullptr;
UProperty* ObjectProperty_Engine_KAsset_ReplicatedMesh = nullptr;
UProperty* ObjectProperty_Engine_KAsset_ReplicatedPhysAsset = nullptr;
UProperty* BoolProperty_Engine_LensFlareSource_bCurrentlyActive = nullptr;
UProperty* BoolProperty_Engine_Light_bEnabled = nullptr;
UProperty* ObjectProperty_Engine_MatineeActor_InterpAction = nullptr;
UProperty* FloatProperty_Engine_MatineeActor_PlayRate = nullptr;
UProperty* FloatProperty_Engine_MatineeActor_Position = nullptr;
UProperty* BoolProperty_Engine_MatineeActor_bIsPlaying = nullptr;
UProperty* BoolProperty_Engine_MatineeActor_bPaused = nullptr;
UProperty* BoolProperty_Engine_MatineeActor_bReversePlayback = nullptr;
UProperty* BoolProperty_Engine_NxForceField_bForceActive = nullptr;
UProperty* ObjectProperty_Engine_Pawn_DrivenVehicle = nullptr;
UProperty* StructProperty_Engine_Pawn_FlashLocation = nullptr;
UProperty* IntProperty_Engine_Pawn_Health = nullptr;
UProperty* ClassProperty_Engine_Pawn_HitDamageType = nullptr;
UProperty* ObjectProperty_Engine_Pawn_PlayerReplicationInfo = nullptr;
UProperty* StructProperty_Engine_Pawn_TakeHitLocation = nullptr;
UProperty* BoolProperty_Engine_Pawn_bIsWalking = nullptr;
UProperty* BoolProperty_Engine_Pawn_bSimulateGravity = nullptr;
UProperty* FloatProperty_Engine_Pawn_AccelRate = nullptr;
UProperty* FloatProperty_Engine_Pawn_AirControl = nullptr;
UProperty* FloatProperty_Engine_Pawn_AirSpeed = nullptr;
UProperty* ObjectProperty_Engine_Pawn_Controller = nullptr;
UProperty* FloatProperty_Engine_Pawn_GroundSpeed = nullptr;
UProperty* ObjectProperty_Engine_Pawn_InvManager = nullptr;
UProperty* FloatProperty_Engine_Pawn_JumpZ = nullptr;
UProperty* FloatProperty_Engine_Pawn_WaterSpeed = nullptr;
UProperty* ByteProperty_Engine_Pawn_FiringMode = nullptr;
UProperty* ByteProperty_Engine_Pawn_FlashCount = nullptr;
UProperty* BoolProperty_Engine_Pawn_bIsCrouched = nullptr;
UProperty* StructProperty_Engine_Pawn_TearOffMomentum = nullptr;
UProperty* ByteProperty_Engine_Pawn_RemoteViewPitch = nullptr;
UProperty* ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate = nullptr;
UProperty* BoolProperty_Engine_PickupFactory_bPickupHidden = nullptr;
UProperty* ClassProperty_Engine_PickupFactory_InventoryType = nullptr;
UProperty* FloatProperty_Engine_PlayerController_TargetEyeHeight = nullptr;
UProperty* StructProperty_Engine_PlayerController_TargetViewRotation = nullptr;
UProperty* FloatProperty_Engine_PlayerReplicationInfo_Deaths = nullptr;
UProperty* StrProperty_Engine_PlayerReplicationInfo_PlayerAlias = nullptr;
UProperty* ObjectProperty_Engine_PlayerReplicationInfo_PlayerLocationHint = nullptr;
UProperty* StrProperty_Engine_PlayerReplicationInfo_PlayerName = nullptr;
UProperty* IntProperty_Engine_PlayerReplicationInfo_PlayerSkill = nullptr;
UProperty* FloatProperty_Engine_PlayerReplicationInfo_Score = nullptr;
UProperty* IntProperty_Engine_PlayerReplicationInfo_StartTime = nullptr;
UProperty* ObjectProperty_Engine_PlayerReplicationInfo_Team = nullptr;
UProperty* StructProperty_Engine_PlayerReplicationInfo_UniqueId = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bAdmin = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bHasFlag = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsFemale = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsSpectator = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bOnlySpectator = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bOutOfLives = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bReadyToPlay = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bWaitingPlayer = nullptr;
UProperty* ByteProperty_Engine_PlayerReplicationInfo_PacketLoss = nullptr;
UProperty* ByteProperty_Engine_PlayerReplicationInfo_Ping = nullptr;
UProperty* IntProperty_Engine_PlayerReplicationInfo_SplitscreenIndex = nullptr;
UProperty* IntProperty_Engine_PlayerReplicationInfo_PlayerID = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bBot = nullptr;
UProperty* BoolProperty_Engine_PlayerReplicationInfo_bIsInactive = nullptr;
UProperty* BoolProperty_Engine_PostProcessVolume_bEnabled = nullptr;
UProperty* FloatProperty_Engine_Projectile_MaxSpeed = nullptr;
UProperty* FloatProperty_Engine_Projectile_Speed = nullptr;
UProperty* BoolProperty_Engine_RB_CylindricalForceActor_bForceActive = nullptr;
UProperty* ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount = nullptr;
UProperty* BoolProperty_Engine_RB_RadialForceActor_bForceActive = nullptr;
UProperty* ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount = nullptr;
UProperty* FloatProperty_Engine_SVehicle_MaxSpeed = nullptr;
UProperty* StructProperty_Engine_SVehicle_VState = nullptr;
UProperty* ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial = nullptr;
UProperty* ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh = nullptr;
UProperty* FloatProperty_Engine_TeamInfo_Score = nullptr;
UProperty* IntProperty_Engine_TeamInfo_TeamIndex = nullptr;
UProperty* StrProperty_Engine_TeamInfo_TeamName = nullptr;
UProperty* StrProperty_Engine_Teleporter_URL = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bEnabled = nullptr;
UProperty* StructProperty_Engine_Teleporter_TargetVelocity = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bChangesVelocity = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bChangesYaw = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bReversesX = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bReversesY = nullptr;
UProperty* BoolProperty_Engine_Teleporter_bReversesZ = nullptr;
UProperty* BoolProperty_Engine_Vehicle_bDriving = nullptr;
UProperty* ObjectProperty_Engine_Vehicle_Driver = nullptr;
UProperty* ObjectProperty_Engine_WorldInfo_Pauser = nullptr;
UProperty* StructProperty_Engine_WorldInfo_ReplicatedMusicTrack = nullptr;
UProperty* FloatProperty_Engine_WorldInfo_TimeDilation = nullptr;
UProperty* FloatProperty_Engine_WorldInfo_WorldGravityZ = nullptr;
UProperty* BoolProperty_Engine_WorldInfo_bHighPriorityLoading = nullptr;
UProperty* ByteProperty_TgGame_TgChestActor_r_eChestState = nullptr;
UProperty* BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive = nullptr;
UProperty* BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired = nullptr;
UProperty* IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning = nullptr;
UProperty* IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount = nullptr;
UProperty* BoolProperty_TgGame_TgDeployable_r_bDelayDeployed = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt = nullptr;
// Regen gap: UC TgDeployable.uc lines 116-120 declares these as always-replicate
// CPF_Net fields (Role==Authority block) but the generator dropped them.
// Without r_nFlashFireCount replicating, the client's
// ReplicatedEvent('r_nFlashFireCount') never fires → PlayFireFx() skipped →
// no heal-pulse visual/audio on stations.  r_nHealth is needed for pulse
// damage/heal UI.  r_fDeployRate/r_fTimeToDeploySecs/r_fInitDeployTime drive
// the client's deploy-progress bar during the Deploy state.
UProperty* ObjectProperty_TgGame_TgDeployable_r_EffectManager = nullptr;
UProperty* FloatProperty_TgGame_TgDeployable_r_fDeployRate = nullptr;
UProperty* FloatProperty_TgGame_TgDeployable_r_fInitDeployTime = nullptr;
UProperty* FloatProperty_TgGame_TgDeployable_r_fTimeToDeploySecs = nullptr;
UProperty* ByteProperty_TgGame_TgDeployable_r_nFlashCount = nullptr;
UProperty* ByteProperty_TgGame_TgDeployable_r_nFlashFireCount = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nHealth = nullptr;
UProperty* StructProperty_TgGame_TgDeployable_r_vFlashLocation = nullptr;
UProperty* ObjectProperty_TgGame_TgDeployable_r_DRI = nullptr;
UProperty* BoolProperty_TgGame_TgDeployable_r_bInitialIsEnemy = nullptr;
UProperty* BoolProperty_TgGame_TgDeployable_r_bTakeDamage = nullptr;
UProperty* FloatProperty_TgGame_TgDeployable_r_fClientProximityRadius = nullptr;
UProperty* FloatProperty_TgGame_TgDeployable_r_fCurrentDeployTime = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nDeployableId = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nPhysicalType = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nTickingTime = nullptr;
UProperty* ObjectProperty_TgGame_TgDeployable_r_Owner = nullptr;
UProperty* IntProperty_TgGame_TgDeployable_r_nOwnerFireMode = nullptr;
UProperty* ByteProperty_TgGame_TgDevice_CurrentFireMode = nullptr;
UProperty* BoolProperty_TgGame_TgDevice_r_bIsStealthDevice = nullptr;
UProperty* ByteProperty_TgGame_TgDevice_r_eEquippedAt = nullptr;
UProperty* IntProperty_TgGame_TgDevice_r_nInventoryId = nullptr;
UProperty* IntProperty_TgGame_TgDevice_r_nMeleeComboSeed = nullptr;
UProperty* BoolProperty_TgGame_TgDevice_r_bConsumedOnDeath = nullptr;
UProperty* BoolProperty_TgGame_TgDevice_r_bConsumedOnUse = nullptr;
UProperty* IntProperty_TgGame_TgDevice_r_nDeviceId = nullptr;
UProperty* IntProperty_TgGame_TgDevice_r_nDeviceInstanceId = nullptr;
UProperty* IntProperty_TgGame_TgDevice_r_nQualityValueId = nullptr;
UProperty* BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring = nullptr;
UProperty* BoolProperty_TgGame_TgDoor_r_bOpen = nullptr;
UProperty* ByteProperty_TgGame_TgDoorMarker_r_eStatus = nullptr;
UProperty* IntProperty_TgGame_TgDroppedItem_r_nItemId = nullptr;
UProperty* IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId = nullptr;
UProperty* ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory = nullptr;
UProperty* StrProperty_TgGame_TgDynamicSMActor_m_sAssembly = nullptr;
UProperty* ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager = nullptr;
UProperty* IntProperty_TgGame_TgDynamicSMActor_r_nHealth = nullptr;
UProperty* StructProperty_TgGame_TgEffectManager_r_EventQueue = nullptr;
UProperty* StructProperty_TgGame_TgEffectManager_r_ManagedEffectList = nullptr;
UProperty* ObjectProperty_TgGame_TgEffectManager_r_Owner = nullptr;
UProperty* BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify = nullptr;
UProperty* IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount = nullptr;
UProperty* IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex = nullptr;
UProperty* NameProperty_TgGame_TgEmitter_BoneName = nullptr;
UProperty* ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition = nullptr;
UProperty* ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce = nullptr;
UProperty* ObjectProperty_TgGame_TgFracturedStaticMeshActor_r_EffectManager = nullptr;
UProperty* IntProperty_TgGame_TgFracturedStaticMeshActor_r_TakeHitNotifier = nullptr;
UProperty* FloatProperty_TgGame_TgFracturedStaticMeshActor_r_DamageRadius = nullptr;
UProperty* ClassProperty_TgGame_TgFracturedStaticMeshActor_r_HitDamageType = nullptr;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_HitInfo = nullptr;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitLocation = nullptr;
UProperty* StructProperty_TgGame_TgFracturedStaticMeshActor_r_vTakeHitMomentum = nullptr;
UProperty* IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId = nullptr;
UProperty* StrProperty_TgGame_TgInterpActor_r_sCurrState = nullptr;
UProperty* IntProperty_TgGame_TgInventoryManager_r_ItemCount = nullptr;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest = nullptr;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nFailCount = nullptr;
UProperty* IntProperty_TgGame_TgKismetTestActor_r_nPassCount = nullptr;
UProperty* BoolProperty_TgGame_TgLevelCamera_r_bEnabled = nullptr;
UProperty* ObjectProperty_TgGame_TgMissionObjective_r_ObjectiveAssignment = nullptr;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bHasBeenCapturedOnce = nullptr;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsActive = nullptr;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsLocked = nullptr;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bIsPending = nullptr;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eOwningCoalition = nullptr;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eStatus = nullptr;
UProperty* FloatProperty_TgGame_TgMissionObjective_r_fCurrCaptureTime = nullptr;
UProperty* FloatProperty_TgGame_TgMissionObjective_r_fLastCompletedTime = nullptr;
UProperty* IntProperty_TgGame_TgMissionObjective_r_nOwnerTaskForce = nullptr;
UProperty* IntProperty_TgGame_TgMissionObjective_nObjectiveId = nullptr;
UProperty* IntProperty_TgGame_TgMissionObjective_nPriority = nullptr;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_OpenWorldPlayerDefaultRole = nullptr;
UProperty* BoolProperty_TgGame_TgMissionObjective_r_bUsePendingState = nullptr;
UProperty* ByteProperty_TgGame_TgMissionObjective_r_eDefaultCoalition = nullptr;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot = nullptr;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo = nullptr;
UProperty* ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor = nullptr;
UProperty* FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate = nullptr;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective = nullptr;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers = nullptr;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots = nullptr;
UProperty* ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders = nullptr;
UProperty* ByteProperty_TgGame_TgObjectiveAssignment_r_eState = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsBot = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHenchman = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bNeedPlaySpawnFx = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fMakeVisibleIncreased = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nAllianceId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nBodyMeshAsmId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nBotRankValueId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashEvent = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashFireInfo = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nFlashQueIndex = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nPawnId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nPhysicalType = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nPreyProfileType = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nProfileId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nProfileTypeValueId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nSoundGroupId = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_vFlashLocation = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_vFlashRayDir = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_vFlashRefireTime = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_vFlashSituationalAttack = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_EquipDeviceInfo = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_bMadeSound = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_eDesiredInHand = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_eEquippedInHandMode = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nReplicateHit = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_ControlPawn = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentOmegaVolume = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneBilboardVol = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_CurrentSubzoneVol = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_ScannerSettings = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_UIClockState = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_UIClockTime = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_UITextBox1MessageID = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_UITextBox1Packet = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_UITextBox1Time = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_UITextBox2MessageID = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_UITextBox2Packet = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_UITextBox2Time = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bAllowAddMoralePoints = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bDisableAllDevices = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableCrafting = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableEquip = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bEnableSkills = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bInCombatFlag = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bInGlobalOffhandCooldown = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fCurrentPowerPool = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fCurrentServerMoralePoints = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fMaxControlRange = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fMaxPowerPool = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fMoraleRechargeRate = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fRequiredMoralePoints = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fSkillRating = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nCurrency = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nHZPoints = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nMoraleDeviceSlot = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nRestDeviceSlot = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nToken = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nXp = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_DistanceToPushback = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_EffectManager = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_FlightAcceleration = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_HangingRotation = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_Owner = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_Pet = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_PlayAnimation = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_PushbackDirection = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_Target = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_TargetActor = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugDestination = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugNextNav = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_r_aDebugTarget = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_bAimType = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bAimingMode = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bCallingForHelp = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsAFK = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsAnimInStrafeMode = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsCrafting = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsCrewing = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsDecoy = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsGrappleDismounting = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHacked = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHacking = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHanging = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsHangingDismounting = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsInSnipeScope = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsRappelling = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bIsStealthed = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bJumpedFromHanging = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bPostureIgnoreTransition = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bResistTagging = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bShouldKnockDownAnimFaceDown = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bTagEnemy = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_r_bUsingBinoculars = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_eCurrentStunType = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_eDeathReason = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteLength = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteRepnotify = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_eEmoteUpdate = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_r_ePosture = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fDeployRate = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fFrictionMultiplier = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fGravityZModifier = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fKnockDownTimeRemaining = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fMakeVisibleFadeRate = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fPostureRateScale = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fRappelGravityModifier = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_r_fStealthTransitionTime = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_fWeightBonus = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_iKnockDownFlash = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nApplyStealth = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nBotSoundCueId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugAggroRange = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugFOV = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugHearingRange = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nDebugSightRange = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nGenericAIEventIndex = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nHealthMaximum = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nNumberTimesCrewed = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nPhase = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nPitchOffset = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nReplicateDying = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nResetCharacter = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nSensorAlertLevel = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nShieldHealthMax = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nShieldHealthRemaining = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nSilentMode = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthAggroRange = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthDisabled = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthSensorRange = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nStealthTypeCode = nullptr;
UProperty* IntProperty_TgGame_TgPawn_r_nYawOffset = nullptr;
UProperty* StrProperty_TgGame_TgPawn_r_sDebugAction = nullptr;
UProperty* StrProperty_TgGame_TgPawn_r_sDebugName = nullptr;
UProperty* StrProperty_TgGame_TgPawn_r_sFactory = nullptr;
UProperty* StructProperty_TgGame_TgPawn_r_vDown = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType = nullptr;
UProperty* StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn = nullptr;
UProperty* IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer = nullptr;
UProperty* IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings = nullptr;
UProperty* StructProperty_TgGame_TgPawn_Character_r_CustomCharacterAssembly = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_Character_r_eAttachedMesh = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nBoostTimeRemaining = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nHeadMeshAsmId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nItemProfileId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nItemProfileNbr = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nMaxMorphIndexSentFromServer = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nMorphSettings = nullptr;
UProperty* ObjectProperty_TgGame_TgPawn_Character_r_CurrentVanityPet = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Character_r_WallJumpUpperLineCheckOffset = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Character_r_WallJumpZ = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_Character_r_bElfGogglesEquipped = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nDeviceSlotUnlockGrpId = nullptr;
UProperty* IntProperty_TgGame_TgPawn_Character_r_nSkillGroupSetId = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct = nullptr;
UProperty* ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection = nullptr;
UProperty* BoolProperty_TgGame_TgPawn_Turret_r_bIsDeployed = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fInitDeployTime = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fTimeToDeploySecs = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fCurrentDeployTime = nullptr;
UProperty* FloatProperty_TgGame_TgPawn_Turret_r_fDeployMaxHealthPCT = nullptr;
UProperty* IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId = nullptr;
UProperty* ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer = nullptr;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects = nullptr;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bGMInvisible = nullptr;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot = nullptr;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation = nullptr;
UProperty* BoolProperty_TgGame_TgPlayerController_r_bRove = nullptr;
UProperty* StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation = nullptr;
UProperty* ObjectProperty_TgGame_TgProj_Missile_r_aSeeking = nullptr;
UProperty* StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation = nullptr;
UProperty* IntProperty_TgGame_TgProj_Missile_r_nNumBounces = nullptr;
UProperty* ByteProperty_TgGame_TgProj_Rocket_FlockIndex = nullptr;
UProperty* BoolProperty_TgGame_TgProj_Rocket_bCurl = nullptr;
UProperty* ObjectProperty_TgGame_TgProjectile_r_Owner = nullptr;
UProperty* FloatProperty_TgGame_TgProjectile_r_fAccelRate = nullptr;
UProperty* FloatProperty_TgGame_TgProjectile_r_fDuration = nullptr;
UProperty* FloatProperty_TgGame_TgProjectile_r_fRange = nullptr;
UProperty* IntProperty_TgGame_TgProjectile_r_nOwnerFireModeId = nullptr;
UProperty* IntProperty_TgGame_TgProjectile_r_nProjectileId = nullptr;
UProperty* StructProperty_TgGame_TgProjectile_r_vSpawnLocation = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc = nullptr;
UProperty* StrProperty_TgGame_TgRepInfo_Beacon_r_nName = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner = nullptr;
UProperty* FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Game_r_MiniMapInfo = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bActiveCombat = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bAllowBuildMorale = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bAllowPlayerRelease = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bDefenseAlarm = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bInOverTime = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTutorialMap = nullptr;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fGameSpeedModifier = nullptr;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fMissionRemainingTime = nullptr;
UProperty* FloatProperty_TgGame_TgRepInfo_Game_r_fServerTimeLastUpdate = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nMaxRoundNumber = nullptr;
UProperty* ByteProperty_TgGame_TgRepInfo_Game_r_nMissionTimerState = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nMissionTimerStateChange = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRaidAttackerRespawnBonus = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRaidDefenderRespawnBonus = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nReleaseDelay = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nRoundNumber = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseAttackers = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nSecsToAutoReleaseDefenders = nullptr;
UProperty* ByteProperty_TgGame_TgRepInfo_Game_r_GameType = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Game_r_MapLogoResIds = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Game_r_Objectives = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsArena = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsMatch = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsMission = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsPVP = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsRaid = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTerritoryMap = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Game_r_bIsTraining = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nAutoKickTimeout = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nPointsToWin = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Game_r_nVictoryBonusLives = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_ApproxLocation = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_CustomCharacterAssembly = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_EquipDeviceInfo = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_MasterPrep = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_PawnOwner = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_Scores = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_Player_r_TaskForce = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_Player_r_bDropped = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_eBonusType = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nCharacterId = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nHealthCurrent = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nHealthMaximum = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nLevel = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nProfileId = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_Player_r_nTitleMsgId = nullptr;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sAgencyName = nullptr;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sAllianceName = nullptr;
UProperty* StrProperty_TgGame_TgRepInfo_Player_r_sOrigPlayerName = nullptr;
UProperty* StructProperty_TgGame_TgRepInfo_Player_r_DeviceStats = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_BeaconManager = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_CurrActiveObjective = nullptr;
UProperty* ObjectProperty_TgGame_TgRepInfo_TaskForce_r_ObjectiveAssignment = nullptr;
UProperty* BoolProperty_TgGame_TgRepInfo_TaskForce_r_bBotOwned = nullptr;
UProperty* ByteProperty_TgGame_TgRepInfo_TaskForce_r_eCoalition = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nCurrentPointCount = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nLeaderCharId = nullptr;
UProperty* FloatProperty_TgGame_TgRepInfo_TaskForce_r_nLookingForMembers = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nNumDeaths = nullptr;
UProperty* ByteProperty_TgGame_TgRepInfo_TaskForce_r_nTaskForce = nullptr;
UProperty* IntProperty_TgGame_TgRepInfo_TaskForce_r_nTeamId = nullptr;
UProperty* FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius = nullptr;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier = nullptr;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce = nullptr;
UProperty* FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce = nullptr;
UProperty* ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget = nullptr;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_Beacon = nullptr;
UProperty* IntProperty_TgGame_TgTeamBeaconManager_r_BeaconDestroyed = nullptr;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconHolder = nullptr;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_BeaconInfo = nullptr;
UProperty* ByteProperty_TgGame_TgTeamBeaconManager_r_BeaconStatus = nullptr;
UProperty* ObjectProperty_TgGame_TgTeamBeaconManager_r_TaskForce = nullptr;
UProperty* ByteProperty_TgGame_TgTimerManager_r_byEventQue = nullptr;
UProperty* ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex = nullptr;
UProperty* FloatProperty_TgGame_TgTimerManager_r_fRemaining = nullptr;
UProperty* FloatProperty_TgGame_TgTimerManager_r_fStartTime = nullptr;


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
	// Regen gap — see declarations above.
	ObjectProperty_TgGame_TgDeployable_r_EffectManager = (UProperty*)ClassPreloader::GetObject("ObjectProperty TgGame.TgDeployable.r_EffectManager");
	FloatProperty_TgGame_TgDeployable_r_fDeployRate = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgDeployable.r_fDeployRate");
	FloatProperty_TgGame_TgDeployable_r_fInitDeployTime = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgDeployable.r_fInitDeployTime");
	FloatProperty_TgGame_TgDeployable_r_fTimeToDeploySecs = (UProperty*)ClassPreloader::GetObject("FloatProperty TgGame.TgDeployable.r_fTimeToDeploySecs");
	ByteProperty_TgGame_TgDeployable_r_nFlashCount = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgDeployable.r_nFlashCount");
	ByteProperty_TgGame_TgDeployable_r_nFlashFireCount = (UProperty*)ClassPreloader::GetObject("ByteProperty TgGame.TgDeployable.r_nFlashFireCount");
	IntProperty_TgGame_TgDeployable_r_nHealth = (UProperty*)ClassPreloader::GetObject("IntProperty TgGame.TgDeployable.r_nHealth");
	StructProperty_TgGame_TgDeployable_r_vFlashLocation = (UProperty*)ClassPreloader::GetObject("StructProperty TgGame.TgDeployable.r_vFlashLocation");
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

	// --- UClass* pointer resolution for class-dispatch ---
	Class_Engine_AIController = (UClass*)ClassPreloader::GetObject("Class Engine.AIController");
	Class_Engine_AccessControl = (UClass*)ClassPreloader::GetObject("Class Engine.AccessControl");
	Class_Engine_Actor = (UClass*)ClassPreloader::GetObject("Class Engine.Actor");
	Class_Engine_Admin = (UClass*)ClassPreloader::GetObject("Class Engine.Admin");
	Class_Engine_AmbientSound = (UClass*)ClassPreloader::GetObject("Class Engine.AmbientSound");
	Class_Engine_AmbientSoundMovable = (UClass*)ClassPreloader::GetObject("Class Engine.AmbientSoundMovable");
	Class_Engine_AmbientSoundNonLoop = (UClass*)ClassPreloader::GetObject("Class Engine.AmbientSoundNonLoop");
	Class_Engine_AmbientSoundSimple = (UClass*)ClassPreloader::GetObject("Class Engine.AmbientSoundSimple");
	Class_Engine_AmbientSoundSimpleToggleable = (UClass*)ClassPreloader::GetObject("Class Engine.AmbientSoundSimpleToggleable");
	Class_Engine_AnimatedCamera = (UClass*)ClassPreloader::GetObject("Class Engine.AnimatedCamera");
	Class_Engine_AutoLadder = (UClass*)ClassPreloader::GetObject("Class Engine.AutoLadder");
	Class_Engine_BlockingVolume = (UClass*)ClassPreloader::GetObject("Class Engine.BlockingVolume");
	Class_Engine_BroadcastHandler = (UClass*)ClassPreloader::GetObject("Class Engine.BroadcastHandler");
	Class_Engine_Brush = (UClass*)ClassPreloader::GetObject("Class Engine.Brush");
	Class_Engine_Camera = (UClass*)ClassPreloader::GetObject("Class Engine.Camera");
	Class_Engine_CameraActor = (UClass*)ClassPreloader::GetObject("Class Engine.CameraActor");
	Class_Engine_ClipMarker = (UClass*)ClassPreloader::GetObject("Class Engine.ClipMarker");
	Class_Engine_ColorScaleVolume = (UClass*)ClassPreloader::GetObject("Class Engine.ColorScaleVolume");
	Class_Engine_Controller = (UClass*)ClassPreloader::GetObject("Class Engine.Controller");
	Class_Engine_CoverGroup = (UClass*)ClassPreloader::GetObject("Class Engine.CoverGroup");
	Class_Engine_CoverLink = (UClass*)ClassPreloader::GetObject("Class Engine.CoverLink");
	Class_Engine_CoverReplicator = (UClass*)ClassPreloader::GetObject("Class Engine.CoverReplicator");
	Class_Engine_CoverSlotMarker = (UClass*)ClassPreloader::GetObject("Class Engine.CoverSlotMarker");
	Class_Engine_CrowdAgent = (UClass*)ClassPreloader::GetObject("Class Engine.CrowdAgent");
	Class_Engine_CrowdAttractor = (UClass*)ClassPreloader::GetObject("Class Engine.CrowdAttractor");
	Class_Engine_CrowdReplicationActor = (UClass*)ClassPreloader::GetObject("Class Engine.CrowdReplicationActor");
	Class_Engine_CullDistanceVolume = (UClass*)ClassPreloader::GetObject("Class Engine.CullDistanceVolume");
	Class_Engine_DebugCameraController = (UClass*)ClassPreloader::GetObject("Class Engine.DebugCameraController");
	Class_Engine_DebugCameraHUD = (UClass*)ClassPreloader::GetObject("Class Engine.DebugCameraHUD");
	Class_Engine_DecalActor = (UClass*)ClassPreloader::GetObject("Class Engine.DecalActor");
	Class_Engine_DecalActorBase = (UClass*)ClassPreloader::GetObject("Class Engine.DecalActorBase");
	Class_Engine_DecalActorMovable = (UClass*)ClassPreloader::GetObject("Class Engine.DecalActorMovable");
	Class_Engine_DecalManager = (UClass*)ClassPreloader::GetObject("Class Engine.DecalManager");
	Class_Engine_DefaultPhysicsVolume = (UClass*)ClassPreloader::GetObject("Class Engine.DefaultPhysicsVolume");
	Class_Engine_DirectionalLight = (UClass*)ClassPreloader::GetObject("Class Engine.DirectionalLight");
	Class_Engine_DirectionalLightToggleable = (UClass*)ClassPreloader::GetObject("Class Engine.DirectionalLightToggleable");
	Class_Engine_DoorMarker = (UClass*)ClassPreloader::GetObject("Class Engine.DoorMarker");
	Class_Engine_DroppedPickup = (UClass*)ClassPreloader::GetObject("Class Engine.DroppedPickup");
	Class_Engine_DynamicAnchor = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicAnchor");
	Class_Engine_DynamicBlockingVolume = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicBlockingVolume");
	Class_Engine_DynamicCameraActor = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicCameraActor");
	Class_Engine_DynamicPhysicsVolume = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicPhysicsVolume");
	Class_Engine_DynamicSMActor = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicSMActor");
	Class_Engine_DynamicSMActor_Spawnable = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicSMActor_Spawnable");
	Class_Engine_DynamicTriggerVolume = (UClass*)ClassPreloader::GetObject("Class Engine.DynamicTriggerVolume");
	Class_Engine_Emitter = (UClass*)ClassPreloader::GetObject("Class Engine.Emitter");
	Class_Engine_EmitterCameraLensEffectBase = (UClass*)ClassPreloader::GetObject("Class Engine.EmitterCameraLensEffectBase");
	Class_Engine_EmitterPool = (UClass*)ClassPreloader::GetObject("Class Engine.EmitterPool");
	Class_Engine_EmitterSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.EmitterSpawnable");
	Class_Engine_FileLog = (UClass*)ClassPreloader::GetObject("Class Engine.FileLog");
	Class_Engine_FileWriter = (UClass*)ClassPreloader::GetObject("Class Engine.FileWriter");
	Class_Engine_FluidInfluenceActor = (UClass*)ClassPreloader::GetObject("Class Engine.FluidInfluenceActor");
	Class_Engine_FluidSurfaceActor = (UClass*)ClassPreloader::GetObject("Class Engine.FluidSurfaceActor");
	Class_Engine_FluidSurfaceActorMovable = (UClass*)ClassPreloader::GetObject("Class Engine.FluidSurfaceActorMovable");
	Class_Engine_FogVolumeConeDensityInfo = (UClass*)ClassPreloader::GetObject("Class Engine.FogVolumeConeDensityInfo");
	Class_Engine_FogVolumeConstantDensityInfo = (UClass*)ClassPreloader::GetObject("Class Engine.FogVolumeConstantDensityInfo");
	Class_Engine_FogVolumeDensityInfo = (UClass*)ClassPreloader::GetObject("Class Engine.FogVolumeDensityInfo");
	Class_Engine_FogVolumeLinearHalfspaceDensityInfo = (UClass*)ClassPreloader::GetObject("Class Engine.FogVolumeLinearHalfspaceDensityInfo");
	Class_Engine_FogVolumeSphericalDensityInfo = (UClass*)ClassPreloader::GetObject("Class Engine.FogVolumeSphericalDensityInfo");
	Class_Engine_FoliageFactory = (UClass*)ClassPreloader::GetObject("Class Engine.FoliageFactory");
	Class_Engine_FractureManager = (UClass*)ClassPreloader::GetObject("Class Engine.FractureManager");
	Class_Engine_FracturedStaticMeshActor = (UClass*)ClassPreloader::GetObject("Class Engine.FracturedStaticMeshActor");
	Class_Engine_FracturedStaticMeshPart = (UClass*)ClassPreloader::GetObject("Class Engine.FracturedStaticMeshPart");
	Class_Engine_GameInfo = (UClass*)ClassPreloader::GetObject("Class Engine.GameInfo");
	Class_Engine_GameReplicationInfo = (UClass*)ClassPreloader::GetObject("Class Engine.GameReplicationInfo");
	Class_Engine_GravityVolume = (UClass*)ClassPreloader::GetObject("Class Engine.GravityVolume");
	Class_Engine_HUD = (UClass*)ClassPreloader::GetObject("Class Engine.HUD");
	Class_Engine_HeightFog = (UClass*)ClassPreloader::GetObject("Class Engine.HeightFog");
	Class_Engine_Info = (UClass*)ClassPreloader::GetObject("Class Engine.Info");
	Class_Engine_InternetInfo = (UClass*)ClassPreloader::GetObject("Class Engine.InternetInfo");
	Class_Engine_InterpActor = (UClass*)ClassPreloader::GetObject("Class Engine.InterpActor");
	Class_Engine_Inventory = (UClass*)ClassPreloader::GetObject("Class Engine.Inventory");
	Class_Engine_InventoryManager = (UClass*)ClassPreloader::GetObject("Class Engine.InventoryManager");
	Class_Engine_KActor = (UClass*)ClassPreloader::GetObject("Class Engine.KActor");
	Class_Engine_KActorSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.KActorSpawnable");
	Class_Engine_KAsset = (UClass*)ClassPreloader::GetObject("Class Engine.KAsset");
	Class_Engine_KAssetSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.KAssetSpawnable");
	Class_Engine_Keypoint = (UClass*)ClassPreloader::GetObject("Class Engine.Keypoint");
	Class_Engine_Ladder = (UClass*)ClassPreloader::GetObject("Class Engine.Ladder");
	Class_Engine_LadderVolume = (UClass*)ClassPreloader::GetObject("Class Engine.LadderVolume");
	Class_Engine_LensFlareSource = (UClass*)ClassPreloader::GetObject("Class Engine.LensFlareSource");
	Class_Engine_LevelStreamingVolume = (UClass*)ClassPreloader::GetObject("Class Engine.LevelStreamingVolume");
	Class_Engine_LiftCenter = (UClass*)ClassPreloader::GetObject("Class Engine.LiftCenter");
	Class_Engine_LiftExit = (UClass*)ClassPreloader::GetObject("Class Engine.LiftExit");
	Class_Engine_Light = (UClass*)ClassPreloader::GetObject("Class Engine.Light");
	Class_Engine_LightVolume = (UClass*)ClassPreloader::GetObject("Class Engine.LightVolume");
	Class_Engine_MantleMarker = (UClass*)ClassPreloader::GetObject("Class Engine.MantleMarker");
	Class_Engine_MaterialInstanceActor = (UClass*)ClassPreloader::GetObject("Class Engine.MaterialInstanceActor");
	Class_Engine_MatineeActor = (UClass*)ClassPreloader::GetObject("Class Engine.MatineeActor");
	Class_Engine_Mutator = (UClass*)ClassPreloader::GetObject("Class Engine.Mutator");
	Class_Engine_NavigationPoint = (UClass*)ClassPreloader::GetObject("Class Engine.NavigationPoint");
	Class_Engine_Note = (UClass*)ClassPreloader::GetObject("Class Engine.Note");
	Class_Engine_NxCylindricalForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxCylindricalForceField");
	Class_Engine_NxCylindricalForceFieldCapsule = (UClass*)ClassPreloader::GetObject("Class Engine.NxCylindricalForceFieldCapsule");
	Class_Engine_NxForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxForceField");
	Class_Engine_NxForceFieldGeneric = (UClass*)ClassPreloader::GetObject("Class Engine.NxForceFieldGeneric");
	Class_Engine_NxForceFieldRadial = (UClass*)ClassPreloader::GetObject("Class Engine.NxForceFieldRadial");
	Class_Engine_NxForceFieldTornado = (UClass*)ClassPreloader::GetObject("Class Engine.NxForceFieldTornado");
	Class_Engine_NxGenericForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxGenericForceField");
	Class_Engine_NxGenericForceFieldBox = (UClass*)ClassPreloader::GetObject("Class Engine.NxGenericForceFieldBox");
	Class_Engine_NxGenericForceFieldBrush = (UClass*)ClassPreloader::GetObject("Class Engine.NxGenericForceFieldBrush");
	Class_Engine_NxGenericForceFieldCapsule = (UClass*)ClassPreloader::GetObject("Class Engine.NxGenericForceFieldCapsule");
	Class_Engine_NxRadialCustomForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxRadialCustomForceField");
	Class_Engine_NxRadialForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxRadialForceField");
	Class_Engine_NxTornadoAngularForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxTornadoAngularForceField");
	Class_Engine_NxTornadoAngularForceFieldCapsule = (UClass*)ClassPreloader::GetObject("Class Engine.NxTornadoAngularForceFieldCapsule");
	Class_Engine_NxTornadoForceField = (UClass*)ClassPreloader::GetObject("Class Engine.NxTornadoForceField");
	Class_Engine_NxTornadoForceFieldCapsule = (UClass*)ClassPreloader::GetObject("Class Engine.NxTornadoForceFieldCapsule");
	Class_Engine_Objective = (UClass*)ClassPreloader::GetObject("Class Engine.Objective");
	Class_Engine_PathBlockingVolume = (UClass*)ClassPreloader::GetObject("Class Engine.PathBlockingVolume");
	Class_Engine_PathNode = (UClass*)ClassPreloader::GetObject("Class Engine.PathNode");
	Class_Engine_PathNode_Dynamic = (UClass*)ClassPreloader::GetObject("Class Engine.PathNode_Dynamic");
	Class_Engine_Pawn = (UClass*)ClassPreloader::GetObject("Class Engine.Pawn");
	Class_Engine_PhysXDestructibleActor = (UClass*)ClassPreloader::GetObject("Class Engine.PhysXDestructibleActor");
	Class_Engine_PhysXDestructiblePart = (UClass*)ClassPreloader::GetObject("Class Engine.PhysXDestructiblePart");
	Class_Engine_PhysXEmitterSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.PhysXEmitterSpawnable");
	Class_Engine_PhysicsVolume = (UClass*)ClassPreloader::GetObject("Class Engine.PhysicsVolume");
	Class_Engine_PickupFactory = (UClass*)ClassPreloader::GetObject("Class Engine.PickupFactory");
	Class_Engine_PlayerController = (UClass*)ClassPreloader::GetObject("Class Engine.PlayerController");
	Class_Engine_PlayerReplicationInfo = (UClass*)ClassPreloader::GetObject("Class Engine.PlayerReplicationInfo");
	Class_Engine_PlayerStart = (UClass*)ClassPreloader::GetObject("Class Engine.PlayerStart");
	Class_Engine_PointLight = (UClass*)ClassPreloader::GetObject("Class Engine.PointLight");
	Class_Engine_PointLightMovable = (UClass*)ClassPreloader::GetObject("Class Engine.PointLightMovable");
	Class_Engine_PointLightToggleable = (UClass*)ClassPreloader::GetObject("Class Engine.PointLightToggleable");
	Class_Engine_PolyMarker = (UClass*)ClassPreloader::GetObject("Class Engine.PolyMarker");
	Class_Engine_PortalMarker = (UClass*)ClassPreloader::GetObject("Class Engine.PortalMarker");
	Class_Engine_PortalTeleporter = (UClass*)ClassPreloader::GetObject("Class Engine.PortalTeleporter");
	Class_Engine_PortalVolume = (UClass*)ClassPreloader::GetObject("Class Engine.PortalVolume");
	Class_Engine_PostProcessVolume = (UClass*)ClassPreloader::GetObject("Class Engine.PostProcessVolume");
	Class_Engine_PotentialClimbWatcher = (UClass*)ClassPreloader::GetObject("Class Engine.PotentialClimbWatcher");
	Class_Engine_PrefabInstance = (UClass*)ClassPreloader::GetObject("Class Engine.PrefabInstance");
	Class_Engine_Projectile = (UClass*)ClassPreloader::GetObject("Class Engine.Projectile");
	Class_Engine_RB_BSJointActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_BSJointActor");
	Class_Engine_RB_ConstraintActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_ConstraintActor");
	Class_Engine_RB_ConstraintActorSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.RB_ConstraintActorSpawnable");
	Class_Engine_RB_CylindricalForceActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_CylindricalForceActor");
	Class_Engine_RB_ForceFieldExcludeVolume = (UClass*)ClassPreloader::GetObject("Class Engine.RB_ForceFieldExcludeVolume");
	Class_Engine_RB_HingeActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_HingeActor");
	Class_Engine_RB_LineImpulseActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_LineImpulseActor");
	Class_Engine_RB_PrismaticActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_PrismaticActor");
	Class_Engine_RB_PulleyJointActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_PulleyJointActor");
	Class_Engine_RB_RadialForceActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_RadialForceActor");
	Class_Engine_RB_RadialImpulseActor = (UClass*)ClassPreloader::GetObject("Class Engine.RB_RadialImpulseActor");
	Class_Engine_RB_Thruster = (UClass*)ClassPreloader::GetObject("Class Engine.RB_Thruster");
	Class_Engine_ReplicationInfo = (UClass*)ClassPreloader::GetObject("Class Engine.ReplicationInfo");
	Class_Engine_ReverbVolume = (UClass*)ClassPreloader::GetObject("Class Engine.ReverbVolume");
	Class_Engine_Route = (UClass*)ClassPreloader::GetObject("Class Engine.Route");
	Class_Engine_SVehicle = (UClass*)ClassPreloader::GetObject("Class Engine.SVehicle");
	Class_Engine_SceneCapture2DActor = (UClass*)ClassPreloader::GetObject("Class Engine.SceneCapture2DActor");
	Class_Engine_SceneCaptureActor = (UClass*)ClassPreloader::GetObject("Class Engine.SceneCaptureActor");
	Class_Engine_SceneCaptureCubeMapActor = (UClass*)ClassPreloader::GetObject("Class Engine.SceneCaptureCubeMapActor");
	Class_Engine_SceneCapturePortalActor = (UClass*)ClassPreloader::GetObject("Class Engine.SceneCapturePortalActor");
	Class_Engine_SceneCaptureReflectActor = (UClass*)ClassPreloader::GetObject("Class Engine.SceneCaptureReflectActor");
	Class_Engine_ScoreBoard = (UClass*)ClassPreloader::GetObject("Class Engine.ScoreBoard");
	Class_Engine_Scout = (UClass*)ClassPreloader::GetObject("Class Engine.Scout");
	Class_Engine_SkeletalMeshActor = (UClass*)ClassPreloader::GetObject("Class Engine.SkeletalMeshActor");
	Class_Engine_SkeletalMeshActorBasedOnExtremeContent = (UClass*)ClassPreloader::GetObject("Class Engine.SkeletalMeshActorBasedOnExtremeContent");
	Class_Engine_SkeletalMeshActorMAT = (UClass*)ClassPreloader::GetObject("Class Engine.SkeletalMeshActorMAT");
	Class_Engine_SkeletalMeshActorMATSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.SkeletalMeshActorMATSpawnable");
	Class_Engine_SkeletalMeshActorSpawnable = (UClass*)ClassPreloader::GetObject("Class Engine.SkeletalMeshActorSpawnable");
	Class_Engine_SkyLight = (UClass*)ClassPreloader::GetObject("Class Engine.SkyLight");
	Class_Engine_SkyLightToggleable = (UClass*)ClassPreloader::GetObject("Class Engine.SkyLightToggleable");
	Class_Engine_SpeedTreeActor = (UClass*)ClassPreloader::GetObject("Class Engine.SpeedTreeActor");
	Class_Engine_SpotLight = (UClass*)ClassPreloader::GetObject("Class Engine.SpotLight");
	Class_Engine_SpotLightMovable = (UClass*)ClassPreloader::GetObject("Class Engine.SpotLightMovable");
	Class_Engine_SpotLightToggleable = (UClass*)ClassPreloader::GetObject("Class Engine.SpotLightToggleable");
	Class_Engine_StaticLightCollectionActor = (UClass*)ClassPreloader::GetObject("Class Engine.StaticLightCollectionActor");
	Class_Engine_StaticMeshActor = (UClass*)ClassPreloader::GetObject("Class Engine.StaticMeshActor");
	Class_Engine_StaticMeshActorBase = (UClass*)ClassPreloader::GetObject("Class Engine.StaticMeshActorBase");
	Class_Engine_StaticMeshActorBasedOnExtremeContent = (UClass*)ClassPreloader::GetObject("Class Engine.StaticMeshActorBasedOnExtremeContent");
	Class_Engine_StaticMeshCollectionActor = (UClass*)ClassPreloader::GetObject("Class Engine.StaticMeshCollectionActor");
	Class_Engine_TargetPoint = (UClass*)ClassPreloader::GetObject("Class Engine.TargetPoint");
	Class_Engine_TeamInfo = (UClass*)ClassPreloader::GetObject("Class Engine.TeamInfo");
	Class_Engine_Teleporter = (UClass*)ClassPreloader::GetObject("Class Engine.Teleporter");
	Class_Engine_Terrain = (UClass*)ClassPreloader::GetObject("Class Engine.Terrain");
	Class_Engine_Trigger = (UClass*)ClassPreloader::GetObject("Class Engine.Trigger");
	Class_Engine_TriggerStreamingLevel = (UClass*)ClassPreloader::GetObject("Class Engine.TriggerStreamingLevel");
	Class_Engine_TriggerVolume = (UClass*)ClassPreloader::GetObject("Class Engine.TriggerVolume");
	Class_Engine_Trigger_Dynamic = (UClass*)ClassPreloader::GetObject("Class Engine.Trigger_Dynamic");
	Class_Engine_Trigger_LOS = (UClass*)ClassPreloader::GetObject("Class Engine.Trigger_LOS");
	Class_Engine_TriggeredPath = (UClass*)ClassPreloader::GetObject("Class Engine.TriggeredPath");
	Class_Engine_Vehicle = (UClass*)ClassPreloader::GetObject("Class Engine.Vehicle");
	Class_Engine_Volume = (UClass*)ClassPreloader::GetObject("Class Engine.Volume");
	Class_Engine_VolumePathNode = (UClass*)ClassPreloader::GetObject("Class Engine.VolumePathNode");
	Class_Engine_VolumeTimer = (UClass*)ClassPreloader::GetObject("Class Engine.VolumeTimer");
	Class_Engine_WaterVolume = (UClass*)ClassPreloader::GetObject("Class Engine.WaterVolume");
	Class_Engine_Weapon = (UClass*)ClassPreloader::GetObject("Class Engine.Weapon");
	Class_Engine_WindDirectionalSource = (UClass*)ClassPreloader::GetObject("Class Engine.WindDirectionalSource");
	Class_Engine_WorldInfo = (UClass*)ClassPreloader::GetObject("Class Engine.WorldInfo");
	Class_Engine_ZoneInfo = (UClass*)ClassPreloader::GetObject("Class Engine.ZoneInfo");
	Class_GameFramework_GameAIController = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameAIController");
	Class_GameFramework_GameExplosionActor = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameExplosionActor");
	Class_GameFramework_GameHUD = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameHUD");
	Class_GameFramework_GamePawn = (UClass*)ClassPreloader::GetObject("Class GameFramework.GamePawn");
	Class_GameFramework_GamePlayerController = (UClass*)ClassPreloader::GetObject("Class GameFramework.GamePlayerController");
	Class_GameFramework_GameProjectile = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameProjectile");
	Class_GameFramework_GameVehicle = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameVehicle");
	Class_GameFramework_GameWeapon = (UClass*)ClassPreloader::GetObject("Class GameFramework.GameWeapon");
	Class_TgGame_TgAIController = (UClass*)ClassPreloader::GetObject("Class TgGame.TgAIController");
	Class_TgGame_TgActionPoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgActionPoint");
	Class_TgGame_TgActorFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgActorFactory");
	Class_TgGame_TgAlarmPoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgAlarmPoint");
	Class_TgGame_TgAnnouncer = (UClass*)ClassPreloader::GetObject("Class TgGame.TgAnnouncer");
	Class_TgGame_TgBaseObjective_CTFBot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBaseObjective_CTFBot");
	Class_TgGame_TgBaseObjective_KOTH = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBaseObjective_KOTH");
	Class_TgGame_TgBeaconFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBeaconFactory");
	Class_TgGame_TgBotEncounterVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBotEncounterVolume");
	Class_TgGame_TgBotFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBotFactory");
	Class_TgGame_TgBotFactorySpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBotFactorySpawnable");
	Class_TgGame_TgBotStart = (UClass*)ClassPreloader::GetObject("Class TgGame.TgBotStart");
	Class_TgGame_TgCharacterBuilderLight = (UClass*)ClassPreloader::GetObject("Class TgGame.TgCharacterBuilderLight");
	Class_TgGame_TgChestActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgChestActor");
	Class_TgGame_TgCollisionProxy = (UClass*)ClassPreloader::GetObject("Class TgGame.TgCollisionProxy");
	Class_TgGame_TgCollisionProxy_Vortex = (UClass*)ClassPreloader::GetObject("Class TgGame.TgCollisionProxy_Vortex");
	Class_TgGame_TgCoverPoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgCoverPoint");
	Class_TgGame_TgDebugCameraController = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDebugCameraController");
	Class_TgGame_TgDecalActor_Logo = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDecalActor_Logo");
	Class_TgGame_TgDefensePoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDefensePoint");
	Class_TgGame_TgDeploy_Artillery = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_Artillery");
	Class_TgGame_TgDeploy_Beacon = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_Beacon");
	Class_TgGame_TgDeploy_BeaconEntrance = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_BeaconEntrance");
	Class_TgGame_TgDeploy_DestructibleCover = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_DestructibleCover");
	Class_TgGame_TgDeploy_ForceField = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_ForceField");
	Class_TgGame_TgDeploy_Sensor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_Sensor");
	Class_TgGame_TgDeploy_SweepSensor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeploy_SweepSensor");
	Class_TgGame_TgDeployable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeployable");
	Class_TgGame_TgDeployableFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeployableFactory");
	Class_TgGame_TgDestructibleFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDestructibleFactory");
	Class_TgGame_TgDevice = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice");
	Class_TgGame_TgDeviceVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeviceVolume");
	Class_TgGame_TgDeviceVolumeInfo = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDeviceVolumeInfo");
	Class_TgGame_TgDevice_Grenade = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_Grenade");
	Class_TgGame_TgDevice_HitPulse = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_HitPulse");
	Class_TgGame_TgDevice_MeleeDualWield = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_MeleeDualWield");
	Class_TgGame_TgDevice_Morale = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_Morale");
	Class_TgGame_TgDevice_NewMelee = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_NewMelee");
	Class_TgGame_TgDevice_NewRange = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDevice_NewRange");
	Class_TgGame_TgDoor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDoor");
	Class_TgGame_TgDoorMarker = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDoorMarker");
	Class_TgGame_TgDroppedItem = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDroppedItem");
	Class_TgGame_TgDummyActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDummyActor");
	Class_TgGame_TgDynamicDestructible = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDynamicDestructible");
	Class_TgGame_TgDynamicSMActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgDynamicSMActor");
	Class_TgGame_TgEffectManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgEffectManager");
	Class_TgGame_TgElevatingVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgElevatingVolume");
	Class_TgGame_TgEmitter = (UClass*)ClassPreloader::GetObject("Class TgGame.TgEmitter");
	Class_TgGame_TgEmitterCrashlanding = (UClass*)ClassPreloader::GetObject("Class TgGame.TgEmitterCrashlanding");
	Class_TgGame_TgEmitterSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgEmitterSpawnable");
	Class_TgGame_TgFlagCaptureVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgFlagCaptureVolume");
	Class_TgGame_TgFracturedStaticMeshActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgFracturedStaticMeshActor");
	Class_TgGame_TgGame = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame");
	Class_TgGame_TgGame_Arena = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Arena");
	Class_TgGame_TgGame_CTF = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_CTF");
	Class_TgGame_TgGame_City = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_City");
	Class_TgGame_TgGame_Control = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Control");
	Class_TgGame_TgGame_Defense = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Defense");
	Class_TgGame_TgGame_DualCTF = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_DualCTF");
	Class_TgGame_TgGame_Escort = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Escort");
	Class_TgGame_TgGame_Mission = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Mission");
	Class_TgGame_TgGame_OpenWorld = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_OpenWorld");
	Class_TgGame_TgGame_OpenWorldPVE = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_OpenWorldPVE");
	Class_TgGame_TgGame_OpenWorldPVP = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_OpenWorldPVP");
	Class_TgGame_TgGame_PointRotation = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_PointRotation");
	Class_TgGame_TgGame_Ticket = (UClass*)ClassPreloader::GetObject("Class TgGame.TgGame_Ticket");
	Class_TgGame_TgHUD = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHUD");
	Class_TgGame_TgHeightFog = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHeightFog");
	Class_TgGame_TgHelpAlertVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHelpAlertVolume");
	Class_TgGame_TgHexItemFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHexItemFactory");
	Class_TgGame_TgHexLandMarkActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHexLandMarkActor");
	Class_TgGame_TgHitDisplayActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHitDisplayActor");
	Class_TgGame_TgHoldSpot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgHoldSpot");
	Class_TgGame_TgInterpActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgInterpActor");
	Class_TgGame_TgInterpolatingCameraActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgInterpolatingCameraActor");
	Class_TgGame_TgInventoryManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgInventoryManager");
	Class_TgGame_TgKActorSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgKActorSpawnable");
	Class_TgGame_TgKAssetSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgKAssetSpawnable");
	Class_TgGame_TgKAsset_ClientSideSim = (UClass*)ClassPreloader::GetObject("Class TgGame.TgKAsset_ClientSideSim");
	Class_TgGame_TgKismetTestActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgKismetTestActor");
	Class_TgGame_TgLevelCamera = (UClass*)ClassPreloader::GetObject("Class TgGame.TgLevelCamera");
	Class_TgGame_TgMeshAssembly = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMeshAssembly");
	Class_TgGame_TgMiniMapActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMiniMapActor");
	Class_TgGame_TgMissionListVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionListVolume");
	Class_TgGame_TgMissionObjective = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionObjective");
	Class_TgGame_TgMissionObjective_Bot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionObjective_Bot");
	Class_TgGame_TgMissionObjective_Escort = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionObjective_Escort");
	Class_TgGame_TgMissionObjective_Kismet = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionObjective_Kismet");
	Class_TgGame_TgMissionObjective_Proximity = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMissionObjective_Proximity");
	Class_TgGame_TgModifyPawnPropertiesVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgModifyPawnPropertiesVolume");
	Class_TgGame_TgMorphFX = (UClass*)ClassPreloader::GetObject("Class TgGame.TgMorphFX");
	Class_TgGame_TgNavRouteIndicator = (UClass*)ClassPreloader::GetObject("Class TgGame.TgNavRouteIndicator");
	Class_TgGame_TgNavigationPoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgNavigationPoint");
	Class_TgGame_TgNavigationPointSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgNavigationPointSpawnable");
	Class_TgGame_TgNewsStand = (UClass*)ClassPreloader::GetObject("Class TgGame.TgNewsStand");
	Class_TgGame_TgObjectiveAssignment = (UClass*)ClassPreloader::GetObject("Class TgGame.TgObjectiveAssignment");
	Class_TgGame_TgObjectiveAttachActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgObjectiveAttachActor");
	Class_TgGame_TgOmegaVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgOmegaVolume");
	Class_TgGame_TgPawn = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn");
	Class_TgGame_TgPawn_AVCompositeWalker = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_AVCompositeWalker");
	Class_TgGame_TgPawn_Ambush = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Ambush");
	Class_TgGame_TgPawn_AndroidMinion = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_AndroidMinion");
	Class_TgGame_TgPawn_AttackTransport = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_AttackTransport");
	Class_TgGame_TgPawn_Boss = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Boss");
	Class_TgGame_TgPawn_Boss_Destroyer = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Boss_Destroyer");
	Class_TgGame_TgPawn_Brawler = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Brawler");
	Class_TgGame_TgPawn_CTR = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_CTR");
	Class_TgGame_TgPawn_Character = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Character");
	Class_TgGame_TgPawn_ColonyEye = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_ColonyEye");
	Class_TgGame_TgPawn_Destructible = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Destructible");
	Class_TgGame_TgPawn_Detonator = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Detonator");
	Class_TgGame_TgPawn_Dismantler = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Dismantler");
	Class_TgGame_TgPawn_DuneCommander = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_DuneCommander");
	Class_TgGame_TgPawn_Elite_Alchemist = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Elite_Alchemist");
	Class_TgGame_TgPawn_Elite_Assassin = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Elite_Assassin");
	Class_TgGame_TgPawn_EscortRobot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_EscortRobot");
	Class_TgGame_TgPawn_FlyingBoss = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_FlyingBoss");
	Class_TgGame_TgPawn_GroundPetA = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_GroundPetA");
	Class_TgGame_TgPawn_Guardian = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Guardian");
	Class_TgGame_TgPawn_Hover = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Hover");
	Class_TgGame_TgPawn_HoverShieldSphere = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_HoverShieldSphere");
	Class_TgGame_TgPawn_Hunter = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Hunter");
	Class_TgGame_TgPawn_Inquisitor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Inquisitor");
	Class_TgGame_TgPawn_Interact_NPC = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Interact_NPC");
	Class_TgGame_TgPawn_Iris = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Iris");
	Class_TgGame_TgPawn_Juggernaut = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Juggernaut");
	Class_TgGame_TgPawn_Marauder = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Marauder");
	Class_TgGame_TgPawn_NPC = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_NPC");
	Class_TgGame_TgPawn_NewWasp = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_NewWasp");
	Class_TgGame_TgPawn_Raptor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Raptor");
	Class_TgGame_TgPawn_Reaper = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Reaper");
	Class_TgGame_TgPawn_RecursiveSpawner = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_RecursiveSpawner");
	Class_TgGame_TgPawn_Remote = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Remote");
	Class_TgGame_TgPawn_Robot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Robot");
	Class_TgGame_TgPawn_Scanner = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Scanner");
	Class_TgGame_TgPawn_ScannerRecursive = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_ScannerRecursive");
	Class_TgGame_TgPawn_Siege = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Siege");
	Class_TgGame_TgPawn_SiegeBarrage = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_SiegeBarrage");
	Class_TgGame_TgPawn_SiegeHover = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_SiegeHover");
	Class_TgGame_TgPawn_SiegeRapidFire = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_SiegeRapidFire");
	Class_TgGame_TgPawn_Sniper = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Sniper");
	Class_TgGame_TgPawn_SonoranCommander = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_SonoranCommander");
	Class_TgGame_TgPawn_SupportForeman = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_SupportForeman");
	Class_TgGame_TgPawn_Switchblade = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Switchblade");
	Class_TgGame_TgPawn_Tentacle = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Tentacle");
	Class_TgGame_TgPawn_ThinkTank = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_ThinkTank");
	Class_TgGame_TgPawn_TreadRobot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TreadRobot");
	Class_TgGame_TgPawn_Turret = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Turret");
	Class_TgGame_TgPawn_TurretAVAFlak = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TurretAVAFlak");
	Class_TgGame_TgPawn_TurretAVARocket = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TurretAVARocket");
	Class_TgGame_TgPawn_TurretFlak = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TurretFlak");
	Class_TgGame_TgPawn_TurretFlame = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TurretFlame");
	Class_TgGame_TgPawn_TurretPlasma = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_TurretPlasma");
	Class_TgGame_TgPawn_UberWalker = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_UberWalker");
	Class_TgGame_TgPawn_Vanguard = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Vanguard");
	Class_TgGame_TgPawn_VanityPet = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_VanityPet");
	Class_TgGame_TgPawn_Vulcan = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Vulcan");
	Class_TgGame_TgPawn_Warlord = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Warlord");
	Class_TgGame_TgPawn_WaspSpawner = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_WaspSpawner");
	Class_TgGame_TgPawn_Widow = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPawn_Widow");
	Class_TgGame_TgPhysAnimTestActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPhysAnimTestActor");
	Class_TgGame_TgPickupFactory = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPickupFactory");
	Class_TgGame_TgPickupFactory_Item = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPickupFactory_Item");
	Class_TgGame_TgPlayerController = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPlayerController");
	Class_TgGame_TgPlayerCountVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPlayerCountVolume");
	Class_TgGame_TgPointOfInterest = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPointOfInterest");
	Class_TgGame_TgPostProcessVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgPostProcessVolume");
	Class_TgGame_TgProj_Bot = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Bot");
	Class_TgGame_TgProj_Bounce = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Bounce");
	Class_TgGame_TgProj_Deployable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Deployable");
	Class_TgGame_TgProj_FreeGrenade = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_FreeGrenade");
	Class_TgGame_TgProj_Grapple = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Grapple");
	Class_TgGame_TgProj_Grenade = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Grenade");
	Class_TgGame_TgProj_Missile = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Missile");
	Class_TgGame_TgProj_Mortar = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Mortar");
	Class_TgGame_TgProj_Net = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Net");
	Class_TgGame_TgProj_Rocket = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Rocket");
	Class_TgGame_TgProj_StraightTeleporter = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_StraightTeleporter");
	Class_TgGame_TgProj_Teleporter = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProj_Teleporter");
	Class_TgGame_TgProjectile = (UClass*)ClassPreloader::GetObject("Class TgGame.TgProjectile");
	Class_TgGame_TgQueuedAnnouncement = (UClass*)ClassPreloader::GetObject("Class TgGame.TgQueuedAnnouncement");
	Class_TgGame_TgRandomSMActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRandomSMActor");
	Class_TgGame_TgRandomSMManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRandomSMManager");
	Class_TgGame_TgReferenceArray = (UClass*)ClassPreloader::GetObject("Class TgGame.TgReferenceArray");
	Class_TgGame_TgRepInfo_Beacon = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_Beacon");
	Class_TgGame_TgRepInfo_Deployable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_Deployable");
	Class_TgGame_TgRepInfo_Game = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_Game");
	Class_TgGame_TgRepInfo_GameOpenWorld = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_GameOpenWorld");
	Class_TgGame_TgRepInfo_Player = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_Player");
	Class_TgGame_TgRepInfo_TaskForce = (UClass*)ClassPreloader::GetObject("Class TgGame.TgRepInfo_TaskForce");
	Class_TgGame_TgScoreboard = (UClass*)ClassPreloader::GetObject("Class TgGame.TgScoreboard");
	Class_TgGame_TgSkeletalMeshActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor");
	Class_TgGame_TgSkeletalMeshActorGenericUIPreview = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActorGenericUIPreview");
	Class_TgGame_TgSkeletalMeshActorNPC = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActorNPC");
	Class_TgGame_TgSkeletalMeshActorNPCVendor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActorNPCVendor");
	Class_TgGame_TgSkeletalMeshActorSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActorSpawnable");
	Class_TgGame_TgSkeletalMeshActor_CharacterBuilder = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor_CharacterBuilder");
	Class_TgGame_TgSkeletalMeshActor_CharacterBuilderSpawnable = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor_CharacterBuilderSpawnable");
	Class_TgGame_TgSkeletalMeshActor_Composite = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor_Composite");
	Class_TgGame_TgSkeletalMeshActor_EquipScreen = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor_EquipScreen");
	Class_TgGame_TgSkeletalMeshActor_MeleePreVis = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkeletalMeshActor_MeleePreVis");
	Class_TgGame_TgSkydiveTarget = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkydiveTarget");
	Class_TgGame_TgSkydivingVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSkydivingVolume");
	Class_TgGame_TgSoundInsulationVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgSoundInsulationVolume");
	Class_TgGame_TgStartPoint = (UClass*)ClassPreloader::GetObject("Class TgGame.TgStartPoint");
	Class_TgGame_TgStartpointPortalNetwork = (UClass*)ClassPreloader::GetObject("Class TgGame.TgStartpointPortalNetwork");
	Class_TgGame_TgStaticMeshActor = (UClass*)ClassPreloader::GetObject("Class TgGame.TgStaticMeshActor");
	Class_TgGame_TgStaticMeshActor_Logo = (UClass*)ClassPreloader::GetObject("Class TgGame.TgStaticMeshActor_Logo");
	Class_TgGame_TgTeamBeaconManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeamBeaconManager");
	Class_TgGame_TgTeamBlocker = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeamBlocker");
	Class_TgGame_TgTeamMarker = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeamMarker");
	Class_TgGame_TgTeamPlayerStart = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeamPlayerStart");
	Class_TgGame_TgTeamScoreboard = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeamScoreboard");
	Class_TgGame_TgTeleportPlayerVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeleportPlayerVolume");
	Class_TgGame_TgTeleporter = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTeleporter");
	Class_TgGame_TgTimerManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTimerManager");
	Class_TgGame_TgTrigger_Instance = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTrigger_Instance");
	Class_TgGame_TgTrigger_Use = (UClass*)ClassPreloader::GetObject("Class TgGame.TgTrigger_Use");
	Class_TgGame_TgVolumePathNode = (UClass*)ClassPreloader::GetObject("Class TgGame.TgVolumePathNode");
	Class_TgGame_TgWaterVolume = (UClass*)ClassPreloader::GetObject("Class TgGame.TgWaterVolume");
	Class_TgGame_TgWindManager = (UClass*)ClassPreloader::GetObject("Class TgGame.TgWindManager");
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
	// Class-identity dispatch: pointer compare against pre-resolved UClass* values
	// (see declarations above + ResolveRepListProperties). Replaces strcmp(GetFullName).
	UClass* cls = actor->Class;
	// Captured once per call; consumed by every DO_REP / DO_REP_ARRAY in this function.
	// True the first time the rep list ever sees this actor, false thereafter.
	const bool isInitialRep = (g_RepListInitialDoneActors.find(actor) == g_RepListInitialDoneActors.end());
	if (
		cls == Class_Engine_Actor

		|| cls == Class_Engine_WorldInfo

		|| cls == Class_Engine_Vehicle
		|| cls == Class_Engine_SVehicle
		|| cls == Class_Engine_Teleporter
		|| cls == Class_Engine_RB_RadialImpulseActor
		|| cls == Class_Engine_RB_RadialForceActor
		|| cls == Class_Engine_RB_LineImpulseActor
		|| cls == Class_Engine_RB_CylindricalForceActor
		|| cls == Class_Engine_PostProcessVolume

		|| cls == Class_Engine_TeamInfo
		|| cls == Class_Engine_GameReplicationInfo
		|| cls == Class_Engine_PlayerReplicationInfo
		|| cls == Class_Engine_PlayerController
		|| cls == Class_Engine_PickupFactory
		|| cls == Class_Engine_PhysXEmitterSpawnable
		|| cls == Class_Engine_NxForceField
		|| cls == Class_Engine_MatineeActor
		|| cls == Class_Engine_Light
		|| cls == Class_Engine_LensFlareSource
		|| cls == Class_Engine_KAsset
		|| cls == Class_Engine_KActor
		|| cls == Class_Engine_InventoryManager
		|| cls == Class_Engine_Inventory
		|| cls == Class_Engine_HeightFog
		|| cls == Class_Engine_FogVolumeDensityInfo
		|| cls == Class_Engine_FluidInfluenceActor
		|| cls == Class_Engine_EmitterSpawnable
		|| cls == Class_Engine_Emitter
		|| cls == Class_Engine_DroppedPickup
		|| cls == Class_Engine_CrowdReplicationActor
		|| cls == Class_Engine_CrowdAttractor
		|| cls == Class_Engine_Controller
		|| cls == Class_Engine_CameraActor
		|| cls == Class_Engine_AmbientSoundSimpleToggleable

		|| cls == Class_Engine_DynamicSMActor
		|| cls == Class_TgGame_TgDynamicSMActor
		|| cls == Class_TgGame_TgObjectiveAttachActor
		|| cls == Class_TgGame_TgInterpActor
		|| cls == Class_TgGame_TgHexLandMarkActor
		|| cls == Class_TgGame_TgFracturedStaticMeshActor
		|| cls == Class_TgGame_TgDynamicDestructible


		|| cls == Class_TgGame_TgFlagCaptureVolume

		|| cls == Class_TgGame_TgEmitter

		|| cls == Class_TgGame_TgStaticMeshActor

		|| cls == Class_TgGame_TgRandomSMActor
		|| cls == Class_TgGame_TgRandomSMManager

		|| cls == Class_TgGame_TgEffectManager

		|| cls == Class_TgGame_TgInventoryManager

		|| cls == Class_TgGame_TgDroppedItem

		|| cls == Class_TgGame_TgChestActor

		|| cls == Class_TgGame_TgDevice
		|| cls == Class_TgGame_TgDevice_Grenade
		|| cls == Class_TgGame_TgDevice_HitPulse
		|| cls == Class_TgGame_TgDevice_Morale
		|| cls == Class_TgGame_TgDevice_NewMelee
		|| cls == Class_TgGame_TgDevice_MeleeDualWield
		|| cls == Class_TgGame_TgDevice_NewRange

		|| cls == Class_TgGame_TgDeployable
		|| cls == Class_TgGame_TgDeploy_Beacon
		|| cls == Class_TgGame_TgDeploy_BeaconEntrance
		|| cls == Class_TgGame_TgDeploy_DestructibleCover

		|| cls == Class_TgGame_TgMissionObjective
		|| cls == Class_TgGame_TgMissionObjective_Bot
		|| cls == Class_TgGame_TgMissionObjective_Escort
		|| cls == Class_TgGame_TgMissionObjective_Proximity

		|| cls == Class_Engine_Pawn
		|| cls == Class_TgGame_TgPawn
		|| cls == Class_TgGame_TgPawn_AVCompositeWalker
		|| cls == Class_TgGame_TgPawn_Ambush
		|| cls == Class_TgGame_TgPawn_AndroidMinion
		|| cls == Class_TgGame_TgPawn_AttackTransport
		|| cls == Class_TgGame_TgPawn_Boss
		|| cls == Class_TgGame_TgPawn_Boss_Destroyer
		|| cls == Class_TgGame_TgPawn_Brawler
		|| cls == Class_TgGame_TgPawn_CTR
		|| cls == Class_TgGame_TgPawn_Character
		|| cls == Class_TgGame_TgPawn_ColonyEye
		|| cls == Class_TgGame_TgPawn_Destructible
		|| cls == Class_TgGame_TgPawn_Detonator
		|| cls == Class_TgGame_TgPawn_Dismantler
		|| cls == Class_TgGame_TgPawn_DuneCommander
		|| cls == Class_TgGame_TgPawn_Elite_Alchemist
		|| cls == Class_TgGame_TgPawn_Elite_Assassin
		|| cls == Class_TgGame_TgPawn_EscortRobot
		|| cls == Class_TgGame_TgPawn_FlyingBoss
		|| cls == Class_TgGame_TgPawn_GroundPetA
		|| cls == Class_TgGame_TgPawn_Guardian
		|| cls == Class_TgGame_TgPawn_Hover
		|| cls == Class_TgGame_TgPawn_HoverShieldSphere
		|| cls == Class_TgGame_TgPawn_Hunter
		|| cls == Class_TgGame_TgPawn_Inquisitor
		|| cls == Class_TgGame_TgPawn_Interact_NPC
		|| cls == Class_TgGame_TgPawn_Iris
		|| cls == Class_TgGame_TgPawn_Juggernaut
		|| cls == Class_TgGame_TgPawn_Marauder
		|| cls == Class_TgGame_TgPawn_NPC
		|| cls == Class_TgGame_TgPawn_NewWasp
		|| cls == Class_TgGame_TgPawn_Raptor
		|| cls == Class_TgGame_TgPawn_Reaper
		|| cls == Class_TgGame_TgPawn_RecursiveSpawner
		|| cls == Class_TgGame_TgPawn_Remote
		|| cls == Class_TgGame_TgPawn_Robot
		|| cls == Class_TgGame_TgPawn_Scanner
		|| cls == Class_TgGame_TgPawn_ScannerRecursive
		|| cls == Class_TgGame_TgPawn_Siege
		|| cls == Class_TgGame_TgPawn_SiegeBarrage
		|| cls == Class_TgGame_TgPawn_SiegeHover
		|| cls == Class_TgGame_TgPawn_SiegeRapidFire
		|| cls == Class_TgGame_TgPawn_Sniper
		|| cls == Class_TgGame_TgPawn_SonoranCommander
		|| cls == Class_TgGame_TgPawn_SupportForeman
		|| cls == Class_TgGame_TgPawn_Switchblade
		|| cls == Class_TgGame_TgPawn_Tentacle
		|| cls == Class_TgGame_TgPawn_ThinkTank
		|| cls == Class_TgGame_TgPawn_TreadRobot
		|| cls == Class_TgGame_TgPawn_Turret
		|| cls == Class_TgGame_TgPawn_TurretAVAFlak
		|| cls == Class_TgGame_TgPawn_TurretAVARocket
		|| cls == Class_TgGame_TgPawn_TurretFlak
		|| cls == Class_TgGame_TgPawn_TurretFlame
		|| cls == Class_TgGame_TgPawn_TurretPlasma
		|| cls == Class_TgGame_TgPawn_UberWalker
		|| cls == Class_TgGame_TgPawn_Vanguard
		|| cls == Class_TgGame_TgPawn_VanityPet
		|| cls == Class_TgGame_TgPawn_Vulcan
		|| cls == Class_TgGame_TgPawn_Warlord
		|| cls == Class_TgGame_TgPawn_WaspSpawner
		|| cls == Class_TgGame_TgPawn_Widow
		
		|| cls == Class_TgGame_TgObjectiveAssignment

		|| cls == Class_TgGame_TgRepInfo_Deployable
		|| cls == Class_TgGame_TgRepInfo_Beacon

		|| cls == Class_TgGame_TgRepInfo_Game
		|| cls == Class_TgGame_TgRepInfo_GameOpenWorld

		|| cls == Class_TgGame_TgRepInfo_Player
		|| cls == Class_TgGame_TgRepInfo_TaskForce

		|| cls == Class_Engine_KAsset

		|| cls == Class_Engine_StaticMeshActor

		|| cls == Class_Engine_Projectile
		|| cls == Class_TgGame_TgProjectile
		|| cls == Class_TgGame_TgProj_Teleporter
		|| cls == Class_TgGame_TgProj_StraightTeleporter
		|| cls == Class_TgGame_TgProj_Rocket
		|| cls == Class_TgGame_TgProj_Net
		|| cls == Class_TgGame_TgProj_Mortar
		|| cls == Class_TgGame_TgProj_Missile
		|| cls == Class_TgGame_TgProj_Grenade
		|| cls == Class_TgGame_TgProj_Grapple
		|| cls == Class_TgGame_TgProj_FreeGrenade
		|| cls == Class_TgGame_TgProj_Deployable
		|| cls == Class_TgGame_TgProj_Bounce
		|| cls == Class_TgGame_TgProj_Bot

		|| cls == Class_TgGame_TgDoorMarker
		|| cls == Class_TgGame_TgDoor


		|| cls == Class_Engine_SkeletalMeshActor
		|| cls == Class_TgGame_TgSkeletalMeshActorGenericUIPreview
		|| cls == Class_TgGame_TgSkeletalMeshActorNPC
		|| cls == Class_TgGame_TgSkeletalMeshActorNPCVendor
		|| cls == Class_TgGame_TgSkeletalMeshActorSpawnable
		|| cls == Class_TgGame_TgSkeletalMeshActor_CharacterBuilder
		|| cls == Class_TgGame_TgSkeletalMeshActor_CharacterBuilderSpawnable
		|| cls == Class_TgGame_TgSkeletalMeshActor_Composite
		|| cls == Class_TgGame_TgSkeletalMeshActor_EquipScreen
		|| cls == Class_TgGame_TgSkeletalMeshActor_MeleePreVis
		|| cls == Class_TgGame_TgSkeletalMeshActor

		|| cls == Class_TgGame_TgSkydiveTarget

		|| cls == Class_TgGame_TgSkydivingVolume

		|| cls == Class_TgGame_TgTeamBeaconManager

		|| cls == Class_TgGame_TgTimerManager
		
		|| cls == Class_TgGame_TgLevelCamera

		|| cls == Class_TgGame_TgKismetTestActor

		|| cls == Class_Engine_AIController
		|| cls == Class_Engine_Admin
		|| cls == Class_Engine_DebugCameraController
		|| cls == Class_Engine_DynamicCameraActor
		|| cls == Class_Engine_AnimatedCamera
		|| cls == Class_Engine_EmitterCameraLensEffectBase
		|| cls == Class_Engine_EmitterPool
		|| cls == Class_Engine_FogVolumeConeDensityInfo
		|| cls == Class_Engine_FogVolumeConstantDensityInfo
		|| cls == Class_Engine_FogVolumeLinearHalfspaceDensityInfo
		|| cls == Class_Engine_FogVolumeSphericalDensityInfo
		|| cls == Class_Engine_DynamicSMActor_Spawnable
		|| cls == Class_Engine_InterpActor
		|| cls == Class_Engine_KActorSpawnable
		|| cls == Class_Engine_KAssetSpawnable
		|| cls == Class_Engine_Scout
		|| cls == Class_Engine_DirectionalLight
		|| cls == Class_Engine_DirectionalLightToggleable
		|| cls == Class_Engine_PointLight
		|| cls == Class_Engine_PointLightMovable
		|| cls == Class_Engine_PointLightToggleable
		|| cls == Class_Engine_SkyLight
		|| cls == Class_Engine_SkyLightToggleable
		|| cls == Class_Engine_SpotLight
		|| cls == Class_Engine_SpotLightMovable
		|| cls == Class_Engine_SpotLightToggleable
		|| cls == Class_Engine_StaticLightCollectionActor
		|| cls == Class_Engine_NxCylindricalForceField
		|| cls == Class_Engine_NxCylindricalForceFieldCapsule
		|| cls == Class_Engine_NxForceFieldGeneric
		|| cls == Class_Engine_NxForceFieldRadial
		|| cls == Class_Engine_NxForceFieldTornado
		|| cls == Class_Engine_NxGenericForceField
		|| cls == Class_Engine_NxGenericForceFieldBox
		|| cls == Class_Engine_NxGenericForceFieldCapsule
		|| cls == Class_Engine_NxRadialCustomForceField
		|| cls == Class_Engine_NxRadialForceField
		|| cls == Class_Engine_NxTornadoAngularForceField
		|| cls == Class_Engine_NxTornadoAngularForceFieldCapsule
		|| cls == Class_Engine_NxTornadoForceField
		|| cls == Class_Engine_NxTornadoForceFieldCapsule
		|| cls == Class_Engine_Admin
		|| cls == Class_Engine_SkeletalMeshActorBasedOnExtremeContent
		|| cls == Class_Engine_SkeletalMeshActorMAT
		|| cls == Class_Engine_SkeletalMeshActorMATSpawnable
		|| cls == Class_Engine_SkeletalMeshActorSpawnable
		|| cls == Class_Engine_Weapon
		|| cls == Class_GameFramework_GameAIController
		|| cls == Class_GameFramework_GamePawn
		|| cls == Class_GameFramework_GamePlayerController
		|| cls == Class_GameFramework_GameProjectile
		|| cls == Class_GameFramework_GameVehicle
		|| cls == Class_GameFramework_GameWeapon
		|| cls == Class_TgGame_TgAIController
		|| cls == Class_TgGame_TgBaseObjective_CTFBot
		|| cls == Class_TgGame_TgBaseObjective_KOTH
		|| cls == Class_TgGame_TgMissionObjective_Kismet
		|| cls == Class_TgGame_TgDeploy_Artillery
		|| cls == Class_TgGame_TgDeploy_ForceField
		|| cls == Class_TgGame_TgDeploy_Sensor
		|| cls == Class_TgGame_TgDeploy_SweepSensor
		|| cls == Class_TgGame_TgDebugCameraController
		|| cls == Class_TgGame_TgEmitterSpawnable
		|| cls == Class_TgGame_TgEmitterCrashlanding
		|| cls == Class_TgGame_TgInterpolatingCameraActor
		|| cls == Class_TgGame_TgTeleporter
		|| cls == Class_TgGame_TgPostProcessVolume
		|| cls == Class_TgGame_TgPickupFactory
		|| cls == Class_TgGame_TgPickupFactory_Item
		|| cls == Class_TgGame_TgKActorSpawnable
		|| cls == Class_TgGame_TgKAssetSpawnable
		|| cls == Class_TgGame_TgKAsset_ClientSideSim
		|| cls == Class_TgGame_TgNavRouteIndicator
		|| cls == Class_TgGame_TgCharacterBuilderLight
		|| cls == Class_Engine_AccessControl
		|| cls == Class_Engine_AmbientSound
		|| cls == Class_Engine_AmbientSoundMovable
		|| cls == Class_Engine_AmbientSoundNonLoop
		|| cls == Class_Engine_AmbientSoundSimple
		|| cls == Class_Engine_AutoLadder
		|| cls == Class_Engine_BlockingVolume
		|| cls == Class_Engine_BroadcastHandler
		|| cls == Class_Engine_Brush
		|| cls == Class_Engine_Camera
		|| cls == Class_Engine_ClipMarker
		|| cls == Class_Engine_ColorScaleVolume
		|| cls == Class_Engine_CoverGroup
		|| cls == Class_Engine_CoverLink
		|| cls == Class_Engine_CoverReplicator
		|| cls == Class_Engine_CoverSlotMarker
		|| cls == Class_Engine_CrowdAgent
		|| cls == Class_Engine_CullDistanceVolume
		|| cls == Class_Engine_DebugCameraHUD
		|| cls == Class_Engine_DecalActor
		|| cls == Class_Engine_DecalActorBase
		|| cls == Class_Engine_DecalActorMovable
		|| cls == Class_Engine_DecalManager
		|| cls == Class_Engine_DefaultPhysicsVolume
		|| cls == Class_Engine_DoorMarker
		|| cls == Class_Engine_DynamicAnchor
		|| cls == Class_Engine_DynamicBlockingVolume
		|| cls == Class_Engine_DynamicPhysicsVolume
		|| cls == Class_Engine_DynamicTriggerVolume
		|| cls == Class_Engine_FileLog
		|| cls == Class_Engine_FileWriter
		|| cls == Class_Engine_FluidSurfaceActor
		|| cls == Class_Engine_FluidSurfaceActorMovable
		|| cls == Class_Engine_FoliageFactory
		|| cls == Class_Engine_FractureManager
		|| cls == Class_Engine_FracturedStaticMeshActor
		|| cls == Class_Engine_FracturedStaticMeshPart
		|| cls == Class_Engine_GameInfo
		|| cls == Class_Engine_GravityVolume
		|| cls == Class_Engine_HUD
		|| cls == Class_Engine_Info
		|| cls == Class_Engine_InternetInfo
		|| cls == Class_Engine_Keypoint
		|| cls == Class_Engine_Ladder
		|| cls == Class_Engine_LadderVolume
		|| cls == Class_Engine_LevelStreamingVolume
		|| cls == Class_Engine_LiftCenter
		|| cls == Class_Engine_LiftExit
		|| cls == Class_Engine_LightVolume
		|| cls == Class_Engine_MantleMarker
		|| cls == Class_Engine_MaterialInstanceActor
		|| cls == Class_Engine_Mutator
		|| cls == Class_Engine_NavigationPoint
		|| cls == Class_Engine_Note
		|| cls == Class_Engine_NxGenericForceFieldBrush
		|| cls == Class_Engine_Objective
		|| cls == Class_Engine_PathBlockingVolume
		|| cls == Class_Engine_PathNode
		|| cls == Class_Engine_PathNode_Dynamic
		|| cls == Class_Engine_PhysXDestructibleActor
		|| cls == Class_Engine_PhysXDestructiblePart
		|| cls == Class_Engine_PhysicsVolume
		|| cls == Class_Engine_PlayerStart
		|| cls == Class_Engine_PolyMarker
		|| cls == Class_Engine_PortalMarker
		|| cls == Class_Engine_PortalTeleporter
		|| cls == Class_Engine_PortalVolume
		|| cls == Class_Engine_PotentialClimbWatcher
		|| cls == Class_Engine_PrefabInstance
		|| cls == Class_Engine_RB_BSJointActor
		|| cls == Class_Engine_RB_ConstraintActor
		|| cls == Class_Engine_RB_ConstraintActorSpawnable
		|| cls == Class_Engine_RB_ForceFieldExcludeVolume
		|| cls == Class_Engine_RB_HingeActor
		|| cls == Class_Engine_RB_PrismaticActor
		|| cls == Class_Engine_RB_PulleyJointActor
		|| cls == Class_Engine_RB_Thruster
		|| cls == Class_Engine_ReplicationInfo
		|| cls == Class_Engine_ReverbVolume
		|| cls == Class_Engine_Route
		|| cls == Class_Engine_SceneCapture2DActor
		|| cls == Class_Engine_SceneCaptureActor
		|| cls == Class_Engine_SceneCaptureCubeMapActor
		|| cls == Class_Engine_SceneCapturePortalActor
		|| cls == Class_Engine_SceneCaptureReflectActor
		|| cls == Class_Engine_ScoreBoard
		|| cls == Class_Engine_SpeedTreeActor
		|| cls == Class_Engine_StaticMeshActorBase
		|| cls == Class_Engine_StaticMeshActorBasedOnExtremeContent
		|| cls == Class_Engine_StaticMeshCollectionActor
		|| cls == Class_Engine_TargetPoint
		|| cls == Class_Engine_Terrain
		|| cls == Class_Engine_Trigger
		|| cls == Class_Engine_TriggerStreamingLevel
		|| cls == Class_Engine_TriggerVolume
		|| cls == Class_Engine_Trigger_Dynamic
		|| cls == Class_Engine_Trigger_LOS
		|| cls == Class_Engine_TriggeredPath
		|| cls == Class_Engine_Volume
		|| cls == Class_Engine_VolumePathNode
		|| cls == Class_Engine_VolumeTimer
		|| cls == Class_Engine_WaterVolume
		|| cls == Class_Engine_WindDirectionalSource
		|| cls == Class_Engine_ZoneInfo
		|| cls == Class_GameFramework_GameExplosionActor
		|| cls == Class_GameFramework_GameHUD
		|| cls == Class_TgGame_TgActionPoint
		|| cls == Class_TgGame_TgActorFactory
		|| cls == Class_TgGame_TgAlarmPoint
		|| cls == Class_TgGame_TgAnnouncer
		|| cls == Class_TgGame_TgBeaconFactory
		|| cls == Class_TgGame_TgBotEncounterVolume
		|| cls == Class_TgGame_TgBotFactory
		|| cls == Class_TgGame_TgBotFactorySpawnable
		|| cls == Class_TgGame_TgBotStart
		|| cls == Class_TgGame_TgCollisionProxy
		|| cls == Class_TgGame_TgCollisionProxy_Vortex
		|| cls == Class_TgGame_TgCoverPoint
		|| cls == Class_TgGame_TgDecalActor_Logo
		|| cls == Class_TgGame_TgDefensePoint
		|| cls == Class_TgGame_TgDeployableFactory
		|| cls == Class_TgGame_TgDestructibleFactory
		|| cls == Class_TgGame_TgDeviceVolume
		|| cls == Class_TgGame_TgDeviceVolumeInfo
		|| cls == Class_TgGame_TgDummyActor
		|| cls == Class_TgGame_TgElevatingVolume
		|| cls == Class_TgGame_TgGame
		|| cls == Class_TgGame_TgGame_Arena
		|| cls == Class_TgGame_TgGame_CTF
		|| cls == Class_TgGame_TgGame_City
		|| cls == Class_TgGame_TgGame_Control
		|| cls == Class_TgGame_TgGame_Defense
		|| cls == Class_TgGame_TgGame_DualCTF
		|| cls == Class_TgGame_TgGame_Escort
		|| cls == Class_TgGame_TgGame_Mission
		|| cls == Class_TgGame_TgGame_OpenWorld
		|| cls == Class_TgGame_TgGame_OpenWorldPVE
		|| cls == Class_TgGame_TgGame_OpenWorldPVP
		|| cls == Class_TgGame_TgGame_PointRotation
		|| cls == Class_TgGame_TgGame_Ticket
		|| cls == Class_TgGame_TgHUD
		|| cls == Class_TgGame_TgHeightFog
		|| cls == Class_TgGame_TgHelpAlertVolume
		|| cls == Class_TgGame_TgHexItemFactory
		|| cls == Class_TgGame_TgHitDisplayActor
		|| cls == Class_TgGame_TgHoldSpot
		|| cls == Class_TgGame_TgMeshAssembly
		|| cls == Class_TgGame_TgMiniMapActor
		|| cls == Class_TgGame_TgMissionListVolume
		|| cls == Class_TgGame_TgModifyPawnPropertiesVolume
		|| cls == Class_TgGame_TgMorphFX
		|| cls == Class_TgGame_TgNavigationPoint
		|| cls == Class_TgGame_TgNavigationPointSpawnable
		|| cls == Class_TgGame_TgNewsStand
		|| cls == Class_TgGame_TgOmegaVolume
		|| cls == Class_TgGame_TgPhysAnimTestActor
		|| cls == Class_TgGame_TgPlayerController
		|| cls == Class_TgGame_TgPlayerCountVolume
		|| cls == Class_TgGame_TgPointOfInterest
		|| cls == Class_TgGame_TgQueuedAnnouncement
		|| cls == Class_TgGame_TgReferenceArray
		|| cls == Class_TgGame_TgScoreboard
		|| cls == Class_TgGame_TgSoundInsulationVolume
		|| cls == Class_TgGame_TgStartPoint
		|| cls == Class_TgGame_TgStartpointPortalNetwork
		|| cls == Class_TgGame_TgStaticMeshActor_Logo
		|| cls == Class_TgGame_TgTeamBlocker
		|| cls == Class_TgGame_TgTeamMarker
		|| cls == Class_TgGame_TgTeamPlayerStart
		|| cls == Class_TgGame_TgTeamScoreboard
		|| cls == Class_TgGame_TgTeleportPlayerVolume
		|| cls == Class_TgGame_TgTrigger_Instance
		|| cls == Class_TgGame_TgTrigger_Use
		|| cls == Class_TgGame_TgVolumePathNode
		|| cls == Class_TgGame_TgWaterVolume
		|| cls == Class_TgGame_TgWindManager
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
	if (cls == Class_Engine_AmbientSoundSimpleToggleable) {
		if (actor->Role == 3) {
			DO_REP(AAmbientSoundSimpleToggleable, bCurrentlyPlaying, BoolProperty_Engine_AmbientSoundSimpleToggleable_bCurrentlyPlaying);
		}
	}
	if (
		cls == Class_Engine_CameraActor
		|| cls == Class_Engine_DynamicCameraActor
		|| cls == Class_TgGame_TgInterpolatingCameraActor
		|| cls == Class_TgGame_TgLevelCamera
	) {
		if (actor->Role == 3) {
			DO_REP(ACameraActor, AspectRatio, FloatProperty_Engine_CameraActor_AspectRatio);
			DO_REP(ACameraActor, FOVAngle, FloatProperty_Engine_CameraActor_FOVAngle);
		}
	}
	if (
		cls == Class_Engine_Controller
		|| cls == Class_Engine_AIController
		|| cls == Class_Engine_Admin
		|| cls == Class_Engine_DebugCameraController
		|| cls == Class_Engine_PlayerController
		|| cls == Class_GameFramework_GameAIController
		|| cls == Class_GameFramework_GamePlayerController
		|| cls == Class_TgGame_TgAIController
		|| cls == Class_TgGame_TgDebugCameraController
		|| cls == Class_TgGame_TgPlayerController
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AController, Pawn, ObjectProperty_Engine_Controller_Pawn);
			DO_REP(AController, PlayerReplicationInfo, ObjectProperty_Engine_Controller_PlayerReplicationInfo);
		}
	}
	if (cls == Class_Engine_CrowdAttractor) {
		if (actor->bNoDelete) {
			DO_REP(ACrowdAttractor, bAttractorEnabled, BoolProperty_Engine_CrowdAttractor_bAttractorEnabled);
		}
	}
	if (cls == Class_Engine_CrowdReplicationActor) {
		if (actor->Role == 3) {
			DO_REP(ACrowdReplicationActor, DestroyAllCount, IntProperty_Engine_CrowdReplicationActor_DestroyAllCount);
			DO_REP(ACrowdReplicationActor, Spawner, ObjectProperty_Engine_CrowdReplicationActor_Spawner);
			DO_REP(ACrowdReplicationActor, bSpawningActive, BoolProperty_Engine_CrowdReplicationActor_bSpawningActive);
		}
	}
	if (cls == Class_Engine_DroppedPickup) {
		if (actor->Role == 3) {
			DO_REP(ADroppedPickup, InventoryClass, ClassProperty_Engine_DroppedPickup_InventoryClass);
			DO_REP(ADroppedPickup, bFadeOut, BoolProperty_Engine_DroppedPickup_bFadeOut);
		}
	}
	if (
		cls == Class_Engine_DynamicSMActor
		|| cls == Class_Engine_DynamicSMActor_Spawnable
		|| cls == Class_Engine_InterpActor
		|| cls == Class_Engine_KActor
		|| cls == Class_Engine_KActorSpawnable
		|| cls == Class_TgGame_TgDoor
		|| cls == Class_TgGame_TgDynamicDestructible
		|| cls == Class_TgGame_TgDynamicSMActor
		|| cls == Class_TgGame_TgInterpActor
		|| cls == Class_TgGame_TgKActorSpawnable
		|| cls == Class_TgGame_TgKismetTestActor
		|| cls == Class_TgGame_TgObjectiveAttachActor
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
		cls == Class_Engine_Emitter
		|| cls == Class_Engine_EmitterCameraLensEffectBase
		|| cls == Class_Engine_EmitterSpawnable
		|| cls == Class_Engine_PhysXEmitterSpawnable
		|| cls == Class_TgGame_TgEmitter
		|| cls == Class_TgGame_TgEmitterCrashlanding
		|| cls == Class_TgGame_TgEmitterSpawnable
	) {
		if (actor->bNoDelete) {
			DO_REP(AEmitter, bCurrentlyActive, BoolProperty_Engine_Emitter_bCurrentlyActive);
		}
	}
	if (cls == Class_Engine_EmitterSpawnable) {
		if (actor->bNetInitial) {
			DO_REP(AEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_EmitterSpawnable_ParticleTemplate);
		}
	}
	if (cls == Class_Engine_FluidInfluenceActor) {
		if (actor->bNetDirty) {
			DO_REP(AFluidInfluenceActor, bActive, BoolProperty_Engine_FluidInfluenceActor_bActive);
			DO_REP(AFluidInfluenceActor, bToggled, BoolProperty_Engine_FluidInfluenceActor_bToggled);
		}
	}
	if (
		cls == Class_Engine_FogVolumeDensityInfo
		|| cls == Class_Engine_FogVolumeConeDensityInfo
		|| cls == Class_Engine_FogVolumeConstantDensityInfo
		|| cls == Class_Engine_FogVolumeLinearHalfspaceDensityInfo
		|| cls == Class_Engine_FogVolumeSphericalDensityInfo
	) {
		if (actor->Role == 3) {
			DO_REP(AFogVolumeDensityInfo, bEnabled, BoolProperty_Engine_FogVolumeDensityInfo_bEnabled);
		}
	}
	if (
		cls == Class_Engine_GameReplicationInfo
		|| cls == Class_TgGame_TgRepInfo_Game
		|| cls == Class_TgGame_TgRepInfo_GameOpenWorld
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
	if (cls == Class_Engine_HeightFog) {
		if (actor->Role == 3) {
			DO_REP(AHeightFog, bEnabled, BoolProperty_Engine_HeightFog_bEnabled);
		}
	}
	if (
		cls == Class_Engine_Inventory
		|| cls == Class_Engine_Weapon
		|| cls == Class_GameFramework_GameWeapon
		|| cls == Class_TgGame_TgDevice
		|| cls == Class_TgGame_TgDevice_Grenade
		|| cls == Class_TgGame_TgDevice_HitPulse
		|| cls == Class_TgGame_TgDevice_MeleeDualWield
		|| cls == Class_TgGame_TgDevice_Morale
		|| cls == Class_TgGame_TgDevice_NewMelee
		|| cls == Class_TgGame_TgDevice_NewRange
	) {
		if (((actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventory, InvManager, ObjectProperty_Engine_Inventory_InvManager);
			DO_REP(AInventory, Inventory, ObjectProperty_Engine_Inventory_Inventory);
		}
	}
	if (
		cls == Class_Engine_InventoryManager
		|| cls == Class_TgGame_TgInventoryManager
	) {
		if ((((!actor->bSkipActorPropertyReplication || actor->bNetInitial) && actor->Role == 3) && actor->bNetDirty) && actor->bNetOwner) {
			DO_REP(AInventoryManager, InventoryChain, ObjectProperty_Engine_InventoryManager_InventoryChain);
		}
	}
	if (
		cls == Class_Engine_KActor
		|| cls == Class_Engine_KActorSpawnable
		|| cls == Class_TgGame_TgKActorSpawnable
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
		cls == Class_Engine_KAsset
		|| cls == Class_Engine_KAssetSpawnable
		|| cls == Class_TgGame_TgKAssetSpawnable
		|| cls == Class_TgGame_TgKAsset_ClientSideSim
	) {
		if (actor->Role == 3) {
			DO_REP(AKAsset, ReplicatedMesh, ObjectProperty_Engine_KAsset_ReplicatedMesh);
			DO_REP(AKAsset, ReplicatedPhysAsset, ObjectProperty_Engine_KAsset_ReplicatedPhysAsset);
		}
	}
	if (cls == Class_Engine_LensFlareSource) {
		if (actor->bNoDelete) {
			DO_REP(ALensFlareSource, bCurrentlyActive, BoolProperty_Engine_LensFlareSource_bCurrentlyActive);
		}
	}
	if (
		cls == Class_Engine_Light
		|| cls == Class_Engine_DirectionalLight
		|| cls == Class_Engine_DirectionalLightToggleable
		|| cls == Class_Engine_PointLight
		|| cls == Class_Engine_PointLightMovable
		|| cls == Class_Engine_PointLightToggleable
		|| cls == Class_Engine_SkyLight
		|| cls == Class_Engine_SkyLightToggleable
		|| cls == Class_Engine_SpotLight
		|| cls == Class_Engine_SpotLightMovable
		|| cls == Class_Engine_SpotLightToggleable
		|| cls == Class_Engine_StaticLightCollectionActor
		|| cls == Class_TgGame_TgCharacterBuilderLight
	) {
		if (actor->Role == 3) {
			DO_REP(ALight, bEnabled, BoolProperty_Engine_Light_bEnabled);
		}
	}
	if (cls == Class_Engine_MatineeActor) {
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
		cls == Class_Engine_NxForceField
		|| cls == Class_Engine_NxCylindricalForceField
		|| cls == Class_Engine_NxCylindricalForceFieldCapsule
		|| cls == Class_Engine_NxForceFieldGeneric
		|| cls == Class_Engine_NxForceFieldRadial
		|| cls == Class_Engine_NxForceFieldTornado
		|| cls == Class_Engine_NxGenericForceField
		|| cls == Class_Engine_NxGenericForceFieldBox
		|| cls == Class_Engine_NxGenericForceFieldCapsule
		|| cls == Class_Engine_NxRadialCustomForceField
		|| cls == Class_Engine_NxRadialForceField
		|| cls == Class_Engine_NxTornadoAngularForceField
		|| cls == Class_Engine_NxTornadoAngularForceFieldCapsule
		|| cls == Class_Engine_NxTornadoForceField
		|| cls == Class_Engine_NxTornadoForceFieldCapsule
	) {
		if (actor->bNetDirty) {
			DO_REP(ANxForceField, bForceActive, BoolProperty_Engine_NxForceField_bForceActive);
		}
	}



	if (
		cls == Class_Engine_Pawn
		|| cls == Class_TgGame_TgPawn
		|| cls == Class_TgGame_TgPawn_AVCompositeWalker
		|| cls == Class_TgGame_TgPawn_Ambush
		|| cls == Class_TgGame_TgPawn_AndroidMinion
		|| cls == Class_TgGame_TgPawn_AttackTransport
		|| cls == Class_TgGame_TgPawn_Boss
		|| cls == Class_TgGame_TgPawn_Boss_Destroyer
		|| cls == Class_TgGame_TgPawn_Brawler
		|| cls == Class_TgGame_TgPawn_CTR
		|| cls == Class_TgGame_TgPawn_Character
		|| cls == Class_TgGame_TgPawn_ColonyEye
		|| cls == Class_TgGame_TgPawn_Destructible
		|| cls == Class_TgGame_TgPawn_Detonator
		|| cls == Class_TgGame_TgPawn_Dismantler
		|| cls == Class_TgGame_TgPawn_DuneCommander
		|| cls == Class_TgGame_TgPawn_Elite_Alchemist
		|| cls == Class_TgGame_TgPawn_Elite_Assassin
		|| cls == Class_TgGame_TgPawn_EscortRobot
		|| cls == Class_TgGame_TgPawn_FlyingBoss
		|| cls == Class_TgGame_TgPawn_GroundPetA
		|| cls == Class_TgGame_TgPawn_Guardian
		|| cls == Class_TgGame_TgPawn_Hover
		|| cls == Class_TgGame_TgPawn_HoverShieldSphere
		|| cls == Class_TgGame_TgPawn_Hunter
		|| cls == Class_TgGame_TgPawn_Inquisitor
		|| cls == Class_TgGame_TgPawn_Interact_NPC
		|| cls == Class_TgGame_TgPawn_Iris
		|| cls == Class_TgGame_TgPawn_Juggernaut
		|| cls == Class_TgGame_TgPawn_Marauder
		|| cls == Class_TgGame_TgPawn_NPC
		|| cls == Class_TgGame_TgPawn_NewWasp
		|| cls == Class_TgGame_TgPawn_Raptor
		|| cls == Class_TgGame_TgPawn_Reaper
		|| cls == Class_TgGame_TgPawn_RecursiveSpawner
		|| cls == Class_TgGame_TgPawn_Remote
		|| cls == Class_TgGame_TgPawn_Robot
		|| cls == Class_TgGame_TgPawn_Scanner
		|| cls == Class_TgGame_TgPawn_ScannerRecursive
		|| cls == Class_TgGame_TgPawn_Siege
		|| cls == Class_TgGame_TgPawn_SiegeBarrage
		|| cls == Class_TgGame_TgPawn_SiegeHover
		|| cls == Class_TgGame_TgPawn_SiegeRapidFire
		|| cls == Class_TgGame_TgPawn_Sniper
		|| cls == Class_TgGame_TgPawn_SonoranCommander
		|| cls == Class_TgGame_TgPawn_SupportForeman
		|| cls == Class_TgGame_TgPawn_Switchblade
		|| cls == Class_TgGame_TgPawn_Tentacle
		|| cls == Class_TgGame_TgPawn_ThinkTank
		|| cls == Class_TgGame_TgPawn_TreadRobot
		|| cls == Class_TgGame_TgPawn_Turret
		|| cls == Class_TgGame_TgPawn_TurretAVAFlak
		|| cls == Class_TgGame_TgPawn_TurretAVARocket
		|| cls == Class_TgGame_TgPawn_TurretFlak
		|| cls == Class_TgGame_TgPawn_TurretFlame
		|| cls == Class_TgGame_TgPawn_TurretPlasma
		|| cls == Class_TgGame_TgPawn_UberWalker
		|| cls == Class_TgGame_TgPawn_Vanguard
		|| cls == Class_TgGame_TgPawn_VanityPet
		|| cls == Class_TgGame_TgPawn_Vulcan
		|| cls == Class_TgGame_TgPawn_Warlord
		|| cls == Class_TgGame_TgPawn_WaspSpawner
		|| cls == Class_TgGame_TgPawn_Widow
		|| cls == Class_Engine_Scout
		|| cls == Class_Engine_Vehicle
		|| cls == Class_Engine_SVehicle
		|| cls == Class_GameFramework_GamePawn
		|| cls == Class_GameFramework_GameVehicle
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
	if (cls == Class_Engine_PhysXEmitterSpawnable) {
		if (actor->bNetInitial) {
			DO_REP(APhysXEmitterSpawnable, ParticleTemplate, ObjectProperty_Engine_PhysXEmitterSpawnable_ParticleTemplate);
		}
	}
	if (
		cls == Class_Engine_PickupFactory
		|| cls == Class_TgGame_TgPickupFactory
		|| cls == Class_TgGame_TgPickupFactory_Item
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(APickupFactory, bPickupHidden, BoolProperty_Engine_PickupFactory_bPickupHidden);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(APickupFactory, InventoryType, ClassProperty_Engine_PickupFactory_InventoryType);
		}
	}
	if (
		cls == Class_Engine_PlayerController
		|| cls == Class_Engine_Admin
		|| cls == Class_Engine_DebugCameraController
		|| cls == Class_GameFramework_GamePlayerController
		|| cls == Class_TgGame_TgDebugCameraController
		|| cls == Class_TgGame_TgPlayerController
	) {
		if (((actor->bNetOwner && actor->Role == 3) && ((APlayerController*)actor)->ViewTarget != ((APlayerController*)actor)->Pawn) && ((APlayerController*)actor)->ViewTarget != NULL) {
			DO_REP(APlayerController, TargetEyeHeight, FloatProperty_Engine_PlayerController_TargetEyeHeight);
			DO_REP(APlayerController, TargetViewRotation, StructProperty_Engine_PlayerController_TargetViewRotation);
		}
	}
	if (
		cls == Class_Engine_PlayerReplicationInfo
		|| cls == Class_TgGame_TgRepInfo_Player
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
		cls == Class_Engine_PostProcessVolume
		|| cls == Class_TgGame_TgPostProcessVolume
	) {
		if (actor->bNetDirty) {
			DO_REP(APostProcessVolume, bEnabled, BoolProperty_Engine_PostProcessVolume_bEnabled);
		}
	}
	if (
		cls == Class_Engine_Projectile
		|| cls == Class_GameFramework_GameProjectile
		|| cls == Class_TgGame_TgProjectile
		|| cls == Class_TgGame_TgProj_Teleporter
		|| cls == Class_TgGame_TgProj_StraightTeleporter
		|| cls == Class_TgGame_TgProj_Rocket
		|| cls == Class_TgGame_TgProj_Net
		|| cls == Class_TgGame_TgProj_Mortar
		|| cls == Class_TgGame_TgProj_Missile
		|| cls == Class_TgGame_TgProj_Grenade
		|| cls == Class_TgGame_TgProj_Grapple
		|| cls == Class_TgGame_TgProj_FreeGrenade
		|| cls == Class_TgGame_TgProj_Deployable
		|| cls == Class_TgGame_TgProj_Bounce
		|| cls == Class_TgGame_TgProj_Bot
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(AProjectile, MaxSpeed, FloatProperty_Engine_Projectile_MaxSpeed);
			DO_REP(AProjectile, Speed, FloatProperty_Engine_Projectile_Speed);
		}
	}
	if (cls == Class_Engine_RB_CylindricalForceActor) {
		if (actor->bNetDirty) {
			DO_REP(ARB_CylindricalForceActor, bForceActive, BoolProperty_Engine_RB_CylindricalForceActor_bForceActive);
		}
	}
	if (cls == Class_Engine_RB_LineImpulseActor) {
		if (actor->bNetDirty) {
			DO_REP(ARB_LineImpulseActor, ImpulseCount, ByteProperty_Engine_RB_LineImpulseActor_ImpulseCount);
		}
	}
	if (cls == Class_Engine_RB_RadialForceActor) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialForceActor, bForceActive, BoolProperty_Engine_RB_RadialForceActor_bForceActive);
		}
	}
	if (cls == Class_Engine_RB_RadialImpulseActor) {
		if (actor->bNetDirty) {
			DO_REP(ARB_RadialImpulseActor, ImpulseCount, ByteProperty_Engine_RB_RadialImpulseActor_ImpulseCount);
		}
	}
	if (
		cls == Class_Engine_SVehicle
		|| cls == Class_GameFramework_GameVehicle
	) {
		if (actor->Physics == 10) {
			DO_REP(ASVehicle, MaxSpeed, FloatProperty_Engine_SVehicle_MaxSpeed);
			DO_REP(ASVehicle, VState, StructProperty_Engine_SVehicle_VState);
		}
	}
	if (
		cls == Class_Engine_SkeletalMeshActor
		|| cls == Class_Engine_SkeletalMeshActorBasedOnExtremeContent
		|| cls == Class_Engine_SkeletalMeshActorMAT
		|| cls == Class_Engine_SkeletalMeshActorMATSpawnable
		|| cls == Class_Engine_SkeletalMeshActorSpawnable
		|| cls == Class_TgGame_TgNavRouteIndicator
		|| cls == Class_TgGame_TgSkeletalMeshActorGenericUIPreview
		|| cls == Class_TgGame_TgSkeletalMeshActorNPC
		|| cls == Class_TgGame_TgSkeletalMeshActorNPCVendor
		|| cls == Class_TgGame_TgSkeletalMeshActorSpawnable
		|| cls == Class_TgGame_TgSkeletalMeshActor_CharacterBuilder
		|| cls == Class_TgGame_TgSkeletalMeshActor_CharacterBuilderSpawnable
		|| cls == Class_TgGame_TgSkeletalMeshActor_Composite
		|| cls == Class_TgGame_TgSkeletalMeshActor_EquipScreen
		|| cls == Class_TgGame_TgSkeletalMeshActor_MeleePreVis
		|| cls == Class_TgGame_TgSkeletalMeshActor
	) {
		if (actor->Role == 3) {
			DO_REP(ASkeletalMeshActor, ReplicatedMaterial, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMaterial);
			DO_REP(ASkeletalMeshActor, ReplicatedMesh, ObjectProperty_Engine_SkeletalMeshActor_ReplicatedMesh);
		}
	}
	if (
		cls == Class_Engine_TeamInfo
		|| cls == Class_TgGame_TgRepInfo_TaskForce
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
		cls == Class_Engine_Teleporter
		|| cls == Class_TgGame_TgTeleporter
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
		cls == Class_Engine_Vehicle
		|| cls == Class_Engine_SVehicle
		|| cls == Class_GameFramework_GameVehicle
	) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AVehicle, bDriving, BoolProperty_Engine_Vehicle_bDriving);
		}
		if ((actor->bNetDirty && (actor->bNetOwner || ((AVehicle*)actor)->Driver == NULL) || !((AVehicle*)actor)->Driver->bHidden) && actor->Role == 3) {
			DO_REP(AVehicle, Driver, ObjectProperty_Engine_Vehicle_Driver);
		}
	}
	if (cls == Class_Engine_WorldInfo) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(AWorldInfo, Pauser, ObjectProperty_Engine_WorldInfo_Pauser);
			DO_REP(AWorldInfo, ReplicatedMusicTrack, StructProperty_Engine_WorldInfo_ReplicatedMusicTrack);
			DO_REP(AWorldInfo, TimeDilation, FloatProperty_Engine_WorldInfo_TimeDilation);
			DO_REP(AWorldInfo, WorldGravityZ, FloatProperty_Engine_WorldInfo_WorldGravityZ);
			DO_REP(AWorldInfo, bHighPriorityLoading, BoolProperty_Engine_WorldInfo_bHighPriorityLoading);
		}
	}
	if (cls == Class_TgGame_TgChestActor) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgChestActor, r_eChestState, ByteProperty_TgGame_TgChestActor_r_eChestState);
		}
	}
	if (cls == Class_TgGame_TgDeploy_BeaconEntrance) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_BeaconEntrance, r_bActive, BoolProperty_TgGame_TgDeploy_BeaconEntrance_r_bActive);
		}
	}
	if (cls == Class_TgGame_TgDeploy_DestructibleCover) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_DestructibleCover, r_bHasFired, BoolProperty_TgGame_TgDeploy_DestructibleCover_r_bHasFired);
		}
	}
	if (cls == Class_TgGame_TgDeploy_Sensor) {
		if (actor->Role == 3) {
			DO_REP(ATgDeploy_Sensor, r_nSensorAudioWarning, IntProperty_TgGame_TgDeploy_Sensor_r_nSensorAudioWarning);
			DO_REP(ATgDeploy_Sensor, r_nTouchedPlayerCount, IntProperty_TgGame_TgDeploy_Sensor_r_nTouchedPlayerCount);
		}
	}
	if (cls == Class_TgGame_TgDeployable
		|| cls == Class_TgGame_TgDeploy_Artillery
		|| cls == Class_TgGame_TgDeploy_Beacon
		|| cls == Class_TgGame_TgDeploy_BeaconEntrance
		|| cls == Class_TgGame_TgDeploy_DestructibleCover
		|| cls == Class_TgGame_TgDeploy_ForceField
		|| cls == Class_TgGame_TgDeploy_Sensor
		|| cls == Class_TgGame_TgDeploy_SweepSensor
	) {
		if (actor->Role == 3) {
			DO_REP(ATgDeployable, r_bDelayDeployed, BoolProperty_TgGame_TgDeployable_r_bDelayDeployed);
			DO_REP(ATgDeployable, r_nReplicateDestroyIt, IntProperty_TgGame_TgDeployable_r_nReplicateDestroyIt);
			// Regen gap — UC TgDeployable.uc 116-120 "Role==Authority" block.
			// r_nFlashFireCount is the FX-pulse signal: every tick of
			// DeviceFiring.RefireCheckTimer bumps it (TgDeployable.uc:619),
			// client's ReplicatedEvent fires PlayFireFx(). Without this entry,
			// stations heal/pulse server-side but are silent + invisible.
			DO_REP(ATgDeployable, r_EffectManager, ObjectProperty_TgGame_TgDeployable_r_EffectManager);
			DO_REP(ATgDeployable, r_fDeployRate, FloatProperty_TgGame_TgDeployable_r_fDeployRate);
			DO_REP(ATgDeployable, r_fInitDeployTime, FloatProperty_TgGame_TgDeployable_r_fInitDeployTime);
			DO_REP(ATgDeployable, r_fTimeToDeploySecs, FloatProperty_TgGame_TgDeployable_r_fTimeToDeploySecs);
			DO_REP(ATgDeployable, r_nFlashCount, ByteProperty_TgGame_TgDeployable_r_nFlashCount);
			DO_REP(ATgDeployable, r_nFlashFireCount, ByteProperty_TgGame_TgDeployable_r_nFlashFireCount);
			DO_REP(ATgDeployable, r_nHealth, IntProperty_TgGame_TgDeployable_r_nHealth);
			DO_REP(ATgDeployable, r_vFlashLocation, StructProperty_TgGame_TgDeployable_r_vFlashLocation);
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
		cls == Class_TgGame_TgDevice
		|| cls == Class_TgGame_TgDevice_Grenade
		|| cls == Class_TgGame_TgDevice_HitPulse
		|| cls == Class_TgGame_TgDevice_Morale
		|| cls == Class_TgGame_TgDevice_NewMelee
		|| cls == Class_TgGame_TgDevice_MeleeDualWield
		|| cls == Class_TgGame_TgDevice_NewRange
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
	if (cls == Class_TgGame_TgDevice_Morale) {
		if (actor->Role == 3) {
			DO_REP(ATgDevice_Morale, r_bIsActivelyFiring, BoolProperty_TgGame_TgDevice_Morale_r_bIsActivelyFiring);
		}
	}
	if (cls == Class_TgGame_TgDoor) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDoor, r_bOpen, BoolProperty_TgGame_TgDoor_r_bOpen);
		}
	}
	if (cls == Class_TgGame_TgDoorMarker) {
		if (actor->Role == 3) {
			DO_REP(ATgDoorMarker, r_eStatus, ByteProperty_TgGame_TgDoorMarker_r_eStatus);
		}
	}
	if (cls == Class_TgGame_TgDroppedItem) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgDroppedItem, r_nItemId, IntProperty_TgGame_TgDroppedItem_r_nItemId);
		}
	}
	if (cls == Class_TgGame_TgDynamicDestructible) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicDestructible, r_nDestructibleId, IntProperty_TgGame_TgDynamicDestructible_r_nDestructibleId);
			DO_REP(ATgDynamicDestructible, r_pFactory, ObjectProperty_TgGame_TgDynamicDestructible_r_pFactory);
		}
	}
	if (
		cls == Class_TgGame_TgDynamicSMActor
		|| cls == Class_TgGame_TgDynamicDestructible
		|| cls == Class_TgGame_TgObjectiveAttachActor
	) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicSMActor, m_sAssembly, StrProperty_TgGame_TgDynamicSMActor_m_sAssembly);
		}
	}
	if (
		cls == Class_TgGame_TgDynamicSMActor
		|| cls == Class_TgGame_TgDynamicDestructible
		|| cls == Class_TgGame_TgObjectiveAttachActor
	) {

		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgDynamicSMActor, r_EffectManager, ObjectProperty_TgGame_TgDynamicSMActor_r_EffectManager);
		}
		if (actor->Role == 3) {
			DO_REP(ATgDynamicSMActor, r_nHealth, IntProperty_TgGame_TgDynamicSMActor_r_nHealth);
		}
	}
	if (cls == Class_TgGame_TgEffectManager) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgEffectManager, r_EventQueue, StructProperty_TgGame_TgEffectManager_r_EventQueue);
			DO_REP_ARRAY(0x10, ATgEffectManager, r_ManagedEffectList, StructProperty_TgGame_TgEffectManager_r_ManagedEffectList);
			DO_REP(ATgEffectManager, r_Owner, ObjectProperty_TgGame_TgEffectManager_r_Owner);
			DO_REP(ATgEffectManager, r_bRelevancyNotify, BoolProperty_TgGame_TgEffectManager_r_bRelevancyNotify);
			DO_REP(ATgEffectManager, r_nInvulnerableCount, IntProperty_TgGame_TgEffectManager_r_nInvulnerableCount);
			DO_REP(ATgEffectManager, r_nNextQueueIndex, IntProperty_TgGame_TgEffectManager_r_nNextQueueIndex);
		}
	}
	if (cls == Class_TgGame_TgEmitter) {
		if (actor->Role == 3) {
			DO_REP(ATgEmitter, BoneName, NameProperty_TgGame_TgEmitter_BoneName);
		}
	}
	if (cls == Class_TgGame_TgFlagCaptureVolume) {
		if (actor->Role == 3) {
			DO_REP(ATgFlagCaptureVolume, r_eCoalition, ByteProperty_TgGame_TgFlagCaptureVolume_r_eCoalition);
			DO_REP(ATgFlagCaptureVolume, r_nTaskForce, ByteProperty_TgGame_TgFlagCaptureVolume_r_nTaskForce);
		}
	}
	if (cls == Class_TgGame_TgFracturedStaticMeshActor) {
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
	if (cls == Class_TgGame_TgHexLandMarkActor) {
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgHexLandMarkActor, r_nMeshAsmId, IntProperty_TgGame_TgHexLandMarkActor_r_nMeshAsmId);
		}
	}
	if (cls == Class_TgGame_TgInterpActor) {
		if (actor->Role == 3) {
			DO_REP(ATgInterpActor, r_sCurrState, StrProperty_TgGame_TgInterpActor_r_sCurrState);
		}
	}
	if (cls == Class_TgGame_TgInventoryManager) {
		if (actor->Role == 3) {
			DO_REP(ATgInventoryManager, r_ItemCount, IntProperty_TgGame_TgInventoryManager_r_ItemCount);
		}
	}
	if (cls == Class_TgGame_TgKismetTestActor) {
		if (actor->Role == 3) {
			DO_REP(ATgKismetTestActor, r_nCurrentTest, IntProperty_TgGame_TgKismetTestActor_r_nCurrentTest);
			DO_REP(ATgKismetTestActor, r_nFailCount, IntProperty_TgGame_TgKismetTestActor_r_nFailCount);
			DO_REP(ATgKismetTestActor, r_nPassCount, IntProperty_TgGame_TgKismetTestActor_r_nPassCount);
		}
	}
	if (cls == Class_TgGame_TgLevelCamera) {
		if (actor->Role == 3) {
			DO_REP(ATgLevelCamera, r_bEnabled, BoolProperty_TgGame_TgLevelCamera_r_bEnabled);
		}
	}
	if (
		cls == Class_TgGame_TgMissionObjective
		|| cls == Class_TgGame_TgBaseObjective_CTFBot
		|| cls == Class_TgGame_TgBaseObjective_KOTH
		|| cls == Class_TgGame_TgMissionObjective_Bot
		|| cls == Class_TgGame_TgMissionObjective_Escort
		|| cls == Class_TgGame_TgMissionObjective_Kismet
		|| cls == Class_TgGame_TgMissionObjective_Proximity
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
		cls == Class_TgGame_TgMissionObjective_Bot
		|| cls == Class_TgGame_TgBaseObjective_CTFBot
	) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBot, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBot);
			DO_REP(ATgMissionObjective_Bot, r_ObjectiveBotInfo, ObjectProperty_TgGame_TgMissionObjective_Bot_r_ObjectiveBotInfo);
		}
	}
	if (cls == Class_TgGame_TgMissionObjective_Escort) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Escort, r_AttachedActor, ObjectProperty_TgGame_TgMissionObjective_Escort_r_AttachedActor);
		}
	}
	if (
		cls == Class_TgGame_TgMissionObjective_Proximity
		|| cls == Class_TgGame_TgBaseObjective_KOTH
		|| cls == Class_TgGame_TgMissionObjective_Escort
	) {
		if (actor->Role == 3) {
			DO_REP(ATgMissionObjective_Proximity, r_fCaptureRate, FloatProperty_TgGame_TgMissionObjective_Proximity_r_fCaptureRate);
		}
	}
	if (cls == Class_TgGame_TgObjectiveAssignment) {
		if (actor->Role == 3) {
			DO_REP(ATgObjectiveAssignment, r_AssignedObjective, ObjectProperty_TgGame_TgObjectiveAssignment_r_AssignedObjective);
			DO_REP(ATgObjectiveAssignment, r_Attackers, ObjectProperty_TgGame_TgObjectiveAssignment_r_Attackers);
			DO_REP(ATgObjectiveAssignment, r_Bots, ObjectProperty_TgGame_TgObjectiveAssignment_r_Bots);
			DO_REP(ATgObjectiveAssignment, r_Defenders, ObjectProperty_TgGame_TgObjectiveAssignment_r_Defenders);
			DO_REP(ATgObjectiveAssignment, r_eState, ByteProperty_TgGame_TgObjectiveAssignment_r_eState);
		}
	}
	if (
		cls == Class_TgGame_TgPawn
		|| cls == Class_TgGame_TgPawn_AVCompositeWalker
		|| cls == Class_TgGame_TgPawn_Ambush
		|| cls == Class_TgGame_TgPawn_AndroidMinion
		|| cls == Class_TgGame_TgPawn_AttackTransport
		|| cls == Class_TgGame_TgPawn_Boss
		|| cls == Class_TgGame_TgPawn_Boss_Destroyer
		|| cls == Class_TgGame_TgPawn_Brawler
		|| cls == Class_TgGame_TgPawn_CTR
		|| cls == Class_TgGame_TgPawn_Character
		|| cls == Class_TgGame_TgPawn_ColonyEye
		|| cls == Class_TgGame_TgPawn_Destructible
		|| cls == Class_TgGame_TgPawn_Detonator
		|| cls == Class_TgGame_TgPawn_Dismantler
		|| cls == Class_TgGame_TgPawn_DuneCommander
		|| cls == Class_TgGame_TgPawn_Elite_Alchemist
		|| cls == Class_TgGame_TgPawn_Elite_Assassin
		|| cls == Class_TgGame_TgPawn_EscortRobot
		|| cls == Class_TgGame_TgPawn_FlyingBoss
		|| cls == Class_TgGame_TgPawn_GroundPetA
		|| cls == Class_TgGame_TgPawn_Guardian
		|| cls == Class_TgGame_TgPawn_Hover
		|| cls == Class_TgGame_TgPawn_HoverShieldSphere
		|| cls == Class_TgGame_TgPawn_Hunter
		|| cls == Class_TgGame_TgPawn_Inquisitor
		|| cls == Class_TgGame_TgPawn_Interact_NPC
		|| cls == Class_TgGame_TgPawn_Iris
		|| cls == Class_TgGame_TgPawn_Juggernaut
		|| cls == Class_TgGame_TgPawn_Marauder
		|| cls == Class_TgGame_TgPawn_NPC
		|| cls == Class_TgGame_TgPawn_NewWasp
		|| cls == Class_TgGame_TgPawn_Raptor
		|| cls == Class_TgGame_TgPawn_Reaper
		|| cls == Class_TgGame_TgPawn_RecursiveSpawner
		|| cls == Class_TgGame_TgPawn_Remote
		|| cls == Class_TgGame_TgPawn_Robot
		|| cls == Class_TgGame_TgPawn_Scanner
		|| cls == Class_TgGame_TgPawn_ScannerRecursive
		|| cls == Class_TgGame_TgPawn_Siege
		|| cls == Class_TgGame_TgPawn_SiegeBarrage
		|| cls == Class_TgGame_TgPawn_SiegeHover
		|| cls == Class_TgGame_TgPawn_SiegeRapidFire
		|| cls == Class_TgGame_TgPawn_Sniper
		|| cls == Class_TgGame_TgPawn_SonoranCommander
		|| cls == Class_TgGame_TgPawn_SupportForeman
		|| cls == Class_TgGame_TgPawn_Switchblade
		|| cls == Class_TgGame_TgPawn_Tentacle
		|| cls == Class_TgGame_TgPawn_ThinkTank
		|| cls == Class_TgGame_TgPawn_TreadRobot
		|| cls == Class_TgGame_TgPawn_Turret
		|| cls == Class_TgGame_TgPawn_TurretAVAFlak
		|| cls == Class_TgGame_TgPawn_TurretAVARocket
		|| cls == Class_TgGame_TgPawn_TurretFlak
		|| cls == Class_TgGame_TgPawn_TurretFlame
		|| cls == Class_TgGame_TgPawn_TurretPlasma
		|| cls == Class_TgGame_TgPawn_UberWalker
		|| cls == Class_TgGame_TgPawn_Vanguard
		|| cls == Class_TgGame_TgPawn_VanityPet
		|| cls == Class_TgGame_TgPawn_Vulcan
		|| cls == Class_TgGame_TgPawn_Warlord
		|| cls == Class_TgGame_TgPawn_WaspSpawner
		|| cls == Class_TgGame_TgPawn_Widow
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
		if ((actor->Role == 3)/* && !actor->bNetOwner*/) {
			DO_REP(ATgPawn, r_bInitialIsEnemy, BoolProperty_TgGame_TgPawn_r_bInitialIsEnemy);
			DO_REP(ATgPawn, r_bMadeSound, ByteProperty_TgGame_TgPawn_r_bMadeSound);
			DO_REP(ATgPawn, r_eDesiredInHand, ByteProperty_TgGame_TgPawn_r_eDesiredInHand);
			DO_REP(ATgPawn, r_eEquippedInHandMode, IntProperty_TgGame_TgPawn_r_eEquippedInHandMode);
			DO_REP(ATgPawn, r_nReplicateHit, IntProperty_TgGame_TgPawn_r_nReplicateHit);
			DO_REP_ARRAY(25, ATgPawn, r_EquipDeviceInfo, StructProperty_TgGame_TgPawn_r_EquipDeviceInfo);
		}
		if ((actor->Role == 3)/* && actor->bNetOwner*/) {
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
		cls == Class_TgGame_TgPawn_Ambush
		|| cls == Class_TgGame_TgPawn_Tentacle
	) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Ambush, r_bIsDeployed, BoolProperty_TgGame_TgPawn_Ambush_r_bIsDeployed);
		}
	}
	if (cls == Class_TgGame_TgPawn_AttackTransport) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_AttackTransport, r_DeathType, ByteProperty_TgGame_TgPawn_AttackTransport_r_DeathType);
		}
	}
	if (cls == Class_TgGame_TgPawn_CTR) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_CTR, r_CustomCharacterAssembly, StructProperty_TgGame_TgPawn_CTR_r_CustomCharacterAssembly);
			DO_REP(ATgPawn_CTR, r_PilotPawn, ObjectProperty_TgGame_TgPawn_CTR_r_PilotPawn);
			DO_REP(ATgPawn_CTR, r_nMaxMorphIndexSentFromServer, IntProperty_TgGame_TgPawn_CTR_r_nMaxMorphIndexSentFromServer);
			DO_REP_ARRAY(0xFF, ATgPawn_CTR, r_nMorphSettings, IntProperty_TgGame_TgPawn_CTR_r_nMorphSettings);
		}
	}
	if (
		cls == Class_TgGame_TgPawn_Character
		|| cls == Class_TgGame_TgPawn_AndroidMinion
		|| cls == Class_TgGame_TgPawn_Boss
		|| cls == Class_TgGame_TgPawn_Boss_Destroyer
		|| cls == Class_TgGame_TgPawn_Brawler
		|| cls == Class_TgGame_TgPawn_CTR
		|| cls == Class_TgGame_TgPawn_ColonyEye
		|| cls == Class_TgGame_TgPawn_Dismantler
		|| cls == Class_TgGame_TgPawn_DuneCommander
		|| cls == Class_TgGame_TgPawn_Elite_Alchemist
		|| cls == Class_TgGame_TgPawn_Elite_Assassin
		|| cls == Class_TgGame_TgPawn_FlyingBoss
		|| cls == Class_TgGame_TgPawn_Guardian
		|| cls == Class_TgGame_TgPawn_Hunter
		|| cls == Class_TgGame_TgPawn_Inquisitor
		|| cls == Class_TgGame_TgPawn_Interact_NPC
		|| cls == Class_TgGame_TgPawn_Juggernaut
		|| cls == Class_TgGame_TgPawn_Marauder
		|| cls == Class_TgGame_TgPawn_NPC
		|| cls == Class_TgGame_TgPawn_Raptor
		|| cls == Class_TgGame_TgPawn_Reaper
		|| cls == Class_TgGame_TgPawn_RecursiveSpawner
		|| cls == Class_TgGame_TgPawn_Sniper
		|| cls == Class_TgGame_TgPawn_SonoranCommander
		|| cls == Class_TgGame_TgPawn_Switchblade
		|| cls == Class_TgGame_TgPawn_ThinkTank
		|| cls == Class_TgGame_TgPawn_Turret
		|| cls == Class_TgGame_TgPawn_TurretAVAFlak
		|| cls == Class_TgGame_TgPawn_TurretAVARocket
		|| cls == Class_TgGame_TgPawn_TurretFlak
		|| cls == Class_TgGame_TgPawn_TurretFlame
		|| cls == Class_TgGame_TgPawn_TurretPlasma
		|| cls == Class_TgGame_TgPawn_UberWalker
		|| cls == Class_TgGame_TgPawn_Vanguard
		|| cls == Class_TgGame_TgPawn_Vulcan
		|| cls == Class_TgGame_TgPawn_Warlord
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
	if (cls == Class_TgGame_TgPawn_DuneCommander) {
		if ((actor->Role == 3) && actor->bNetDirty || actor->bNetInitial) {
			DO_REP(ATgPawn_DuneCommander, r_bDoCrashLanding, BoolProperty_TgGame_TgPawn_DuneCommander_r_bDoCrashLanding);
		}
	}
	if (cls == Class_TgGame_TgPawn_Iris) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_Iris, r_nStartNewScan, ByteProperty_TgGame_TgPawn_Iris_r_nStartNewScan);
		}
	}
	if (cls == Class_TgGame_TgPawn_Reaper) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Reaper, r_fBatteryPct, FloatProperty_TgGame_TgPawn_Reaper_r_fBatteryPct);
		}
	}
	if (
		cls == Class_TgGame_TgPawn_Siege
		|| cls == Class_TgGame_TgPawn_SiegeBarrage
		|| cls == Class_TgGame_TgPawn_SiegeHover
		|| cls == Class_TgGame_TgPawn_SiegeRapidFire
	) {
		if ((actor->Role == 3) && actor->bNetDirty) {
			DO_REP(ATgPawn_Siege, r_AccelDirection, ByteProperty_TgGame_TgPawn_Siege_r_AccelDirection);
		}
	}
	if (
		cls == Class_TgGame_TgPawn_Turret
		|| cls == Class_TgGame_TgPawn_TurretAVAFlak
		|| cls == Class_TgGame_TgPawn_TurretAVARocket
		|| cls == Class_TgGame_TgPawn_TurretFlak
		|| cls == Class_TgGame_TgPawn_TurretFlame
		|| cls == Class_TgGame_TgPawn_TurretPlasma
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
	if (cls == Class_TgGame_TgPawn_VanityPet) {
		if (actor->Role == 3) {
			DO_REP(ATgPawn_VanityPet, r_nSpawningItemId, IntProperty_TgGame_TgPawn_VanityPet_r_nSpawningItemId);
		}
	}
	if (cls == Class_TgGame_TgPlayerController) {
		if ((actor->Role == 3) && actor->bNetOwner) {
			DO_REP(ATgPlayerController, r_WatchOtherPlayer, ByteProperty_TgGame_TgPlayerController_r_WatchOtherPlayer);
			// DO_REP(ATgPlayerController, r_bEDDebugEffects, BoolProperty_TgGame_TgPlayerController_r_bEDDebugEffects);
			DO_REP(ATgPlayerController, r_bGMInvisible, BoolProperty_TgGame_TgPlayerController_r_bGMInvisible);
			DO_REP(ATgPlayerController, r_bIsHackingABot, BoolProperty_TgGame_TgPlayerController_r_bIsHackingABot);
			DO_REP(ATgPlayerController, r_bLockYawRotation, BoolProperty_TgGame_TgPlayerController_r_bLockYawRotation);
			DO_REP(ATgPlayerController, r_bRove, BoolProperty_TgGame_TgPlayerController_r_bRove);
		}
	}
	if (cls == Class_TgGame_TgProj_Grapple) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Grapple, r_vTargetLocation, StructProperty_TgGame_TgProj_Grapple_r_vTargetLocation);
		}
	}
	if (cls == Class_TgGame_TgProj_Missile) {
		if (actor->Role == 3) {
			DO_REP(ATgProj_Missile, r_aSeeking, ObjectProperty_TgGame_TgProj_Missile_r_aSeeking);
			DO_REP(ATgProj_Missile, r_vTargetWorldLocation, StructProperty_TgGame_TgProj_Missile_r_vTargetWorldLocation);
		}
		if ((actor->Role == 3) && actor->bNetInitial) {
			DO_REP(ATgProj_Missile, r_nNumBounces, IntProperty_TgGame_TgProj_Missile_r_nNumBounces);
		}
	}
	if (cls == Class_TgGame_TgProj_Rocket) {
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgProj_Rocket, FlockIndex, ByteProperty_TgGame_TgProj_Rocket_FlockIndex);
			DO_REP(ATgProj_Rocket, bCurl, BoolProperty_TgGame_TgProj_Rocket_bCurl);
		}
	}
	if (
		cls == Class_TgGame_TgProjectile
		|| cls == Class_TgGame_TgProj_Teleporter
		|| cls == Class_TgGame_TgProj_StraightTeleporter
		|| cls == Class_TgGame_TgProj_Rocket
		|| cls == Class_TgGame_TgProj_Net
		|| cls == Class_TgGame_TgProj_Mortar
		|| cls == Class_TgGame_TgProj_Missile
		|| cls == Class_TgGame_TgProj_Grenade
		|| cls == Class_TgGame_TgProj_Grapple
		|| cls == Class_TgGame_TgProj_FreeGrenade
		|| cls == Class_TgGame_TgProj_Deployable
		|| cls == Class_TgGame_TgProj_Bounce
		|| cls == Class_TgGame_TgProj_Bot
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
	if (cls == Class_TgGame_TgRepInfo_Beacon) {
		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_bDeployed, BoolProperty_TgGame_TgRepInfo_Beacon_r_bDeployed);
			DO_REP(ATgRepInfo_Beacon, r_vLoc, StructProperty_TgGame_TgRepInfo_Beacon_r_vLoc);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgRepInfo_Beacon, r_nName, StrProperty_TgGame_TgRepInfo_Beacon_r_nName);
		}
	}
	if (
		cls == Class_TgGame_TgRepInfo_Deployable
		|| cls == Class_TgGame_TgRepInfo_Beacon
	) {
		// DIAG: DRI replication isn't reaching the client — no ReplicatedEvent
		// for r_TaskforceInfo / r_InstigatorInfo / r_bOwnedByTaskforce / etc.
		// ever fires client-side (verified in replicated_event.txt).  If this
		// log is ABSENT, the engine isn't queuing the DRI for replication at
		// all (relevance/channel failure).  If PRESENT, rep is evaluated but
		// something downstream drops the bunch.
		// Logger::Log("heal_tick",
		// 	"[V2 DRI rep] dri=0x%p class=%s  Role=%d  bNetInitial=%d  bNetDirty=%d\n",
		// 	actor, cls->GetFullName(),
		// 	(int)actor->Role, (int)actor->bNetInitial, (int)actor->bNetDirty);

		if (actor->bNetDirty && actor->Role == 3) {
			DO_REP(ATgRepInfo_Deployable, r_InstigatorInfo, ObjectProperty_TgGame_TgRepInfo_Deployable_r_InstigatorInfo);
			DO_REP(ATgRepInfo_Deployable, r_TaskforceInfo, ObjectProperty_TgGame_TgRepInfo_Deployable_r_TaskforceInfo);
			DO_REP(ATgRepInfo_Deployable, r_bOwnedByTaskforce, BoolProperty_TgGame_TgRepInfo_Deployable_r_bOwnedByTaskforce);
			DO_REP(ATgRepInfo_Deployable, r_nHealthCurrent, IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthCurrent);
		}
		if (actor->bNetInitial && actor->Role == 3) {
			DO_REP(ATgRepInfo_Deployable, r_fDeployMaxHealthPCT, FloatProperty_TgGame_TgRepInfo_Deployable_r_fDeployMaxHealthPCT);
			DO_REP(ATgRepInfo_Deployable, r_nDeployableId, IntProperty_TgGame_TgRepInfo_Deployable_r_nDeployableId);
			DO_REP(ATgRepInfo_Deployable, r_nHealthMaximum, IntProperty_TgGame_TgRepInfo_Deployable_r_nHealthMaximum);
			DO_REP(ATgRepInfo_Deployable, r_nUniqueDeployableId, IntProperty_TgGame_TgRepInfo_Deployable_r_nUniqueDeployableId);
			DO_REP(ATgRepInfo_Deployable, r_DeployableOwner, ObjectProperty_TgGame_TgRepInfo_Deployable_r_DeployableOwner);
		}
	}
	if (
		cls == Class_TgGame_TgRepInfo_Game
		|| cls == Class_TgGame_TgRepInfo_GameOpenWorld
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
	if (cls == Class_TgGame_TgRepInfo_GameOpenWorld) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x3, ATgRepInfo_GameOpenWorld, r_GameTickets, IntProperty_TgGame_TgRepInfo_GameOpenWorld_r_GameTickets);
		}
	}
	if (cls == Class_TgGame_TgRepInfo_Player) {
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
		cls == Class_TgGame_TgRepInfo_TaskForce
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
	if (cls == Class_TgGame_TgSkydiveTarget) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydiveTarget, m_LandRadius, FloatProperty_TgGame_TgSkydiveTarget_m_LandRadius);
		}
	}
	if (cls == Class_TgGame_TgSkydivingVolume) {
		if (actor->Role == 3) {
			DO_REP(ATgSkydivingVolume, r_PawnGravityModifier, FloatProperty_TgGame_TgSkydivingVolume_r_PawnGravityModifier);
			DO_REP(ATgSkydivingVolume, r_PawnLaunchForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnLaunchForce);
			DO_REP(ATgSkydivingVolume, r_PawnUpForce, FloatProperty_TgGame_TgSkydivingVolume_r_PawnUpForce);
			DO_REP(ATgSkydivingVolume, r_SkydiveTarget, ObjectProperty_TgGame_TgSkydivingVolume_r_SkydiveTarget);
		}
	}
	if (cls == Class_TgGame_TgTeamBeaconManager) {
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
	if (cls == Class_TgGame_TgTimerManager) {
		if (actor->Role == 3) {
			DO_REP_ARRAY(0x20, ATgTimerManager, r_byEventQue, ByteProperty_TgGame_TgTimerManager_r_byEventQue);
			DO_REP(ATgTimerManager, r_byEventQueIndex, ByteProperty_TgGame_TgTimerManager_r_byEventQueIndex);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fRemaining, FloatProperty_TgGame_TgTimerManager_r_fRemaining);
			DO_REP_ARRAY(0x20, ATgTimerManager, r_fStartTime, FloatProperty_TgGame_TgTimerManager_r_fStartTime);
		}
	}

	// Mark the actor as having had its initial replication pass; subsequent calls
	// will see isInitialRep == false. Performed once at end-of-call (vs the original
	// per-property-on-first-DO_REP fan-out into 511 separate maps).
	if (isInitialRep) g_RepListInitialDoneActors.insert(actor);
	return param_3;
}

