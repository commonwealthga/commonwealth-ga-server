/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: GameFramework_f_structs.h
# ========================================================================================= #
# Credits: uNrEaL, Tamimego, SystemFiles, R00T88, _silencer, the1domo, K@N@VEL
# Thanks: HOOAH07, lowHertz
# Forums: www.uc-forum.com, www.gamedeception.net
#############################################################################################
*/

#ifdef _MSC_VER
	#pragma pack ( push, 0x4 )
#endif

/*
# ========================================================================================= #
# Function Structs
# ========================================================================================= #
*/

// Function GameFramework.GameAIController.SetDesiredRotation
// [0x00024002] 
struct AGameAIController_execSetDesiredRotation_Parms
{
	struct FRotator                                    TargetDesiredRotation;                            		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bZeroPitch : 1;                                   		// 0x000C (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function GameFramework.GameAIController.AILog_Internal
// [0x00024800] ( FUNC_Event )
struct AGameAIController_eventAILog_Internal_Parms
{
	struct FString                                     LogText;                                          		// 0x0000 (0x000C) [0x0000000000400880]              ( CPF_Parm | CPF_CoerceParm | CPF_NeedCtorLink )
	struct FName                                       LogCategory;                                      		// 0x000C (0x0008) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bForce : 1;                                       		// 0x0014 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function GameFramework.GameAIController.RecordDemoAILog
// [0x00080000] 
struct AGameAIController_execRecordDemoAILog_Parms
{
	struct FString                                     LogText;                                          		// 0x0000 (0x000C) [0x0000000000400880]              ( CPF_Parm | CPF_CoerceParm | CPF_NeedCtorLink )
};

// Function GameFramework.GameAIController.Destroyed
// [0x00020802] ( FUNC_Event )
struct AGameAIController_eventDestroyed_Parms
{
};

// Function GameFramework.GameAIController.FindCommandOfClass
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execFindCommandOfClass_Parms
{
	class UClass*                                      SearchClass;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UGameAICommand*                              ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000D80]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_CoerceParm )
};

// Function GameFramework.GameAIController.DumpCommandStack
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execDumpCommandStack_Parms
{
};

// Function GameFramework.GameAIController.CheckCommandCount
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execCheckCommandCount_Parms
{
};

// Function GameFramework.GameAIController.GetActiveCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execGetActiveCommand_Parms
{
	class UGameAICommand*                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameAIController.AbortCommand
// [0x00024401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execAbortCommand_Parms
{
	class UGameAICommand*                              AbortCmd;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UClass*                                      AbortClass;                                       		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameAIController.PopCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execPopCommand_Parms
{
	class UGameAICommand*                              NewCommand;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAIController.PushCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct AGameAIController_execPushCommand_Parms
{
	class UGameAICommand*                              NewCommand;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.GetDumpString
// [0x00020802] ( FUNC_Event )
struct UGameAICommand_eventGetDumpString_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function GameFramework.GameAICommand.Resumed
// [0x00020002] 
struct UGameAICommand_execResumed_Parms
{
	struct FName                                       OldCommandName;                                   		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.Paused
// [0x00020002] 
struct UGameAICommand_execPaused_Parms
{
	class UGameAICommand*                              NewCommand;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.Popped
// [0x00020002] 
struct UGameAICommand_execPopped_Parms
{
};

// Function GameFramework.GameAICommand.Pushed
// [0x00020002] 
struct UGameAICommand_execPushed_Parms
{
};

// Function GameFramework.GameAICommand.PostPopped
// [0x00020000] 
struct UGameAICommand_execPostPopped_Parms
{
};

// Function GameFramework.GameAICommand.PrePushed
// [0x00020000] 
struct UGameAICommand_execPrePushed_Parms
{
	class AGameAIController*                           AI;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.AllowTransitionTo
// [0x00020002] 
struct UGameAICommand_execAllowTransitionTo_Parms
{
	class UClass*                                      AttemptCommand;                                   		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameAICommand.Tick
// [0x00020000] 
struct UGameAICommand_execTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.ShouldIgnoreNotifies
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UGameAICommand_execShouldIgnoreNotifies_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameAICommand.InternalTick
// [0x00020803] ( FUNC_Final | FUNC_Event )
struct UGameAICommand_eventInternalTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.InternalResumed
// [0x00020803] ( FUNC_Final | FUNC_Event )
struct UGameAICommand_eventInternalResumed_Parms
{
	struct FName                                       OldCommandName;                                   		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.InternalPaused
// [0x00020803] ( FUNC_Final | FUNC_Event )
struct UGameAICommand_eventInternalPaused_Parms
{
	class UGameAICommand*                              NewCommand;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.InternalPopped
// [0x00020802] ( FUNC_Event )
struct UGameAICommand_eventInternalPopped_Parms
{
};

// Function GameFramework.GameAICommand.InternalPushed
// [0x00020803] ( FUNC_Final | FUNC_Event )
struct UGameAICommand_eventInternalPushed_Parms
{
};

// Function GameFramework.GameAICommand.InternalPrePushed
// [0x00020803] ( FUNC_Final | FUNC_Event )
struct UGameAICommand_eventInternalPrePushed_Parms
{
	class AGameAIController*                           AI;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameAICommand.InitCommand
// [0x00022002] 
struct UGameAICommand_execInitCommand_Parms
{
	class AGameAIController*                           AI;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UGameAICommand*                           Cmd;                                              		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function GameFramework.GameAICommand.InitCommandUserActor
// [0x00022002] 
struct UGameAICommand_execInitCommandUserActor_Parms
{
	class AGameAIController*                           AI;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class AActor*                                      UserActor;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameExplosionActor.Tick
// [0x00020102] 
struct AGameExplosionActor_execTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// float                                           TimerCount;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	// float                                           Pct;                                              		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.DoExplosionDamage
// [0x00020102] 
struct AGameExplosionActor_execDoExplosionDamage_Parms
{
};

// Function GameFramework.GameExplosionActor.Explode
// [0x00020902] ( FUNC_Event )
struct AGameExplosionActor_eventExplode_Parms
{
	class UGameExplosion*                              NewExplosionTemplate;                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// float                                           HowLongToLive;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UPhysicalMaterial*                        PhysMat;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.SpawnExplosionFogVolume
// [0x00020100] 
struct AGameExplosionActor_execSpawnExplosionFogVolume_Parms
{
};

// Function GameFramework.GameExplosionActor.DoExplosionCameraShakes
// [0x00020100] 
struct AGameExplosionActor_execDoExplosionCameraShakes_Parms
{
};

// Function GameFramework.GameExplosionActor.SpawnExplosionDecal
// [0x00020100] 
struct AGameExplosionActor_execSpawnExplosionDecal_Parms
{
};

// Function GameFramework.GameExplosionActor.SpawnExplosionParticleSystem
// [0x00020100] 
struct AGameExplosionActor_execSpawnExplosionParticleSystem_Parms
{
	class UParticleSystem*                             Template;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameExplosionActor.UpdateExplosionTemplateWithPerMaterialFX
// [0x00080100] 
struct AGameExplosionActor_execUpdateExplosionTemplateWithPerMaterialFX_Parms
{
	class UPhysicalMaterial*                           PhysMaterial;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameExplosionActor.KnockdownPawn
// [0x00080002] 
struct AGameExplosionActor_execKnockdownPawn_Parms
{
	class AGamePawn*                                   Victim;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              DistFromExplosion;                                		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameExplosionActor.DoCringesAndKnockdowns
// [0x00880002] 
struct AGameExplosionActor_execDoCringesAndKnockdowns_Parms
{
	// class AGamePawn*                                Victim;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	// float                                           DistFromExplosion;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	// struct FVector                                  ExplosionToVictimDir;                             		// 0x0008 (0x000C) [0x0000000000000000]              
	// float                                           KnockDownRad;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	// float                                           CringeRad;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.ShouldDoCringeFor
// [0x00080002] 
struct AGameExplosionActor_execShouldDoCringeFor_Parms
{
	class AGamePawn*                                   Victim;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameExplosionActor.DoBreakFracturedMeshes
// [0x00080102] 
struct AGameExplosionActor_execDoBreakFracturedMeshes_Parms
{
	struct FVector                                     ExploOrigin;                                      		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	float                                              DamageRadius;                                     		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              RBStrength;                                       		// 0x0010 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UClass*                                      dmgType;                                          		// 0x0014 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// class AFracturedStaticMeshActor*                FracActor;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
	// unsigned char                                   bWantPhysChunksAndParticles;                      		// 0x001C (0x0001) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.HurtExplosion
// [0x00880103] ( FUNC_Final )
struct AGameExplosionActor_execHurtExplosion_Parms
{
	float                                              BaseDamage;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              DamageRadius;                                     		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              DamageFalloffExp;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UClass*                                      DamageType;                                       		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              MomentumScale;                                    		// 0x0010 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FVector                                     ExploOrigin;                                      		// 0x0014 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	class AActor*                                      IgnoredActor;                                     		// 0x0020 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UClass*                                      ActorClassToIgnoreForDamage;                      		// 0x0024 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class AController*                                 InstigatedByController;                           		// 0x0028 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bDoFullDamage : 1;                                		// 0x002C (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	// class AActor*                                   Victim;                                           		// 0x0030 (0x0004) [0x0000000000000000]              
	// class AActor*                                   HitActor;                                         		// 0x0034 (0x0004) [0x0000000000000000]              
	// struct FVector                                  HitL;                                             		// 0x0038 (0x000C) [0x0000000000000000]              
	// struct FVector                                  HitN;                                             		// 0x0044 (0x000C) [0x0000000000000000]              
	// struct FVector                                  Dir;                                              		// 0x0050 (0x000C) [0x0000000000000000]              
	// unsigned long                                   bDamageBlocked : 1;                               		// 0x005C (0x0004) [0x0000000000000000] [0x00000001] 
	// float                                           ColRadius;                                        		// 0x0060 (0x0004) [0x0000000000000000]              
	// float                                           ColHeight;                                        		// 0x0064 (0x0004) [0x0000000000000000]              
	// float                                           DamageScale;                                      		// 0x0068 (0x0004) [0x0000000000000000]              
	// float                                           Dist;                                             		// 0x006C (0x0004) [0x0000000000000000]              
	// TArray< class AActor* >                         VictimsList;                                      		// 0x0070 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// struct FBox                                     BBox;                                             		// 0x007C (0x001C) [0x0000000000000000]              
	// struct FVector                                  BBoxCenter;                                       		// 0x0098 (0x000C) [0x0000000000000000]              
	// class AController*                              ModInstigator;                                    		// 0x00A4 (0x0004) [0x0000000000000000]              
	// class APawn*                                    VictimPawn;                                       		// 0x00A8 (0x0004) [0x0000000000000000]              
	// class UClass*                                   ModDamageType;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.DoFullDamageToActor
// [0x00020102] 
struct AGameExplosionActor_execDoFullDamageToActor_Parms
{
	class AActor*                                      Victim;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function GameFramework.GameExplosionActor.GetPhysicalMaterial
// [0x00880102] 
struct AGameExplosionActor_execGetPhysicalMaterial_Parms
{
	class UPhysicalMaterial*                           ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UPhysicalMaterial*                        retval;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// struct FVector                                  TraceStart;                                       		// 0x0008 (0x000C) [0x0000000000000000]              
	// struct FVector                                  TraceDest;                                        		// 0x0014 (0x000C) [0x0000000000000000]              
	// struct FVector                                  OutHitNorm;                                       		// 0x0020 (0x000C) [0x0000000000000000]              
	// struct FVector                                  ExploNormal;                                      		// 0x002C (0x000C) [0x0000000000000000]              
	// struct FTraceHitInfo                            OutHitInfo;                                       		// 0x0038 (0x001C) [0x0000000000000000]              
};

// Function GameFramework.GameExplosionActor.PreBeginPlay
// [0x00020802] ( FUNC_Event )
struct AGameExplosionActor_eventPreBeginPlay_Parms
{
};

// Function GameFramework.GameHUD.StopDrawingChapterTitle
// [0x00020102] 
struct AGameHUD_execStopDrawingChapterTitle_Parms
{
};

// Function GameFramework.GameHUD.StartDrawingChapterTitle
// [0x00020102] 
struct AGameHUD_execStartDrawingChapterTitle_Parms
{
	struct FString                                     ChapterName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     ActName;                                          		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	float                                              TotalDrawTime;                                    		// 0x0018 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              TotalFadeTime;                                    		// 0x001C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function GameFramework.GameHUD.SetHUDDrawColor
// [0x00024002] 
struct AGameHUD_execSetHUDDrawColor_Parms
{
	unsigned char                                      eColor;                                           		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      Alpha;                                            		// 0x0001 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function GameFramework.GamePawn.Cringe
// [0x00024800] ( FUNC_Event )
struct AGamePawn_eventCringe_Parms
{
	float                                              Duration;                                         		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function GameFramework.GamePawn.ServerKnockdown
// [0x002240C0] 
struct AGamePawn_execServerKnockdown_Parms
{
	struct FVector                                     RBLinearVelocity;                                 		// 0x0000 (0x000C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FVector                                     RBAngularVelocity;                                		// 0x000C (0x000C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FVector                                     RadialOrigin;                                     		// 0x0018 (0x000C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	float                                              RadialRadius;                                     		// 0x0024 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	float                                              RadialStrength;                                   		// 0x0028 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FVector                                     PointImpulse;                                     		// 0x002C (0x000C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FVector                                     PointImpulsePosition;                             		// 0x0038 (0x000C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FName                                       PointImpulseBoneName;                             		// 0x0044 (0x0008) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function GameFramework.GamePawn.GetTargetFrictionCylinder
// [0x00420102] 
struct AGamePawn_execGetTargetFrictionCylinder_Parms
{
	float                                              CylinderRadius;                                   		// 0x0000 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	float                                              CylinderHeight;                                   		// 0x0004 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif