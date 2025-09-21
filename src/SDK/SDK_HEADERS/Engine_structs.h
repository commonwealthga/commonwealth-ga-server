/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: Engine_structs.h
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
# Script Structs
# ========================================================================================= #
*/

// ScriptStruct Engine.Actor.TimerData
// 0x001C
struct FTimerData
{
	unsigned long                                      bLoop : 1;                                        		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FName                                       FuncName;                                         		// 0x0004 (0x0008) [0x0000000000000000]              
	float                                              Rate;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              Count;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	class UObject*                                     TimerObj;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	unsigned long                                      bPaused : 1;                                      		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.Actor.TraceHitInfo
// 0x001C
struct FTraceHitInfo
{
	class UMaterial*                                   Material;                                         		// 0x0000 (0x0004) [0x0000000000100000]              
	class UPhysicalMaterial*                           PhysMaterial;                                     		// 0x0004 (0x0004) [0x0000000000100000]              
	int                                                Item;                                             		// 0x0008 (0x0004) [0x0000000000100000]              
	int                                                LevelIndex;                                       		// 0x000C (0x0004) [0x0000000000100000]              
	struct FName                                       BoneName;                                         		// 0x0010 (0x0008) [0x0000000000100000]              
	class UPrimitiveComponent*                         HitComponent;                                     		// 0x0018 (0x0004) [0x0000000004180008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
};

// ScriptStruct Engine.Actor.ImpactInfo
// 0x0060
struct FImpactInfo
{
	class AActor*                                      HitActor;                                         		// 0x0000 (0x0004) [0x0000000000100000]              
	struct FVector                                     HitLocation;                                      		// 0x0004 (0x000C) [0x0000000000100000]              
	struct FVector                                     HitNormal;                                        		// 0x0010 (0x000C) [0x0000000000100000]              
	struct FVector                                     RayDir;                                           		// 0x001C (0x000C) [0x0000000000100000]              
	struct FVector                                     StartTrace;                                       		// 0x0028 (0x000C) [0x0000000000100000]              
	struct FTraceHitInfo                               HitInfo;                                          		// 0x0034 (0x001C) [0x0000000000180000]              ( CPF_Component )
	float                                              PercAbsorbedDamage;                               		// 0x0050 (0x0004) [0x0000000000100000]              
	class UObject*                                     DeviceModeReference;                              		// 0x0054 (0x0004) [0x0000000000100000]              
	unsigned long                                      bDirectHit : 1;                                   		// 0x0058 (0x0004) [0x0000000000100000] [0x00000001] 
	float                                              PctDamageDecreaseDueToWeakSpot;                   		// 0x005C (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.Actor.AnimSlotInfo
// 0x0014
struct FAnimSlotInfo
{
	struct FName                                       SlotName;                                         		// 0x0000 (0x0008) [0x0000000000100000]              
	TArray< float >                                    ChannelWeights;                                   		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.Actor.AnimSlotDesc
// 0x000C
struct FAnimSlotDesc
{
	struct FName                                       SlotName;                                         		// 0x0000 (0x0008) [0x0000000000100000]              
	int                                                NumChannels;                                      		// 0x0008 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.Actor.RigidBodyState
// 0x0035
struct FRigidBodyState
{
	struct FVector                                     Position;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FQuat                                       Quaternion;                                       		// 0x000C (0x0010) [0x0000000000000000]              
	struct FVector                                     LinVel;                                           		// 0x001C (0x000C) [0x0000000000000000]              
	struct FVector                                     AngVel;                                           		// 0x0028 (0x000C) [0x0000000000000000]              
	unsigned char                                      bNewData;                                         		// 0x0034 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.Actor.RigidBodyContactInfo
// 0x003C
struct FRigidBodyContactInfo
{
	struct FVector                                     ContactPosition;                                  		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FVector                                     ContactNormal;                                    		// 0x000C (0x000C) [0x0000000000000000]              
	float                                              ContactPenetration;                               		// 0x0018 (0x0004) [0x0000000000000000]              
	struct FVector                                     ContactVelocity[ 0x2 ];                           		// 0x001C (0x0018) [0x0000000000000000]              
	class UPhysicalMaterial*                           PhysMaterial[ 0x2 ];                              		// 0x0034 (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.Actor.CollisionImpactData
// 0x0024
struct FCollisionImpactData
{
	TArray< struct FRigidBodyContactInfo >             ContactInfos;                                     		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FVector                                     TotalNormalForceVector;                           		// 0x000C (0x000C) [0x0000000000000000]              
	struct FVector                                     TotalFrictionForceVector;                         		// 0x0018 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.Actor.ReplicatedHitImpulse
// 0x0028
struct FReplicatedHitImpulse
{
	struct FVector                                     AppliedImpulse;                                   		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FVector                                     HitLocation;                                      		// 0x000C (0x000C) [0x0000000000000000]              
	struct FName                                       BoneName;                                         		// 0x0018 (0x0008) [0x0000000000000000]              
	unsigned char                                      ImpulseCount;                                     		// 0x0020 (0x0001) [0x0000000000000000]              
	unsigned long                                      bRadialImpulse : 1;                               		// 0x0024 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.Actor.PhysEffectInfo
// 0x0010
struct FPhysEffectInfo
{
	float                                              Threshold;                                        		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              ReFireDelay;                                      		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UParticleSystem*                             Effect;                                           		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USoundCue*                                   Sound;                                            		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Actor.ActorReference
// 0x0014
struct FActorReference
{
	class AActor*                                      Actor;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       Guid;                                             		// 0x0004 (0x0010) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
};

// ScriptStruct Engine.Actor.NavReference
// 0x0014
struct FNavReference
{
	class ANavigationPoint*                            Nav;                                              		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       Guid;                                             		// 0x0004 (0x0010) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
};

// ScriptStruct Engine.Actor.TgLogoParameter
// 0x000C
struct FTgLogoParameter
{
	unsigned char                                      m_LogoType;                                       		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_nmParameterName;                                		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Actor.TgLogoInfo
// 0x0018
struct FTgLogoInfo
{
	TArray< int >                                      m_nMaterialIDs;                                   		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FTgLogoParameter >                  m_LogoParameters;                                 		// 0x000C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SequenceOp.SeqOpInputLink
// 0x0028
struct FSeqOpInputLink
{
	struct FString                                     LinkDesc;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bHasImpulse : 1;                                  		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                QueuedActivations;                                		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      bDisabled : 1;                                    		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bDisabledPIE : 1;                                 		// 0x0014 (0x0004) [0x0000000000000000] [0x00000002] 
	class USequenceOp*                                 LinkedOp;                                         		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                DrawY;                                            		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      bHidden : 1;                                      		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              ActivateDelay;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.AnimNode.BoneAtom
// 0x0020
struct FBoneAtom
{
	struct FQuat                                       Rotation;                                         		// 0x0000 (0x0010) [0x0000000000000000]              
	struct FVector                                     Translation;                                      		// 0x0010 (0x000C) [0x0000000000000000]              
	float                                              Scale;                                            		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PrimitiveComponent.MaterialViewRelevance
// 0x0004
struct FMaterialViewRelevance
{
	unsigned long                                      bOpaque : 1;                                      		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bMasked : 1;                                      		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bTranslucent : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bDistortion : 1;                                  		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bOneLayerDistortionRelevance : 1;                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      bLit : 1;                                         		// 0x0000 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      bUsesSceneColor : 1;                              		// 0x0000 (0x0004) [0x0000000000000000] [0x00000040] 
};

// ScriptStruct Engine.PrimitiveComponent.RBCollisionChannelContainer
// 0x0004
struct UPrimitiveComponent_FRBCollisionChannelContainer
{
	unsigned long                                      Default : 1;                                      		// 0x0000 (0x0004) [0x0000000000000003] [0x00000001] ( CPF_Edit | CPF_Const )
	unsigned long                                      Nothing : 1;                                      		// 0x0000 (0x0004) [0x0000000000000002] [0x00000002] ( CPF_Const )
	unsigned long                                      Pawn : 1;                                         		// 0x0000 (0x0004) [0x0000000000000003] [0x00000004] ( CPF_Edit | CPF_Const )
	unsigned long                                      Vehicle : 1;                                      		// 0x0000 (0x0004) [0x0000000000000003] [0x00000008] ( CPF_Edit | CPF_Const )
	unsigned long                                      Water : 1;                                        		// 0x0000 (0x0004) [0x0000000000000003] [0x00000010] ( CPF_Edit | CPF_Const )
	unsigned long                                      GameplayPhysics : 1;                              		// 0x0000 (0x0004) [0x0000000000000003] [0x00000020] ( CPF_Edit | CPF_Const )
	unsigned long                                      EffectPhysics : 1;                                		// 0x0000 (0x0004) [0x0000000000000003] [0x00000040] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled1 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000080] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled2 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000100] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled3 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000200] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled4 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000400] ( CPF_Edit | CPF_Const )
	unsigned long                                      Cloth : 1;                                        		// 0x0000 (0x0004) [0x0000000000000003] [0x00000800] ( CPF_Edit | CPF_Const )
	unsigned long                                      FluidDrain : 1;                                   		// 0x0000 (0x0004) [0x0000000000000003] [0x00001000] ( CPF_Edit | CPF_Const )
	unsigned long                                      SoftBody : 1;                                     		// 0x0000 (0x0004) [0x0000000000000003] [0x00002000] ( CPF_Edit | CPF_Const )
	unsigned long                                      FracturedMeshPart : 1;                            		// 0x0000 (0x0004) [0x0000000000000003] [0x00004000] ( CPF_Edit | CPF_Const )
	unsigned long                                      BlockingVolume : 1;                               		// 0x0000 (0x0004) [0x0000000000000003] [0x00008000] ( CPF_Edit | CPF_Const )
	unsigned long                                      DeadPawn : 1;                                     		// 0x0000 (0x0004) [0x0000000000000003] [0x00010000] ( CPF_Edit | CPF_Const )
};

// ScriptStruct Engine.LightComponent.LightingChannelContainer
// 0x0004
struct FLightingChannelContainer
{
	unsigned long                                      bInitialized : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      BSP : 1;                                          		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      Static : 1;                                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      Dynamic : 1;                                      		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      CompositeDynamic : 1;                             		// 0x0000 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
	unsigned long                                      Skybox : 1;                                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000020] ( CPF_Edit )
	unsigned long                                      Unnamed : 1;                                      		// 0x0000 (0x0004) [0x0000000000000001] [0x00000040] ( CPF_Edit )
	unsigned long                                      Unnamed01 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000080] ( CPF_Edit )
	unsigned long                                      Unnamed02 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000100] ( CPF_Edit )
	unsigned long                                      Unnamed03 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000200] ( CPF_Edit )
	unsigned long                                      Unnamed04 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000400] ( CPF_Edit )
	unsigned long                                      Unnamed05 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000800] ( CPF_Edit )
	unsigned long                                      Cinematic : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00001000] ( CPF_Edit )
	unsigned long                                      Cinematic01 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00002000] ( CPF_Edit )
	unsigned long                                      Cinematic02 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00004000] ( CPF_Edit )
	unsigned long                                      Cinematic03 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00008000] ( CPF_Edit )
	unsigned long                                      Cinematic04 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00010000] ( CPF_Edit )
	unsigned long                                      Cinematic05 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00020000] ( CPF_Edit )
	unsigned long                                      Cinematic06 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00040000] ( CPF_Edit )
	unsigned long                                      Cinematic07 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00080000] ( CPF_Edit )
	unsigned long                                      Cinematic08 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00100000] ( CPF_Edit )
	unsigned long                                      Cinematic09 : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00200000] ( CPF_Edit )
	unsigned long                                      Gameplay : 1;                                     		// 0x0000 (0x0004) [0x0000000000000001] [0x00400000] ( CPF_Edit )
	unsigned long                                      Gameplay01 : 1;                                   		// 0x0000 (0x0004) [0x0000000000000001] [0x00800000] ( CPF_Edit )
	unsigned long                                      Gameplay02 : 1;                                   		// 0x0000 (0x0004) [0x0000000000000001] [0x01000000] ( CPF_Edit )
	unsigned long                                      Gameplay03 : 1;                                   		// 0x0000 (0x0004) [0x0000000000000001] [0x02000000] ( CPF_Edit )
	unsigned long                                      Crowd : 1;                                        		// 0x0000 (0x0004) [0x0000000000000001] [0x04000000] ( CPF_Edit )
	unsigned long                                      CharacterBuilderHead : 1;                         		// 0x0000 (0x0004) [0x0000000000000001] [0x08000000] ( CPF_Edit )
};

// ScriptStruct Engine.Texture2D.Texture2DMipMap
// 0x003C
struct FTexture2DMipMap
{
	struct FUntypedBulkData_Mirror                     Data;                                             		// 0x0000 (0x0034) [0x0000000000001000]              ( CPF_Native )
	int                                                SizeX;                                            		// 0x0034 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                SizeY;                                            		// 0x0038 (0x0004) [0x0000000000001000]              ( CPF_Native )
};

// ScriptStruct Engine.Texture2D.TextureLinkedListMirror
// 0x000C
struct FTextureLinkedListMirror
{
	struct FPointer                                    Element;                                          		// 0x0000 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    Next;                                             		// 0x0004 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    PrevLink;                                         		// 0x0008 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.Info.KeyValuePair
// 0x0018
struct FKeyValuePair
{
	struct FString                                     Key;                                              		// 0x0000 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     Value;                                            		// 0x000C (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.Info.PlayerResponseLine
// 0x002C
struct FPlayerResponseLine
{
	int                                                PlayerNum;                                        		// 0x0000 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                PlayerID;                                         		// 0x0004 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	struct FString                                     PlayerName;                                       		// 0x0008 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	int                                                Ping;                                             		// 0x0014 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                Score;                                            		// 0x0018 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                StatsID;                                          		// 0x001C (0x0004) [0x0000000000100001]              ( CPF_Edit )
	TArray< struct FKeyValuePair >                     PlayerInfo;                                       		// 0x0020 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.Info.ServerResponseLine
// 0x0060
struct FServerResponseLine
{
	int                                                ServerID;                                         		// 0x0000 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	struct FString                                     IP;                                               		// 0x0004 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	int                                                Port;                                             		// 0x0010 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                QueryPort;                                        		// 0x0014 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	struct FString                                     ServerName;                                       		// 0x0018 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     MapName;                                          		// 0x0024 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     GameType;                                         		// 0x0030 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	int                                                CurrentPlayers;                                   		// 0x003C (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                MaxPlayers;                                       		// 0x0040 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	int                                                Ping;                                             		// 0x0044 (0x0004) [0x0000000000100001]              ( CPF_Edit )
	TArray< struct FKeyValuePair >                     ServerInfo;                                       		// 0x0048 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FPlayerResponseLine >               PlayerInfo;                                       		// 0x0054 (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.UniqueNetId
// 0x0008
struct FUniqueNetId
{
	struct FQWord                                      Uid;                                              		// 0x0000 (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineSubsystem.FriendsQuery
// 0x000C
struct FFriendsQuery
{
	struct FUniqueNetId                                UniqueId;                                         		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned long                                      bIsFriend : 1;                                    		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.OnlineSubsystem.OnlineFriend
// 0x0030
struct FOnlineFriend
{
	struct FUniqueNetId                                UniqueId;                                         		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FQWord                                      SessionId;                                        		// 0x0008 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     NickName;                                         		// 0x0010 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	struct FString                                     PresenceInfo;                                     		// 0x001C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	unsigned char                                      FriendState;                                      		// 0x0028 (0x0001) [0x0000000000000002]              ( CPF_Const )
	unsigned long                                      bIsOnline : 1;                                    		// 0x002C (0x0004) [0x0000000000000002] [0x00000001] ( CPF_Const )
	unsigned long                                      bIsPlaying : 1;                                   		// 0x002C (0x0004) [0x0000000000000002] [0x00000002] ( CPF_Const )
	unsigned long                                      bIsPlayingThisGame : 1;                           		// 0x002C (0x0004) [0x0000000000000002] [0x00000004] ( CPF_Const )
	unsigned long                                      bIsJoinable : 1;                                  		// 0x002C (0x0004) [0x0000000000000002] [0x00000008] ( CPF_Const )
	unsigned long                                      bHasVoiceSupport : 1;                             		// 0x002C (0x0004) [0x0000000000000002] [0x00000010] ( CPF_Const )
	unsigned long                                      bHaveInvited : 1;                                 		// 0x002C (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      bHasInvitedYou : 1;                               		// 0x002C (0x0004) [0x0000000000000002] [0x00000040] ( CPF_Const )
};

// ScriptStruct Engine.OnlineSubsystem.OnlineContent
// 0x0034
struct FOnlineContent
{
	int                                                UserIndex;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     FriendlyName;                                     		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     ContentPath;                                      		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           ContentPackages;                                  		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           ContentFiles;                                     		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.OnlineRegistrant
// 0x0008
struct FOnlineRegistrant
{
	struct FUniqueNetId                                PlayerNetId;                                      		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.OnlineSubsystem.NamedInterface
// 0x000C
struct FNamedInterface
{
	struct FName                                       InterfaceName;                                    		// 0x0000 (0x0008) [0x0000000000000000]              
	class UObject*                                     InterfaceObject;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineSubsystem.NamedInterfaceDef
// 0x0014
struct FNamedInterfaceDef
{
	struct FName                                       InterfaceName;                                    		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FString                                     InterfaceClassName;                               		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.OnlineArbitrationRegistrant
// 0x000C(0x0014 - 0x0008)
struct FOnlineArbitrationRegistrant : FOnlineRegistrant
{
	struct FQWord                                      MachineId;                                        		// 0x0008 (0x0008) [0x0000000000000002]              ( CPF_Const )
	int                                                Trustworthiness;                                  		// 0x0010 (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.OnlineSubsystem.SpeechRecognizedWord
// 0x0014
struct FSpeechRecognizedWord
{
	int                                                WordId;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     WordText;                                         		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              Confidence;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineSubsystem.OnlinePlayerScore
// 0x0010
struct FOnlinePlayerScore
{
	struct FUniqueNetId                                PlayerID;                                         		// 0x0000 (0x0008) [0x0000000000000000]              
	int                                                TeamID;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                Score;                                            		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineSubsystem.LocalTalker
// 0x0004
struct FLocalTalker
{
	unsigned long                                      bHasVoice : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bHasNetworkedVoice : 1;                           		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bIsRecognizingSpeech : 1;                         		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bWasTalking : 1;                                  		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bIsRegistered : 1;                                		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
};

// ScriptStruct Engine.OnlineSubsystem.RemoteTalker
// 0x000C
struct FRemoteTalker
{
	struct FUniqueNetId                                TalkerId;                                         		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned long                                      bWasTalking : 1;                                  		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bIsRegistered : 1;                                		// 0x0008 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.OnlineSubsystem.OnlineFriendMessage
// 0x0024
struct FOnlineFriendMessage
{
	struct FUniqueNetId                                SendingPlayerId;                                  		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FString                                     SendingPlayerNick;                                		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bIsFriendInvite : 1;                              		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bIsGameInvite : 1;                                		// 0x0014 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bWasAccepted : 1;                                 		// 0x0014 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bWasDenied : 1;                                   		// 0x0014 (0x0004) [0x0000000000000000] [0x00000008] 
	struct FString                                     Message;                                          		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.TitleFile
// 0x001C
struct FTitleFile
{
	struct FString                                     Filename;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned char                                      AsyncState;                                       		// 0x000C (0x0001) [0x0000000000000000]              
	TArray< unsigned char >                            Data;                                             		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.CommunityContentFile
// 0x0034
struct FCommunityContentFile
{
	int                                                ContentId;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                FileId;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                ContentType;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                FileSize;                                         		// 0x000C (0x0004) [0x0000000000000000]              
	struct FUniqueNetId                                Owner;                                            		// 0x0010 (0x0008) [0x0000000000000000]              
	int                                                DownloadCount;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	float                                              AverageRating;                                    		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                RatingCount;                                      		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                LastRatingGiven;                                  		// 0x0024 (0x0004) [0x0000000000000000]              
	struct FString                                     LocalFilePath;                                    		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.Settings.LocalizedStringSetting
// 0x0009
struct FLocalizedStringSetting
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                ValueIndex;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned char                                      AdvertisementType;                                		// 0x0008 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.Settings.SettingsData
// 0x000C
struct FSettingsData
{
	unsigned char                                      Type;                                             		// 0x0000 (0x0001) [0x0000000000000002]              ( CPF_Const )
	int                                                Value1;                                           		// 0x0004 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FPointer                                    Value2;                                           		// 0x0008 (0x0004) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
};

// ScriptStruct Engine.Settings.SettingsProperty
// 0x0011
struct FSettingsProperty
{
	int                                                PropertyId;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FSettingsData                               Data;                                             		// 0x0004 (0x000C) [0x0000000000000000]              
	unsigned char                                      AdvertisementType;                                		// 0x0010 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineSubsystem.CommunityContentMetadata
// 0x0010
struct FCommunityContentMetadata
{
	int                                                ContentType;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< struct FSettingsProperty >                 MetadataItems;                                    		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineSubsystem.NamedSession
// 0x0028
struct FNamedSession
{
	struct FName                                       SessionName;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FPointer                                    SessionInfo;                                      		// 0x0008 (0x0004) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
	class UOnlineGameSettings*                         GameSettings;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	TArray< struct FOnlineRegistrant >                 Registrants;                                      		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FOnlineArbitrationRegistrant >      ArbitrationRegistrants;                           		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.Controller.VisiblePortalInfo
// 0x0008
struct FVisiblePortalInfo
{
	class AActor*                                      Source;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class AActor*                                      Destination;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Controller.BasedPosition
// 0x0034
struct FBasedPosition
{
	class AActor*                                      Base;                                             		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Position;                                         		// 0x0004 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     CachedBaseLocation;                               		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FRotator                                    CachedBaseRotation;                               		// 0x001C (0x000C) [0x0000000000000000]              
	struct FVector                                     CachedTransPosition;                              		// 0x0028 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.PlayerController.InputEntry
// 0x000D
struct FInputEntry
{
	unsigned char                                      Type;                                             		// 0x0000 (0x0001) [0x0000000000000000]              
	float                                              Value;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              TimeDelta;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned char                                      Action;                                           		// 0x000C (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.PlayerController.InputMatchRequest
// 0x0030
struct FInputMatchRequest
{
	TArray< struct FInputEntry >                       Inputs;                                           		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class AActor*                                      MatchActor;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	struct FName                                       MatchFuncName;                                    		// 0x0010 (0x0008) [0x0000000000000000]              
	struct FName                                       FailedFuncName;                                   		// 0x0018 (0x0008) [0x0000000000000000]              
	struct FName                                       RequestName;                                      		// 0x0020 (0x0008) [0x0000000000000000]              
	int                                                MatchIdx;                                         		// 0x0028 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	float                                              LastMatchTime;                                    		// 0x002C (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.PlayerController.DebugTextInfo
// 0x0034
struct FDebugTextInfo
{
	class AActor*                                      SrcActor;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FVector                                     SrcActorOffset;                                   		// 0x0004 (0x000C) [0x0000000000000000]              
	struct FVector                                     SrcActorDesiredOffset;                            		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FString                                     DebugText;                                        		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              TimeRemaining;                                    		// 0x0028 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	float                                              Duration;                                         		// 0x002C (0x0004) [0x0000000000000000]              
	struct FColor                                      TextColor;                                        		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PlayerController.ClientAdjustment
// 0x0031
struct FClientAdjustment
{
	float                                              TimeStamp;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned char                                      newPhysics;                                       		// 0x0004 (0x0001) [0x0000000000000000]              
	struct FVector                                     NewLoc;                                           		// 0x0008 (0x000C) [0x0000000000000000]              
	struct FVector                                     NewVel;                                           		// 0x0014 (0x000C) [0x0000000000000000]              
	class AActor*                                      NewBase;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
	struct FVector                                     NewFloor;                                         		// 0x0024 (0x000C) [0x0000000000000000]              
	unsigned char                                      bAckGoodMove;                                     		// 0x0030 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.Camera.ViewTargetTransitionParams
// 0x000C
struct FViewTargetTransitionParams
{
	float                                              BlendTime;                                        		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      BlendFunction;                                    		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              BlendExp;                                         		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Camera.TViewTarget
// 0x002C
struct FTViewTarget
{
	class AActor*                                      Target;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AController*                                 Controller;                                       		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FTPOV                                       POV;                                              		// 0x0008 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	float                                              AspectRatio;                                      		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class APlayerReplicationInfo*                      PRI;                                              		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.PostProcessVolume.PostProcessSettings
// 0x0080
struct FPostProcessSettings
{
	unsigned long                                      bEnableBloom : 1;                                 		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bEnableDOF : 1;                                   		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bEnableMotionBlur : 1;                            		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bEnableSceneEffect : 1;                           		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      bAllowAmbientOcclusion : 1;                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
	float                                              Bloom_Scale;                                      		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Bloom_InterpolationDuration;                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_FalloffExponent;                              		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_BlurKernelSize;                               		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_MaxNearBlurAmount;                            		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_MaxFarBlurAmount;                             		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FColor                                      DOF_ModulateBlurColor;                            		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      DOF_FocusType;                                    		// 0x0020 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_FocusInnerRadius;                             		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_FocusDistance;                                		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     DOF_FocusPosition;                                		// 0x002C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              DOF_InterpolationDuration;                        		// 0x0038 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MotionBlur_MaxVelocity;                           		// 0x003C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MotionBlur_Amount;                                		// 0x0040 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      MotionBlur_FullMotionBlur : 1;                    		// 0x0044 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              MotionBlur_CameraRotationThreshold;               		// 0x0048 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MotionBlur_CameraTranslationThreshold;            		// 0x004C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MotionBlur_InterpolationDuration;                 		// 0x0050 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Scene_Desaturation;                               		// 0x0054 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Scene_HighLights;                                 		// 0x0058 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Scene_MidTones;                                   		// 0x0064 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Scene_Shadows;                                    		// 0x0070 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              Scene_InterpolationDuration;                      		// 0x007C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Camera.TCameraCache
// 0x0020
struct FTCameraCache
{
	float                                              TimeStamp;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FTPOV                                       POV;                                              		// 0x0004 (0x001C) [0x0000000000000000]              
};

// ScriptStruct Engine.PlayerReplicationInfo.AutomatedTestingDatum
// 0x0008
struct FAutomatedTestingDatum
{
	int                                                NumberOfMatchesPlayed;                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                NumMapListCyclesDone;                             		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineGameSearch.OnlineGameSearchResult
// 0x0008
struct FOnlineGameSearchResult
{
	class UOnlineGameSettings*                         GameSettings;                                     		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FPointer                                    PlatformData;                                     		// 0x0004 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.OnlineGameSearch.NamedObjectProperty
// 0x0014
struct FNamedObjectProperty
{
	struct FName                                       ObjectPropertyName;                               		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FString                                     ObjectPropertyValue;                              		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineGameSearch.OverrideSkill
// 0x0028
struct FOverrideSkill
{
	int                                                LeaderboardId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< struct FUniqueNetId >                      Players;                                          		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FDouble >                           Mus;                                              		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FDouble >                           Sigmas;                                           		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineGameSearch.OnlineGameSearchParameter
// 0x000E
struct FOnlineGameSearchParameter
{
	int                                                EntryId;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       ObjectPropertyName;                               		// 0x0004 (0x0008) [0x0000000000000000]              
	unsigned char                                      EntryType;                                        		// 0x000C (0x0001) [0x0000000000000000]              
	unsigned char                                      ComparisonType;                                   		// 0x000D (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineGameSearch.OnlineGameSearchSortClause
// 0x000E
struct FOnlineGameSearchSortClause
{
	int                                                EntryId;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       ObjectPropertyName;                               		// 0x0004 (0x0008) [0x0000000000000000]              
	unsigned char                                      EntryType;                                        		// 0x000C (0x0001) [0x0000000000000000]              
	unsigned char                                      SortType;                                         		// 0x000D (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineGameSearch.OnlineGameSearchORClause
// 0x000C
struct FOnlineGameSearchORClause
{
	TArray< struct FOnlineGameSearchParameter >        OrParams;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineGameSearch.OnlineGameSearchQuery
// 0x0018
struct FOnlineGameSearchQuery
{
	TArray< struct FOnlineGameSearchORClause >         OrClauses;                                        		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FOnlineGameSearchSortClause >       SortClauses;                                      		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.AudioComponent.AudioComponentParam
// 0x0010
struct FAudioComponentParam
{
	struct FName                                       ParamName;                                        		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              FloatParam;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USoundNodeWave*                              WaveParam;                                        		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundCue.SoundNodeEditorData
// 0x0008
struct FSoundNodeEditorData
{
	int                                                NodePosX;                                         		// 0x0000 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	int                                                NodePosY;                                         		// 0x0004 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.SoundNodeAmbient.AmbientSoundSlot
// 0x0010
struct FAmbientSoundSlot
{
	class USoundNodeWave*                              Wave;                                             		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              PitchScale;                                       		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              VolumeScale;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Weight;                                           		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AmbientSoundSimpleToggleable.CheckpointRecord
// 0x0004
struct AAmbientSoundSimpleToggleable_FCheckpointRecord
{
	unsigned long                                      bCurrentlyPlaying : 1;                            		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.AnimSequence.AnimNotifyEvent
// 0x0010
struct FAnimNotifyEvent
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UAnimNotify*                                 Notify;                                           		// 0x0004 (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	struct FName                                       Comment;                                          		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AnimSequence.RawAnimSequenceTrack
// 0x0024
struct FRawAnimSequenceTrack
{
	TArray< struct FVector >                           PosKeys;                                          		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FQuat >                             RotKeys;                                          		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< float >                                    KeyTimes;                                         		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.AnimSequence.TranslationTrack
// 0x0018
struct FTranslationTrack
{
	TArray< struct FVector >                           PosKeys;                                          		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< float >                                    Times;                                            		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.AnimSequence.RotationTrack
// 0x0018
struct FRotationTrack
{
	TArray< struct FQuat >                             RotKeys;                                          		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< float >                                    Times;                                            		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.AnimSequence.CompressedTrack
// 0x0030
struct FCompressedTrack
{
	TArray< unsigned char >                            ByteStream;                                       		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< float >                                    Times;                                            		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              Mins[ 0x3 ];                                      		// 0x0018 (0x000C) [0x0000000000000000]              
	float                                              Ranges[ 0x3 ];                                    		// 0x0024 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.AnimNodeBlendBase.AnimBlendChild
// 0x0044
struct FAnimBlendChild
{
	struct FName                                       Name;                                             		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UAnimNode*                                   Anim;                                             		// 0x0008 (0x0004) [0x0000000004400008]              ( CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	float                                              Weight;                                           		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              TotalWeight;                                      		// 0x0010 (0x0004) [0x0000000000000002]              ( CPF_Const )
	float                                              BlendWeight;                                      		// 0x0014 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	int                                                bHasRootMotion;                                   		// 0x0018 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FBoneAtom                                   RootMotion;                                       		// 0x001C (0x0020) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	unsigned long                                      bMirrorSkeleton : 1;                              		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bIsAdditive : 1;                                  		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                DrawY;                                            		// 0x0040 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.AnimNodeAimOffset.AimTransform
// 0x001C
struct FAimTransform
{
	struct FQuat                                       Quaternion;                                       		// 0x0000 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Translation;                                      		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AnimNodeAimOffset.AimComponent
// 0x0104
struct FAimComponent
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               LU;                                               		// 0x0008 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               LC;                                               		// 0x0024 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               LD;                                               		// 0x0040 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               CU;                                               		// 0x005C (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               CC;                                               		// 0x0078 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               CD;                                               		// 0x0094 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               RU;                                               		// 0x00B0 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               RC;                                               		// 0x00CC (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FAimTransform                               RD;                                               		// 0x00E8 (0x001C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AnimNodeAimOffset.AimOffsetProfile
// 0x0074
struct FAimOffsetProfile
{
	struct FName                                       ProfileName;                                      		// 0x0000 (0x0008) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
	struct FVector2D                                   HorizontalRange;                                  		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FVector2D                                   VerticalRange;                                    		// 0x0010 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FAimComponent >                     AimComponents;                                    		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FName                                       AnimName_LU;                                      		// 0x0024 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_LC;                                      		// 0x002C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_LD;                                      		// 0x0034 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_CU;                                      		// 0x003C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_CC;                                      		// 0x0044 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_CD;                                      		// 0x004C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_RU;                                      		// 0x0054 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_RC;                                      		// 0x005C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName_RD;                                      		// 0x0064 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_nmPrefix;                                       		// 0x006C (0x0008) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
};

// ScriptStruct Engine.AnimNodeBlendMultiBone.ChildBoneBlendInfo
// 0x0030
struct FChildBoneBlendInfo
{
	TArray< float >                                    TargetPerBoneWeight;                              		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FName                                       InitTargetStartBone;                              		// 0x000C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              InitPerBoneIncrease;                              		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       OldStartBone;                                     		// 0x0018 (0x0008) [0x0000000000000002]              ( CPF_Const )
	float                                              OldBoneIncrease;                                  		// 0x0020 (0x0004) [0x0000000000000002]              ( CPF_Const )
	TArray< unsigned char >                            TargetRequiredBones;                              		// 0x0024 (0x000C) [0x0000000000402000]              ( CPF_Transient | CPF_NeedCtorLink )
};

// ScriptStruct Engine.AnimNodeRandom.RandomAnimInfo
// 0x0019
struct FRandomAnimInfo
{
	float                                              Chance;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      LoopCountMin;                                     		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      LoopCountMax;                                     		// 0x0005 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              BlendInTime;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector2D                                   PlayRateRange;                                    		// 0x000C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bStillFrame : 1;                                  		// 0x0014 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned char                                      LoopCount;                                        		// 0x0018 (0x0001) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.AnimNodeSequenceBlendBase.AnimInfo
// 0x0010
struct FAnimInfo
{
	struct FName                                       AnimSeqName;                                      		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
	class UAnimSequence*                               AnimSeq;                                          		// 0x0008 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	int                                                AnimLinkupIndex;                                  		// 0x000C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.AnimNodeSequenceBlendBase.AnimBlendInfo
// 0x001C
struct FAnimBlendInfo
{
	struct FName                                       AnimName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FAnimInfo                                   AnimInfo;                                         		// 0x0008 (0x0010) [0x0000000000000000]              
	float                                              Weight;                                           		// 0x0018 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.AnimNodeSynch.SynchGroup
// 0x0020
struct FSynchGroup
{
	TArray< class UAnimNodeSequence* >                 SeqNodes;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UAnimNodeSequence*                           MasterNode;                                       		// 0x000C (0x0004) [0x0000000000002000]              ( CPF_Transient )
	struct FName                                       GroupName;                                        		// 0x0010 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bFireSlaveNotifies : 1;                           		// 0x0018 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              RateScale;                                        		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AnimSet.AnimSetMeshLinkup
// 0x0034
struct FAnimSetMeshLinkup
{
	struct FGuid                                       SkelMeshLinkupGUID;                               		// 0x0000 (0x0010) [0x0000000000000000]              
	TArray< int >                                      BoneToTrackTable;                                 		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            BoneUseAnimTranslation;                           		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            ForceUseMeshTranslation;                          		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.AnimTree.AnimGroup
// 0x0024
struct FAnimGroup
{
	TArray< class UAnimNodeSequence* >                 SeqNodes;                                         		// 0x0000 (0x000C) [0x0000000000402002]              ( CPF_Const | CPF_Transient | CPF_NeedCtorLink )
	class UAnimNodeSequence*                           SynchMaster;                                      		// 0x000C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	class UAnimNodeSequence*                           NotifyMaster;                                     		// 0x0010 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FName                                       GroupName;                                        		// 0x0014 (0x0008) [0x0000000000000003]              ( CPF_Edit | CPF_Const )
	float                                              RateScale;                                        		// 0x001C (0x0004) [0x0000000000000003]              ( CPF_Edit | CPF_Const )
	float                                              SynchPctPosition;                                 		// 0x0020 (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.AnimTree.SkelControlListHead
// 0x0010
struct FSkelControlListHead
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000000]              
	class USkelControlBase*                            ControlHead;                                      		// 0x0008 (0x0004) [0x0000000004400008]              ( CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	int                                                DrawY;                                            		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.AnimTree.HelmetMorphRestrictions
// 0x000C
struct FHelmetMorphRestrictions
{
	struct FName                                       MorphName;                                        		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              Weight;                                           		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AudioDevice.Listener
// 0x0034
struct FListener
{
	class APortalVolume*                               PortalVolume;                                     		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FVector                                     Location;                                         		// 0x0004 (0x000C) [0x0000000000000000]              
	struct FVector                                     Up;                                               		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FVector                                     Right;                                            		// 0x001C (0x000C) [0x0000000000000000]              
	struct FVector                                     Front;                                            		// 0x0028 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.AudioDevice.SoundClassProperties
// 0x001C
struct FSoundClassProperties
{
	float                                              Volume;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Pitch;                                            		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              VoiceCenterChannelVolume;                         		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              VoiceRadioVolume;                                 		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bApplyEffects : 1;                                		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bAlwaysPlay : 1;                                  		// 0x0010 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bIsUISound : 1;                                   		// 0x0010 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bIsMusic : 1;                                     		// 0x0010 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      bNoReverb : 1;                                    		// 0x0010 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
	unsigned long                                      bBleedStereo : 1;                                 		// 0x0010 (0x0004) [0x0000000000000001] [0x00000020] ( CPF_Edit )
	struct FName                                       m_nmSoundMode;                                    		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.AudioDevice.SoundClass
// 0x0030
struct FSoundClass
{
	struct FSoundClassProperties                       Properties;                                       		// 0x0000 (0x001C) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       Name;                                             		// 0x001C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FName >                             ChildClassNames;                                  		// 0x0024 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.AudioDevice.AudioClassInfo
// 0x0010
struct FAudioClassInfo
{
	int                                                NumResident;                                      		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	int                                                SizeResident;                                     		// 0x0004 (0x0004) [0x0000000000000002]              ( CPF_Const )
	int                                                NumRealTime;                                      		// 0x0008 (0x0004) [0x0000000000000002]              ( CPF_Const )
	int                                                SizeRealTime;                                     		// 0x000C (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.NavigationPoint.DebugNavCost
// 0x0010
struct FDebugNavCost
{
	struct FString                                     Desc;                                             		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                Cost;                                             		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.NavigationPoint.NavigationOctreeObject
// 0x0031
struct FNavigationOctreeObject
{
	struct FBox                                        BoundingBox;                                      		// 0x0000 (0x001C) [0x0000000000000000]              
	struct FVector                                     BoxCenter;                                        		// 0x001C (0x000C) [0x0000000000000000]              
	struct FPointer                                    OctreeNode;                                       		// 0x0028 (0x0004) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
	class UObject*                                     Owner;                                            		// 0x002C (0x0004) [0x0000000000800002]              ( CPF_Const | CPF_NoExport )
	unsigned char                                      OwnerType;                                        		// 0x0030 (0x0001) [0x0000000000800002]              ( CPF_Const | CPF_NoExport )
};

// ScriptStruct Engine.NavigationPoint.CheckpointRecord
// 0x0004
struct ANavigationPoint_FCheckpointRecord
{
	unsigned long                                      bDisabled : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bBlocked : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.KMeshProps.KSphereElem
// 0x0048
struct FKSphereElem
{
	struct FMatrix                                     TM;                                               		// 0x0000 (0x0040) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              Radius;                                           		// 0x0040 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bNoRBCollision : 1;                               		// 0x0044 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPerPolyShape : 1;                                		// 0x0044 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
};

// ScriptStruct Engine.KMeshProps.KBoxElem
// 0x0050
struct FKBoxElem
{
	struct FMatrix                                     TM;                                               		// 0x0000 (0x0040) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              X;                                                		// 0x0040 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              Y;                                                		// 0x0044 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              Z;                                                		// 0x0048 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bNoRBCollision : 1;                               		// 0x004C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPerPolyShape : 1;                                		// 0x004C (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
};

// ScriptStruct Engine.KMeshProps.KSphylElem
// 0x004C
struct FKSphylElem
{
	struct FMatrix                                     TM;                                               		// 0x0000 (0x0040) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              Radius;                                           		// 0x0040 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	float                                              Length;                                           		// 0x0044 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bNoRBCollision : 1;                               		// 0x0048 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPerPolyShape : 1;                                		// 0x0048 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
};

// ScriptStruct Engine.KMeshProps.KConvexElem
// 0x0064
struct FKConvexElem
{
	TArray< struct FVector >                           VertexData;                                       		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FPlane >                            PermutedVertexData;                               		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      FaceTriData;                                      		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FVector >                           EdgeDirections;                                   		// 0x0024 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FVector >                           FaceNormalDirections;                             		// 0x0030 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FPlane >                            FacePlaneData;                                    		// 0x003C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FBox                                        ElemBox;                                          		// 0x0048 (0x001C) [0x0000000000000000]              
};

// ScriptStruct Engine.KMeshProps.KAggregateGeom
// 0x0038
struct FKAggregateGeom
{
	TArray< struct FKSphereElem >                      SphereElems;                                      		// 0x0000 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
	TArray< struct FKBoxElem >                         BoxElems;                                         		// 0x000C (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
	TArray< struct FKSphylElem >                       SphylElems;                                       		// 0x0018 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
	TArray< struct FKConvexElem >                      ConvexElems;                                      		// 0x0024 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
	struct FPointer                                    RenderInfo;                                       		// 0x0030 (0x0004) [0x0000000001001000]              ( CPF_Native )
	unsigned long                                      bSkipCloseAndParallelChecks : 1;                  		// 0x0034 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.BrushComponent.KCachedConvexData_Mirror
// 0x000C
struct FKCachedConvexData_Mirror
{
	TArray< int >                                      CachedConvexElements;                             		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.Brush.GeomSelection
// 0x0010
struct FGeomSelection
{
	int                                                Type;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Index;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                SelectionIndex;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              SelStrength;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.StaticMeshComponent.StaticMeshComponentLODInfo
// 0x001C
struct FStaticMeshComponentLODInfo
{
	TArray< class UShadowMap2D* >                      ShadowMaps;                                       		// 0x0000 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	TArray< class UObject* >                           ShadowVertexBuffers;                              		// 0x000C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	struct FPointer                                    LightMap;                                         		// 0x0018 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.Font.FontCharacter
// 0x0018
struct FFontCharacter
{
	int                                                StartU;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                StartV;                                           		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                USize;                                            		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                VSize;                                            		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      TextureIndex;                                     		// 0x0010 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	int                                                VerticalOffset;                                   		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.FontImportOptions.FontImportOptionsData
// 0x0088
struct FFontImportOptionsData
{
	struct FString                                     FontName;                                         		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	float                                              Height;                                           		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bEnableAntialiasing : 1;                          		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bEnableBold : 1;                                  		// 0x0010 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bEnableItalic : 1;                                		// 0x0010 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bEnableUnderline : 1;                             		// 0x0010 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned char                                      CharacterSet;                                     		// 0x0014 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FString                                     Chars;                                            		// 0x0018 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     UnicodeRange;                                     		// 0x0024 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     CharsFilePath;                                    		// 0x0030 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     CharsFileWildcard;                                		// 0x003C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	unsigned long                                      bCreatePrintableOnly : 1;                         		// 0x0048 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bIncludeASCIIRange : 1;                           		// 0x0048 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	struct FLinearColor                                ForegroundColor;                                  		// 0x004C (0x0010) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bEnableDropShadow : 1;                            		// 0x005C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	int                                                TexturePageWidth;                                 		// 0x0060 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                TexturePageMaxHeight;                             		// 0x0064 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                XPadding;                                         		// 0x0068 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                YPadding;                                         		// 0x006C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ExtendBoxTop;                                     		// 0x0070 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ExtendBoxBottom;                                  		// 0x0074 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ExtendBoxRight;                                   		// 0x0078 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ExtendBoxLeft;                                    		// 0x007C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bEnableLegacyMode : 1;                            		// 0x0080 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	int                                                Kerning;                                          		// 0x0084 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Canvas.CanvasIcon
// 0x0014
struct FCanvasIcon
{
	class UTexture*                                    Texture;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              U;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              V;                                                		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              UL;                                               		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              VL;                                               		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIRoot.WIDGET_ID
// 0x0000(0x0010 - 0x0010)
struct FWIDGET_ID : FGuid
{
};

// ScriptStruct Engine.UIRoot.STYLE_ID
// 0x0000(0x0010 - 0x0010)
struct FSTYLE_ID : FGuid
{
};

// ScriptStruct Engine.UIRoot.UIRangeData
// 0x0014
struct FUIRangeData
{
	float                                              CurrentValue;                                     		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinValue;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxValue;                                         		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              NudgeValue;                                       		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bIntRange : 1;                                    		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.TextureCoordinates
// 0x0010
struct FTextureCoordinates
{
	float                                              U;                                                		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              V;                                                		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              UL;                                               		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              VL;                                               		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIProviderScriptFieldValue
// 0x0054
struct FUIProviderScriptFieldValue
{
	struct FName                                       PropertyTag;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned char                                      PropertyType;                                     		// 0x0008 (0x0001) [0x0000000000000000]              
	struct FString                                     StringValue;                                      		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class USurface*                                    ImageValue;                                       		// 0x0018 (0x0004) [0x0000000000000000]              
	TArray< int >                                      ArrayValue;                                       		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FUIRangeData                                RangeValue;                                       		// 0x0028 (0x0014) [0x0000000000000000]              
	struct FUniqueNetId                                NetIdValue;                                       		// 0x003C (0x0008) [0x0000000000000000]              
	struct FTextureCoordinates                         AtlasCoordinates;                                 		// 0x0044 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.UIRoot.UIProviderFieldValue
// 0x0004(0x0058 - 0x0054)
struct FUIProviderFieldValue : FUIProviderScriptFieldValue
{
	struct FPointer                                    CustomStringNode;                                 		// 0x0054 (0x0004) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIStyleReference
// 0x0020
struct FUIStyleReference
{
	struct FName                                       DefaultStyleTag;                                  		// 0x0000 (0x0008) [0x0000000000000000]              
	class UClass*                                      RequiredStyleClass;                               		// 0x0008 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FSTYLE_ID                                   AssignedStyleID;                                  		// 0x000C (0x0010) [0x0000000000000002]              ( CPF_Const )
	class UUIStyle*                                    ResolvedStyle;                                    		// 0x001C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIScreenValue
// 0x0006
struct FUIScreenValue
{
	float                                              Value;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ScaleType;                                        		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      Orientation;                                      		// 0x0005 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIScreenValue_Extent
// 0x0006
struct FUIScreenValue_Extent
{
	float                                              Value;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ScaleType;                                        		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      Orientation;                                      		// 0x0005 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIScreenValue_Position
// 0x000A
struct FUIScreenValue_Position
{
	float                                              Value[ 0x2 ];                                     		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ScaleType[ 0x2 ];                                 		// 0x0008 (0x0002) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIScreenValue_Bounds
// 0x0019
struct FUIScreenValue_Bounds
{
	float                                              Value[ 0x4 ];                                     		// 0x0000 (0x0010) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      ScaleType[ 0x4 ];                                 		// 0x0010 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      bInvalidated[ 0x4 ];                              		// 0x0014 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	unsigned char                                      AspectRatioMode;                                  		// 0x0018 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIAnchorPosition
// 0x0006(0x0010 - 0x000A)
struct FUIAnchorPosition : FUIScreenValue_Position
{
	float                                              ZDepth;                                           		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.ScreenPositionRange
// 0x0000(0x000A - 0x000A)
struct FScreenPositionRange : FUIScreenValue_Position
{
};

// ScriptStruct Engine.UIRoot.UIScreenValue_DockPadding
// 0x0014
struct FUIScreenValue_DockPadding
{
	float                                              PaddingValue[ 0x4 ];                              		// 0x0000 (0x0010) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      PaddingScaleType[ 0x4 ];                          		// 0x0010 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
};

// ScriptStruct Engine.UIRoot.UIScreenValue_AutoSizeRegion
// 0x000A
struct FUIScreenValue_AutoSizeRegion
{
	float                                              Value[ 0x2 ];                                     		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      EvalType[ 0x2 ];                                  		// 0x0008 (0x0002) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.AutoSizePadding
// 0x0000(0x000A - 0x000A)
struct FAutoSizePadding : FUIScreenValue_AutoSizeRegion
{
};

// ScriptStruct Engine.UIRoot.AutoSizeData
// 0x001C
struct FAutoSizeData
{
	struct FUIScreenValue_AutoSizeRegion               Extent;                                           		// 0x0000 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FAutoSizePadding                            Padding;                                          		// 0x000C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bAutoSizeEnabled : 1;                             		// 0x0018 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIRenderingSubregion
// 0x0018
struct FUIRenderingSubregion
{
	struct FUIScreenValue_Extent                       ClampRegionSize;                                  		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FUIScreenValue_Extent                       ClampRegionOffset;                                		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ClampRegionAlignment;                             		// 0x0010 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bSubregionEnabled : 1;                            		// 0x0014 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.InputEventSubscription
// 0x0014
struct FInputEventSubscription
{
	struct FName                                       KeyName;                                          		// 0x0000 (0x0008) [0x0000000000100000]              
	TArray< class UUIScreenObject* >                   Subscribers;                                      		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIRoot.DefaultEventSpecification
// 0x0008
struct FDefaultEventSpecification
{
	class UUIEvent*                                    EventTemplate;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UClass*                                      EventState;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.SequenceOp.SeqOpOutputInputLink
// 0x0008
struct FSeqOpOutputInputLink
{
	class USequenceOp*                                 LinkedOp;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                InputLinkIdx;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.SequenceOp.SeqOpOutputLink
// 0x002C
struct FSeqOpOutputLink
{
	TArray< struct FSeqOpOutputInputLink >             Links;                                            		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     LinkDesc;                                         		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bHasImpulse : 1;                                  		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bDisabled : 1;                                    		// 0x0018 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bDisabledPIE : 1;                                 		// 0x0018 (0x0004) [0x0000000000000000] [0x00000004] 
	class USequenceOp*                                 LinkedOp;                                         		// 0x001C (0x0004) [0x0000000000000000]              
	float                                              ActivateDelay;                                    		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                DrawY;                                            		// 0x0024 (0x0004) [0x0000000000000000]              
	unsigned long                                      bHidden : 1;                                      		// 0x0028 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.SequenceOp.SeqVarLink
// 0x0040
struct FSeqVarLink
{
	class UClass*                                      ExpectedType;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< class USequenceVariable* >                 LinkedVariables;                                  		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     LinkDesc;                                         		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FName                                       LinkVar;                                          		// 0x001C (0x0008) [0x0000000000000000]              
	struct FName                                       PropertyName;                                     		// 0x0024 (0x0008) [0x0000000000000000]              
	unsigned long                                      bWriteable : 1;                                   		// 0x002C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bModifiesLinkedObject : 1;                        		// 0x002C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bHidden : 1;                                      		// 0x002C (0x0004) [0x0000000000000000] [0x00000004] 
	int                                                MinVars;                                          		// 0x0030 (0x0004) [0x0000000000000000]              
	int                                                MaxVars;                                          		// 0x0034 (0x0004) [0x0000000000000000]              
	int                                                DrawX;                                            		// 0x0038 (0x0004) [0x0000000000000000]              
	class UProperty*                                   CachedProperty;                                   		// 0x003C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.SequenceOp.SeqEventLink
// 0x0024
struct FSeqEventLink
{
	class UClass*                                      ExpectedType;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< class USequenceEvent* >                    LinkedEvents;                                     		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     LinkDesc;                                         		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                DrawX;                                            		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      bHidden : 1;                                      		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.UIRoot.InputKeyAction
// 0x0024
struct FInputKeyAction
{
	struct FName                                       InputKeyName;                                     		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      InputKeyState;                                    		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FSeqOpOutputInputLink >             TriggeredOps;                                     		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< class USequenceOp* >                       ActionsToExecute;                                 		// 0x0018 (0x000C) [0x0000000020400000]              ( CPF_NeedCtorLink | CPF_Deprecated )
};

// ScriptStruct Engine.UIRoot.StateInputKeyAction
// 0x0004(0x0028 - 0x0024)
struct FStateInputKeyAction : FInputKeyAction
{
	class UClass*                                      Scope;                                            		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.PlayerInteractionData
// 0x0008
struct FPlayerInteractionData
{
	class UUIObject*                                   FocusedControl;                                   		// 0x0000 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	class UUIObject*                                   LastFocusedControl;                               		// 0x0004 (0x0004) [0x0000000000102000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIFocusPropagationData
// 0x0014
struct FUIFocusPropagationData
{
	class UUIObject*                                   FirstFocusTarget;                                 		// 0x0000 (0x0004) [0x0000000000022003]              ( CPF_Edit | CPF_Const | CPF_Transient | CPF_EditConst )
	class UUIObject*                                   LastFocusTarget;                                  		// 0x0004 (0x0004) [0x0000000000022003]              ( CPF_Edit | CPF_Const | CPF_Transient | CPF_EditConst )
	class UUIObject*                                   NextFocusTarget;                                  		// 0x0008 (0x0004) [0x0000000000022003]              ( CPF_Edit | CPF_Const | CPF_Transient | CPF_EditConst )
	class UUIObject*                                   PrevFocusTarget;                                  		// 0x000C (0x0004) [0x0000000000022003]              ( CPF_Edit | CPF_Const | CPF_Transient | CPF_EditConst )
	unsigned long                                      bPendingReceiveFocus : 1;                         		// 0x0010 (0x0004) [0x0000000000002000] [0x00000001] ( CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UINavigationData
// 0x0024
struct FUINavigationData
{
	class UUIObject*                                   NavigationTarget[ 0x4 ];                          		// 0x0000 (0x0010) [0x0000000000022001]              ( CPF_Edit | CPF_Transient | CPF_EditConst )
	class UUIObject*                                   ForcedNavigationTarget[ 0x4 ];                    		// 0x0010 (0x0010) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      bNullOverride[ 0x4 ];                             		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIDockingSet
// 0x0038
struct FUIDockingSet
{
	class UUIObject*                                   OwnerWidget;                                      		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UUIObject*                                   TargetWidget[ 0x4 ];                              		// 0x0004 (0x0010) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	struct FUIScreenValue_DockPadding                  DockPadding;                                      		// 0x0014 (0x0014) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bLockWidthWhenDocked : 1;                         		// 0x0028 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bLockHeightWhenDocked : 1;                        		// 0x0028 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned char                                      TargetFace[ 0x4 ];                                		// 0x002C (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      bResolved[ 0x4 ];                                 		// 0x0030 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	unsigned char                                      bLinking[ 0x4 ];                                  		// 0x0034 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIDockingNode
// 0x0005
struct FUIDockingNode
{
	class UUIObject*                                   Widget;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      Face;                                             		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIRotation
// 0x0061
struct FUIRotation
{
	struct FRotator                                    Rotation;                                         		// 0x0000 (0x000C) [0x0000000000000003]              ( CPF_Edit | CPF_Const )
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x000C (0x0004) MISSED OFFSET
	struct FMatrix                                     TransformMatrix;                                  		// 0x0010 (0x0040) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FUIAnchorPosition                           AnchorPosition;                                   		// 0x0050 (0x0010) [0x0000000000000003]              ( CPF_Edit | CPF_Const )
	unsigned char                                      AnchorType;                                       		// 0x0060 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIDataStoreBinding
// 0x0030
struct FUIDataStoreBinding
{
	class UUIDataStoreSubscriber*                      Subscriber;                                       		// 0x0000 (0x0008) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x0004 (0x0004) FIX WRONG TYPE SIZE OF PREVIUS PROPERTY
	unsigned char                                      RequiredFieldType;                                		// 0x0008 (0x0001) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
	struct FString                                     MarkupString;                                     		// 0x000C (0x000C) [0x0000000000400003]              ( CPF_Edit | CPF_Const | CPF_NeedCtorLink )
	int                                                BindingIndex;                                     		// 0x0018 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FName                                       DataStoreName;                                    		// 0x001C (0x0008) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FName                                       DataStoreField;                                   		// 0x0024 (0x0008) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	class UUIDataStore*                                ResolvedDataStore;                                		// 0x002C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIStyleSubscriberReference
// 0x0010
struct FUIStyleSubscriberReference
{
	struct FName                                       SubscriberId;                                     		// 0x0000 (0x0008) [0x0000000000100000]              
	class UUIStyleResolver*                            Subscriber;                                       		// 0x0008 (0x0008) [0x0000000000100000]              
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x000C (0x0004) FIX WRONG TYPE SIZE OF PREVIUS PROPERTY
};

// ScriptStruct Engine.UIRoot.StyleReferenceId
// 0x000C
struct FStyleReferenceId
{
	struct FName                                       StyleReferenceTag;                                		// 0x0000 (0x0008) [0x0000000000100000]              
	class UProperty*                                   StyleProperty;                                    		// 0x0008 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.UIRoot.UITextAttributes
// 0x0004
struct FUITextAttributes
{
	unsigned long                                      Bold : 1;                                         		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      Italic : 1;                                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      Underline : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      Shadow : 1;                                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      Strikethrough : 1;                                		// 0x0000 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIImageAdjustmentData
// 0x0012
struct FUIImageAdjustmentData
{
	struct FUIScreenValue_Extent                       ProtectedRegion[ 0x2 ];                           		// 0x0000 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      AdjustmentType;                                   		// 0x0010 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      Alignment;                                        		// 0x0011 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIStringCaretParameters
// 0x0020
struct FUIStringCaretParameters
{
	unsigned long                                      bDisplayCaret : 1;                                		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned char                                      CaretType;                                        		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              CaretWidth;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       CaretStyle;                                       		// 0x000C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	int                                                CaretPosition;                                    		// 0x0014 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UMaterialInterface*                          CaretMaterial;                                    		// 0x0018 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	int                                                CaretLineIndex;                                   		// 0x001C (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIRoot.RenderParameters
// 0x0044
struct FRenderParameters
{
	float                                              DrawX;                                            		// 0x0000 (0x0004) [0x0000000000100000]              
	float                                              DrawY;                                            		// 0x0004 (0x0004) [0x0000000000100000]              
	float                                              DrawXL;                                           		// 0x0008 (0x0004) [0x0000000000100000]              
	float                                              DrawYL;                                           		// 0x000C (0x0004) [0x0000000000100000]              
	struct FVector2D                                   Scaling;                                          		// 0x0010 (0x0008) [0x0000000000100000]              
	class UFont*                                       DrawFont;                                         		// 0x0018 (0x0004) [0x0000000000100000]              
	unsigned char                                      TextAlignment[ 0x2 ];                             		// 0x001C (0x0002) [0x0000000000100000]              
	struct FVector2D                                   ImageExtent;                                      		// 0x0020 (0x0008) [0x0000000000100000]              
	struct FTextureCoordinates                         DrawCoords;                                       		// 0x0028 (0x0010) [0x0000000000100000]              
	struct FVector2D                                   SpacingAdjust;                                    		// 0x0038 (0x0008) [0x0000000000100000]              
	float                                              ViewportHeight;                                   		// 0x0040 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.UIRoot.TextAutoScaleValue
// 0x0005
struct FTextAutoScaleValue
{
	float                                              MinScale;                                         		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      AutoScaleMode;                                    		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIStyleOverride
// 0x0020
struct FUIStyleOverride
{
	struct FLinearColor                                DrawColor;                                        		// 0x0000 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	float                                              Opacity;                                          		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Padding[ 0x2 ];                                   		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bOverrideDrawColor : 1;                           		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bOverrideOpacity : 1;                             		// 0x001C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bOverridePadding : 1;                             		// 0x001C (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct Engine.UIRoot.UITextStyleOverride
// 0x0028(0x0048 - 0x0020)
struct FUITextStyleOverride : FUIStyleOverride
{
	class UFont*                                       DrawFont;                                         		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FUITextAttributes                           TextAttributes;                                   		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      TextAlignment[ 0x2 ];                             		// 0x0028 (0x0002) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ClipMode;                                         		// 0x002A (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ClipAlignment;                                    		// 0x002B (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FTextAutoScaleValue                         AutoScaling;                                      		// 0x002C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              DrawScale[ 0x2 ];                                 		// 0x0034 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              SpacingAdjust[ 0x2 ];                             		// 0x003C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bOverrideDrawFont : 1;                            		// 0x0044 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bOverrideAttributes : 1;                          		// 0x0044 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bOverrideAlignment : 1;                           		// 0x0044 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bOverrideClipMode : 1;                            		// 0x0044 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bOverrideClipAlignment : 1;                       		// 0x0044 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      bOverrideAutoScale : 1;                           		// 0x0044 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      bOverrideScale : 1;                               		// 0x0044 (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      bOverrideSpacingAdjust : 1;                       		// 0x0044 (0x0004) [0x0000000000000000] [0x00000080] 
};

// ScriptStruct Engine.UIRoot.UIImageStyleOverride
// 0x003C(0x005C - 0x0020)
struct FUIImageStyleOverride : FUIStyleOverride
{
	struct FTextureCoordinates                         Coordinates;                                      		// 0x0020 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FUIImageAdjustmentData                      Formatting[ 0x2 ];                                		// 0x0030 (0x0028) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bOverrideCoordinates : 1;                         		// 0x0058 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bOverrideFormatting : 1;                          		// 0x0058 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.UIRoot.UICombinedStyleData
// 0x0094
struct FUICombinedStyleData
{
	struct FLinearColor                                TextColor;                                        		// 0x0000 (0x0010) [0x0000000000100000]              
	struct FLinearColor                                ImageColor;                                       		// 0x0010 (0x0010) [0x0000000000100000]              
	float                                              TextPadding[ 0x2 ];                               		// 0x0020 (0x0008) [0x0000000000100000]              
	float                                              ImagePadding[ 0x2 ];                              		// 0x0028 (0x0008) [0x0000000000100000]              
	class UFont*                                       DrawFont;                                         		// 0x0030 (0x0004) [0x0000000000100000]              
	class USurface*                                    FallbackImage;                                    		// 0x0034 (0x0004) [0x0000000000100000]              
	struct FTextureCoordinates                         AtlasCoords;                                      		// 0x0038 (0x0010) [0x0000000000100000]              
	struct FUITextAttributes                           TextAttributes;                                   		// 0x0048 (0x0004) [0x0000000000100000]              
	unsigned char                                      TextAlignment[ 0x2 ];                             		// 0x004C (0x0002) [0x0000000000100000]              
	unsigned char                                      TextClipMode;                                     		// 0x004E (0x0001) [0x0000000000100000]              
	unsigned char                                      TextClipAlignment;                                		// 0x004F (0x0001) [0x0000000000100000]              
	struct FUIImageAdjustmentData                      AdjustmentType[ 0x2 ];                            		// 0x0050 (0x0028) [0x0000000000100000]              
	struct FTextAutoScaleValue                         TextAutoScaling;                                  		// 0x0078 (0x0008) [0x0000000000100000]              
	struct FVector2D                                   TextScale;                                        		// 0x0080 (0x0008) [0x0000000000100000]              
	struct FVector2D                                   TextSpacingAdjust;                                		// 0x0088 (0x0008) [0x0000000000100000]              
	unsigned long                                      bInitialized : 1;                                 		// 0x0090 (0x0004) [0x0000000000100002] [0x00000001] ( CPF_Const )
};

// ScriptStruct Engine.UIRoot.UIStringNodeModifier.ModifierData
// 0x0010
struct FModifierData
{
	class UUIStyle_Data*                               Style;                                            		// 0x0000 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	TArray< class UFont* >                             InlineFontStack;                                  		// 0x0004 (0x000C) [0x0000000000502002]              ( CPF_Const | CPF_Transient | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIRoot.UIStringNodeModifier
// 0x0138
struct FUIStringNodeModifier
{
	struct FUICombinedStyleData                        CustomStyleData;                                  		// 0x0000 (0x0094) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	struct FUICombinedStyleData                        BaseStyleData;                                    		// 0x0094 (0x0094) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	TArray< struct FModifierData >                     ModifierStack;                                    		// 0x0128 (0x000C) [0x0000000000502002]              ( CPF_Const | CPF_Transient | CPF_NeedCtorLink )
	class UUIState*                                    CurrentMenuState;                                 		// 0x0134 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIStringNode
// 0x002C
struct FUIStringNode
{
	struct FPointer                                    VfTable;                                          		// 0x0000 (0x0004) [0x0000000000903002]              ( CPF_Const | CPF_Native | CPF_Transient | CPF_NoExport )
	class UUIDataStore*                                NodeDataStore;                                    		// 0x0004 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	struct FPointer                                    ParentNode;                                       		// 0x0008 (0x0004) [0x0000000000103002]              ( CPF_Const | CPF_Native | CPF_Transient )
	struct FString                                     SourceText;                                       		// 0x000C (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FVector2D                                   Extent;                                           		// 0x0018 (0x0008) [0x0000000000100001]              ( CPF_Edit )
	struct FVector2D                                   Scaling;                                          		// 0x0020 (0x0008) [0x0000000000100001]              ( CPF_Edit )
	unsigned long                                      bForceWrap : 1;                                   		// 0x0028 (0x0004) [0x0000000000100000] [0x00000001] 
};

// ScriptStruct Engine.UIRoot.UIStringNode_Text
// 0x00A0(0x00CC - 0x002C)
struct FUIStringNode_Text : FUIStringNode
{
	struct FString                                     RenderedText;                                     		// 0x002C (0x000C) [0x0000000000500001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FUICombinedStyleData                        NodeStyleParameters;                              		// 0x0038 (0x0094) [0x0000000000100000]              
};

// ScriptStruct Engine.UIRoot.UIStringNode_Image
// 0x001C(0x0048 - 0x002C)
struct FUIStringNode_Image : FUIStringNode
{
	struct FVector2D                                   ForcedExtent;                                     		// 0x002C (0x0008) [0x0000000000100001]              ( CPF_Edit )
	struct FTextureCoordinates                         TexCoords;                                        		// 0x0034 (0x0010) [0x0000000000100001]              ( CPF_Edit )
	class UUITexture*                                  RenderedImage;                                    		// 0x0044 (0x0004) [0x0000000000100001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.UIStringNode_NestedMarkupParent
// 0x0000(0x002C - 0x002C)
struct FUIStringNode_NestedMarkupParent : FUIStringNode
{
};

// ScriptStruct Engine.UIRoot.UIStringNode_FormattedNodeParent
// 0x0000(0x00CC - 0x00CC)
struct FUIStringNode_FormattedNodeParent : FUIStringNode_Text
{
};

// ScriptStruct Engine.UIRoot.WrappedStringElement
// 0x0014
struct FWrappedStringElement
{
	struct FString                                     Value;                                            		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FVector2D                                   LineExtent;                                       		// 0x000C (0x0008) [0x0000000000100000]              
};

// ScriptStruct Engine.UIRoot.UIMouseCursor
// 0x000C
struct FUIMouseCursor
{
	struct FName                                       CursorStyle;                                      		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UUITexture*                                  Cursor;                                           		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIRoot.InputEventParameters
// 0x0024
struct FInputEventParameters
{
	class UUIScreenObject*                             UIObjectReference;                                		// 0x0000 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	int                                                ControllerId;                                     		// 0x0008 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	struct FName                                       InputKeyName;                                     		// 0x000C (0x0008) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	unsigned char                                      EventType;                                        		// 0x0014 (0x0001) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	float                                              InputDelta;                                       		// 0x0018 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	float                                              DeltaTime;                                        		// 0x001C (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	unsigned long                                      bAltPressed : 1;                                  		// 0x0020 (0x0004) [0x0000000000102002] [0x00000001] ( CPF_Const | CPF_Transient )
	unsigned long                                      bCtrlPressed : 1;                                 		// 0x0020 (0x0004) [0x0000000000102002] [0x00000002] ( CPF_Const | CPF_Transient )
	unsigned long                                      bShiftPressed : 1;                                		// 0x0020 (0x0004) [0x0000000000102002] [0x00000004] ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.SubscribedInputEventParameters
// 0x0008(0x002C - 0x0024)
struct FSubscribedInputEventParameters : FInputEventParameters
{
	struct FName                                       InputAliasName;                                   		// 0x0024 (0x0008) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIAxisEmulationDefinition
// 0x0024
struct FUIAxisEmulationDefinition
{
	struct FName                                       AxisInputKey;                                     		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FName                                       AdjacentAxisInputKey;                             		// 0x0008 (0x0008) [0x0000000000000000]              
	unsigned long                                      bEmulateButtonPress : 1;                          		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FName                                       InputKeyToEmulate[ 0x2 ];                         		// 0x0014 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.UIRoot.RawInputKeyEventData
// 0x0009
struct FRawInputKeyEventData
{
	struct FName                                       InputKeyName;                                     		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned char                                      ModifierKeyFlags;                                 		// 0x0008 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.UIRoot.UIInputActionAlias
// 0x0014
struct FUIInputActionAlias
{
	struct FName                                       InputAliasName;                                   		// 0x0000 (0x0008) [0x0000000000000000]              
	TArray< struct FRawInputKeyEventData >             LinkedInputKeys;                                  		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIRoot.UIInputAliasValue
// 0x000C
struct FUIInputAliasValue
{
	unsigned char                                      ModifierFlagMask;                                 		// 0x0000 (0x0001) [0x0000000000100000]              
	struct FName                                       InputAliasName;                                   		// 0x0004 (0x0008) [0x0000000000100000]              
};

// ScriptStruct Engine.UIRoot.UIInputAliasMap
// 0x003C
struct FUIInputAliasMap
{
	struct FMultiMap_Mirror                            InputAliasLookupTable;                            		// 0x0000 (0x003C) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
};

// ScriptStruct Engine.UIRoot.UIInputAliasStateMap
// 0x001C
struct FUIInputAliasStateMap
{
	struct FString                                     StateClassName;                                   		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UClass*                                      State;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	TArray< struct FUIInputActionAlias >               StateInputAliases;                                		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIRoot.UIInputAliasClassMap
// 0x0094
struct FUIInputAliasClassMap
{
	struct FString                                     WidgetClassName;                                  		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UClass*                                      WidgetClass;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	TArray< struct FUIInputAliasStateMap >             WidgetStates;                                     		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned char                                      UnknownData00[ 0x3C ];                            		// 0x001C (0x003C) UNKNOWN PROPERTY: MapProperty Engine.UIRoot.UIInputAliasClassMap.StateLookupTable
	unsigned char                                      UnknownData01[ 0x3C ];                            		// 0x0058 (0x003C) UNKNOWN PROPERTY: MapProperty Engine.UIRoot.UIInputAliasClassMap.StateReverseLookupTable
};

// ScriptStruct Engine.ReverbVolume.ReverbSettings
// 0x000C
struct FReverbSettings
{
	unsigned char                                      ReverbType;                                       		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              Volume;                                           		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FadeTime;                                         		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.NetViewer
// 0x0020
struct FNetViewer
{
	class APlayerController*                           InViewer;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class AActor*                                      Viewer;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FVector                                     ViewLocation;                                     		// 0x0008 (0x000C) [0x0000000000000000]              
	struct FVector                                     ViewDir;                                          		// 0x0014 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.MusicTrackDataStructures.MusicTrackStruct
// 0x0018
struct FMusicTrackStruct
{
	class USoundCue*                                   TheSoundCue;                                      		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bAutoPlay : 1;                                    		// 0x0004 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPersistentAcrossLevels : 1;                      		// 0x0004 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	float                                              FadeInTime;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FadeInVolumeLevel;                                		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FadeOutTime;                                      		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FadeOutVolumeLevel;                               		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.CompartmentRunList
// 0x0004
struct FCompartmentRunList
{
	unsigned long                                      RigidBody : 1;                                    		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      Fluid : 1;                                        		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      Cloth : 1;                                        		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      SoftBody : 1;                                     		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.ScreenMessageString
// 0x001C
struct FScreenMessageString
{
	int                                                Key;                                              		// 0x0000 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	struct FString                                     ScreenMessage;                                    		// 0x0004 (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	struct FColor                                      DisplayColor;                                     		// 0x0010 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	float                                              TimeToDisplay;                                    		// 0x0014 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	float                                              CurrentTimeDisplayed;                             		// 0x0018 (0x0004) [0x0000000000102000]              ( CPF_Transient )
};

// ScriptStruct Engine.WorldInfo.WorldFractureSettings
// 0x001C
struct FWorldFractureSettings
{
	float                                              ChanceOfPhysicsChunkOverride;                     		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned long                                      bEnableChanceOfPhysicsChunkOverride : 1;          		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bLimitExplosionChunkSize : 1;                     		// 0x0004 (0x0004) [0x0000000000000000] [0x00000002] 
	float                                              MaxExplosionChunkSize;                            		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned long                                      bLimitDamageChunkSize : 1;                        		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              MaxDamageChunkSize;                               		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                MaxNumFacturedChunksToSpawnInAFrame;              		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              FractureExplosionVelScale;                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.WorldInfo.LightClamp
// 0x0008
struct FLightClamp
{
	float                                              Min;                                              		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Max;                                              		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.PhysXEmitterVerticalProperties
// 0x0018
struct FPhysXEmitterVerticalProperties
{
	unsigned long                                      bDisableLod : 1;                                  		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	int                                                ParticlesLodMin;                                  		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ParticlesLodMax;                                  		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                PacketsPerPhysXParticleSystemMax;                 		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bApplyCylindricalPacketCulling : 1;               		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              SpawnLodVsFifoBias;                               		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.PhysXVerticalProperties
// 0x0018
struct FPhysXVerticalProperties
{
	struct FPhysXEmitterVerticalProperties             Emitters;                                         		// 0x0000 (0x0018) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
};

// ScriptStruct Engine.WorldInfo.PhysXSimulationProperties
// 0x000C
struct FPhysXSimulationProperties
{
	unsigned long                                      bUseHardware : 1;                                 		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bFixedTimeStep : 1;                               		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	float                                              TimeStep;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                MaxSubSteps;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.WorldInfo.PhysXSceneProperties
// 0x003C
struct FPhysXSceneProperties
{
	struct FPhysXSimulationProperties                  PrimaryScene;                                     		// 0x0000 (0x000C) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	struct FPhysXSimulationProperties                  CompartmentRigidBody;                             		// 0x000C (0x000C) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	struct FPhysXSimulationProperties                  CompartmentFluid;                                 		// 0x0018 (0x000C) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	struct FPhysXSimulationProperties                  CompartmentCloth;                                 		// 0x0024 (0x000C) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	struct FPhysXSimulationProperties                  CompartmentSoftBody;                              		// 0x0030 (0x000C) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
};

// ScriptStruct Engine.Console.AutoCompleteNode
// 0x001C
struct FAutoCompleteNode
{
	int                                                IndexChar;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< int >                                      AutoCompleteListIndices;                          		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FPointer >                          ChildNodes;                                       		// 0x0010 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIAnimation.UIAnimationNotify
// 0x000C
struct FUIAnimationNotify
{
	unsigned char                                      NotifyType;                                       		// 0x0000 (0x0001) [0x0000000000000000]              
	struct FName                                       NotifyName;                                       		// 0x0004 (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.UIAnimation.UIAnimationRawData
// 0x0038
struct FUIAnimationRawData
{
	float                                              DestAsFloat;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FLinearColor                                DestAsColor;                                      		// 0x0004 (0x0010) [0x0000000000000000]              
	struct FRotator                                    DestAsRotator;                                    		// 0x0014 (0x000C) [0x0000000000000000]              
	struct FVector                                     DestAsVector;                                     		// 0x0020 (0x000C) [0x0000000000000000]              
	struct FUIAnimationNotify                          DestAsNotify;                                     		// 0x002C (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.UIAnimation.UIAnimationKeyFrame
// 0x0044
struct FUIAnimationKeyFrame
{
	float                                              RemainingTime;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned char                                      InterpMode;                                       		// 0x0004 (0x0001) [0x0000000000000000]              
	float                                              InterpExponent;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FUIAnimationRawData                         Data;                                             		// 0x000C (0x0038) [0x0000000000000000]              
};

// ScriptStruct Engine.UIAnimation.UIAnimTrack
// 0x001C
struct FUIAnimTrack
{
	unsigned char                                      TrackType;                                        		// 0x0000 (0x0001) [0x0000000000000000]              
	TArray< struct FUIAnimationKeyFrame >              KeyFrames;                                        		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FUIAnimationKeyFrame >              LoopFrames;                                       		// 0x0010 (0x000C) [0x0000000000402000]              ( CPF_Transient | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIAnimation.UIAnimSequence
// 0x0018
struct FUIAnimSequence
{
	class UUIAnimationSeq*                             SequenceRef;                                      		// 0x0000 (0x0004) [0x0000000000100000]              
	TArray< struct FUIAnimTrack >                      AnimationTracks;                                  		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned char                                      LoopMode;                                         		// 0x0010 (0x0001) [0x0000000000100000]              
	float                                              PlaybackRate;                                     		// 0x0014 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.SequenceEvent.QueuedActivationInfo
// 0x0018
struct FQueuedActivationInfo
{
	class AActor*                                      InOriginator;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class AActor*                                      InInstigator;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	TArray< int >                                      ActivateIndices;                                  		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bPushTop : 1;                                     		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.UIComp_DrawStringEditbox.UIStringSelectionRegion
// 0x0008
struct FUIStringSelectionRegion
{
	int                                                SelectionStartCharIndex;                          		// 0x0000 (0x0004) [0x0000000000100000]              
	int                                                SelectionEndCharIndex;                            		// 0x0004 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.CoverMeshComponent.CoverMeshes
// 0x002C
struct FCoverMeshes
{
	class UStaticMesh*                                 Base;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 LeanLeft;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 LeanRight;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 Climb;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 Mantle;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 SlipLeft;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 SlipRight;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 SwatLeft;                                         		// 0x001C (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 SwatRight;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 PopUp;                                            		// 0x0024 (0x0004) [0x0000000000000000]              
	class UStaticMesh*                                 PlayerOnly;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.CoverLink.FireLinkItem
// 0x0004
struct FFireLinkItem
{
	unsigned char                                      SrcType;                                          		// 0x0000 (0x0001) [0x0000000000000000]              
	unsigned char                                      SrcAction;                                        		// 0x0001 (0x0001) [0x0000000000000000]              
	unsigned char                                      DestType;                                         		// 0x0002 (0x0001) [0x0000000000000000]              
	unsigned char                                      DestAction;                                       		// 0x0003 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.CoverLink.FireLink
// 0x003C
struct FFireLink
{
	struct FActorReference                             TargetMarker;                                     		// 0x0000 (0x0014) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
	TArray< struct FFireLinkItem >                     Items;                                            		// 0x0014 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FVector                                     LastTargetMarkerLocation;                         		// 0x0020 (0x000C) [0x0000000000000000]              
	struct FVector                                     LastSrcMarkerLocation;                            		// 0x002C (0x000C) [0x0000000000000000]              
	unsigned long                                      bFallbackLink : 1;                                		// 0x0038 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.CoverLink.ExposedLink
// 0x0015
struct FExposedLink
{
	struct FActorReference                             TargetMarker;                                     		// 0x0000 (0x0014) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
	unsigned char                                      ExposedScale;                                     		// 0x0014 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.CoverLink.DangerLink
// 0x0018
struct FDangerLink
{
	struct FActorReference                             DangerNav;                                        		// 0x0000 (0x0014) [0x0000000000020003]              ( CPF_Edit | CPF_Const | CPF_EditConst )
	int                                                DangerCost;                                       		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.CoverLink.CoverReference
// 0x0008(0x001C - 0x0014)
struct FCoverReference : FActorReference
{
	int                                                SlotIdx;                                          		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                Direction;                                        		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.CoverLink.CoverSlot
// 0x00C4
struct FCoverSlot
{
	class APawn*                                       SlotOwner;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              SlotValidAfterTime;                               		// 0x0004 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	unsigned char                                      ForceCoverType;                                   		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      CoverType;                                        		// 0x0009 (0x0001) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	struct FVector                                     LocationOffset;                                   		// 0x000C (0x000C) [0x0000000000000000]              
	struct FRotator                                    RotationOffset;                                   		// 0x0018 (0x000C) [0x0000000000000000]              
	TArray< unsigned char >                            Actions;                                          		// 0x0024 (0x000C) [0x0000000000600000]              ( CPF_NeedCtorLink )
	TArray< struct FFireLink >                         FireLinks;                                        		// 0x0030 (0x000C) [0x0000000000600000]              ( CPF_NeedCtorLink )
	TArray< struct FFireLink >                         ForcedFireLinks;                                  		// 0x003C (0x000C) [0x0000000000600000]              ( CPF_NeedCtorLink )
	TArray< struct FFireLink >                         RejectedFireLinks;                                		// 0x0048 (0x000C) [0x0000000000402000]              ( CPF_Transient | CPF_NeedCtorLink )
	TArray< struct FExposedLink >                      ExposedFireLinks;                                 		// 0x0054 (0x000C) [0x0000000000600001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FDangerLink >                       DangerLinks;                                      		// 0x0060 (0x000C) [0x0000000000600001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FCoverReference                             MantleTarget;                                     		// 0x006C (0x001C) [0x0000000000200000]              
	TArray< struct FCoverReference >                   TurnTarget;                                       		// 0x0088 (0x000C) [0x0000000000600000]              ( CPF_NeedCtorLink )
	TArray< struct FCoverReference >                   SlipTarget;                                       		// 0x0094 (0x000C) [0x0000000000600000]              ( CPF_NeedCtorLink )
	TArray< struct FCoverReference >                   OverlapClaims;                                    		// 0x00A0 (0x000C) [0x0000000000620001]              ( CPF_Edit | CPF_EditConst | CPF_NeedCtorLink )
	unsigned long                                      bLeanLeft : 1;                                    		// 0x00AC (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bLeanRight : 1;                                   		// 0x00AC (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bForceCanPopUp : 1;                               		// 0x00AC (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bCanPopUp : 1;                                    		// 0x00AC (0x0004) [0x0000000000020001] [0x00000008] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bCanMantle : 1;                                   		// 0x00AC (0x0004) [0x0000000000020001] [0x00000010] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bCanClimbUp : 1;                                  		// 0x00AC (0x0004) [0x0000000000020001] [0x00000020] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bForceCanCoverSlip_Left : 1;                      		// 0x00AC (0x0004) [0x0000000000000001] [0x00000040] ( CPF_Edit )
	unsigned long                                      bForceCanCoverSlip_Right : 1;                     		// 0x00AC (0x0004) [0x0000000000000001] [0x00000080] ( CPF_Edit )
	unsigned long                                      bCanCoverSlip_Left : 1;                           		// 0x00AC (0x0004) [0x0000000000020001] [0x00000100] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bCanCoverSlip_Right : 1;                          		// 0x00AC (0x0004) [0x0000000000020001] [0x00000200] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bCanSwatTurn_Left : 1;                            		// 0x00AC (0x0004) [0x0000000000020001] [0x00000400] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bCanSwatTurn_Right : 1;                           		// 0x00AC (0x0004) [0x0000000000020001] [0x00000800] ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bEnabled : 1;                                     		// 0x00AC (0x0004) [0x0000000000000001] [0x00001000] ( CPF_Edit )
	unsigned long                                      bAllowPopup : 1;                                  		// 0x00AC (0x0004) [0x0000000000000001] [0x00002000] ( CPF_Edit )
	unsigned long                                      bAllowMantle : 1;                                 		// 0x00AC (0x0004) [0x0000000000000001] [0x00004000] ( CPF_Edit )
	unsigned long                                      bAllowCoverSlip : 1;                              		// 0x00AC (0x0004) [0x0000000000000001] [0x00008000] ( CPF_Edit )
	unsigned long                                      bAllowClimbUp : 1;                                		// 0x00AC (0x0004) [0x0000000000000001] [0x00010000] ( CPF_Edit )
	unsigned long                                      bAllowSwatTurn : 1;                               		// 0x00AC (0x0004) [0x0000000000000001] [0x00020000] ( CPF_Edit )
	unsigned long                                      bForceNoGroundAdjust : 1;                         		// 0x00AC (0x0004) [0x0000000000000001] [0x00040000] ( CPF_Edit )
	unsigned long                                      bPlayerOnly : 1;                                  		// 0x00AC (0x0004) [0x0000000000000001] [0x00080000] ( CPF_Edit )
	unsigned long                                      bSelected : 1;                                    		// 0x00AC (0x0004) [0x0000000000002000] [0x00100000] ( CPF_Transient )
	int                                                ExtraCost;                                        		// 0x00B0 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              LeanTraceDist;                                    		// 0x00B4 (0x0004) [0x0000000000000000]              
	class ACoverSlotMarker*                            SlotMarker;                                       		// 0x00B8 (0x0004) [0x0000000000220001]              ( CPF_Edit | CPF_EditConst )
	unsigned char                                      LocationDescription;                              		// 0x00BC (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bFailedToFindSurface : 1;                         		// 0x00C0 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.CoverLink.LinkSlotHelper
// 0x0010
struct FLinkSlotHelper
{
	class ACoverLink*                                  Link;                                             		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      Slots;                                            		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.CoverLink.CoverInfo
// 0x0008
struct FCoverInfo
{
	class ACoverLink*                                  Link;                                             		// 0x0000 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	int                                                SlotIdx;                                          		// 0x0004 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
};

// ScriptStruct Engine.CoverLink.TargetInfo
// 0x000C
struct FTargetInfo
{
	class AActor*                                      Target;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                SlotIdx;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                Direction;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.CoverLink.CovPosInfo
// 0x0034
struct FCovPosInfo
{
	class ACoverLink*                                  Link;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                LtSlotIdx;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                RtSlotIdx;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              LtToRtPct;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	struct FVector                                     Location;                                         		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FVector                                     Normal;                                           		// 0x001C (0x000C) [0x0000000000000000]              
	struct FVector                                     Tangent;                                          		// 0x0028 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.CoverReplicator.ManualCoverTypeInfo
// 0x0002
struct FManualCoverTypeInfo
{
	unsigned char                                      SlotIndex;                                        		// 0x0000 (0x0001) [0x0000000000000000]              
	unsigned char                                      ManualCoverType;                                  		// 0x0001 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.CoverReplicator.CoverReplicationInfo
// 0x0034
struct FCoverReplicationInfo
{
	class ACoverLink*                                  Link;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< unsigned char >                            SlotsEnabled;                                     		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            SlotsDisabled;                                    		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            SlotsAdjusted;                                    		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FManualCoverTypeInfo >              SlotsCoverTypeChanged;                            		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMeshComponent.ActiveMorph
// 0x000C
struct FActiveMorph
{
	class UMorphTarget*                                Target;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              Weight;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      bAbsolute : 1;                                    		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.SkeletalMeshComponent.Attachment
// 0x0030
struct FAttachment
{
	class UActorComponent*                             Component;                                        		// 0x0000 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	struct FName                                       BoneName;                                         		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RelativeLocation;                                 		// 0x000C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FRotator                                    RelativeRotation;                                 		// 0x0018 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RelativeScale;                                    		// 0x0024 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMeshComponent.TgMeshBoneTransformIndicies
// 0x000C
struct FTgMeshBoneTransformIndicies
{
	TArray< unsigned char >                            m_TgMeshBoneTransformIndex;                       		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMeshComponent.BonePair
// 0x0010
struct FBonePair
{
	struct FName                                       Bones[ 0x2 ];                                     		// 0x0000 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.SkeletalMeshComponent.MorphCorrection
// 0x0014
struct FMorphCorrection
{
	struct FName                                       CorrectionGroupName;                              		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     CorrectionAmount;                                 		// 0x0008 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.CullDistanceVolume.CullDistanceSizePair
// 0x0008
struct FCullDistanceSizePair
{
	float                                              Size;                                             		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              CullDistance;                                     		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIDataProvider.UIDataProviderField
// 0x0018
struct FUIDataProviderField
{
	struct FName                                       FieldTag;                                         		// 0x0000 (0x0008) [0x0000000000100000]              
	unsigned char                                      FieldType;                                        		// 0x0008 (0x0001) [0x0000000000100000]              
	TArray< class UUIDataProvider* >                   FieldProviders;                                   		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.CurrentGameDataStore.GameDataProviderTypes
// 0x000C
struct FGameDataProviderTypes
{
	class UClass*                                      GameDataProviderClass;                            		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UClass*                                      PlayerDataProviderClass;                          		// 0x0004 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UClass*                                      TeamDataProviderClass;                            		// 0x0008 (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.CurveEdPresetCurve.PresetGeneratedPoint
// 0x0015
struct FPresetGeneratedPoint
{
	float                                              KeyIn;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              KeyOut;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      TangentsValid : 1;                                		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              TangentIn;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              TangentOut;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned char                                      IntepMode;                                        		// 0x0014 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.DataStoreClient.PlayerDataStoreGroup
// 0x0010
struct FPlayerDataStoreGroup
{
	class ULocalPlayer*                                PlayerOwner;                                      		// 0x0000 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	TArray< class UUIDataStore* >                      DataStores;                                       		// 0x0004 (0x000C) [0x0000000000502002]              ( CPF_Const | CPF_Transient | CPF_NeedCtorLink )
};

// ScriptStruct Engine.HUD.ConsoleMessage
// 0x0018
struct FConsoleMessage
{
	struct FString                                     Text;                                             		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FColor                                      TextColor;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              MessageLife;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	class APlayerReplicationInfo*                      PRI;                                              		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.HUD.HudLocalizedMessage
// 0x0040
struct FHudLocalizedMessage
{
	class UClass*                                      Message;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     StringMessage;                                    		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                Switch;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
	float                                              EndOfLife;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              Lifetime;                                         		// 0x0018 (0x0004) [0x0000000000000000]              
	float                                              PosY;                                             		// 0x001C (0x0004) [0x0000000000000000]              
	struct FColor                                      DrawColor;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                FontSize;                                         		// 0x0024 (0x0004) [0x0000000000000000]              
	class UFont*                                       StringFont;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	float                                              DX;                                               		// 0x002C (0x0004) [0x0000000000000000]              
	float                                              DY;                                               		// 0x0030 (0x0004) [0x0000000000000000]              
	unsigned long                                      Drawn : 1;                                        		// 0x0034 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                Count;                                            		// 0x0038 (0x0004) [0x0000000000000000]              
	class UObject*                                     OptionalObject;                                   		// 0x003C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Input.KeyBind
// 0x0018
struct FKeyBind
{
	struct FName                                       Name;                                             		// 0x0000 (0x0008) [0x0000000000004000]              ( CPF_Config )
	struct FString                                     Command;                                          		// 0x0008 (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
	unsigned long                                      Control : 1;                                      		// 0x0014 (0x0004) [0x0000000000004000] [0x00000001] ( CPF_Config )
	unsigned long                                      Shift : 1;                                        		// 0x0014 (0x0004) [0x0000000000004000] [0x00000002] ( CPF_Config )
	unsigned long                                      Alt : 1;                                          		// 0x0014 (0x0004) [0x0000000000004000] [0x00000004] ( CPF_Config )
	unsigned long                                      bIgnoreCtrl : 1;                                  		// 0x0014 (0x0004) [0x0000000000004000] [0x00000008] ( CPF_Config )
	unsigned long                                      bIgnoreShift : 1;                                 		// 0x0014 (0x0004) [0x0000000000004000] [0x00000010] ( CPF_Config )
	unsigned long                                      bIgnoreAlt : 1;                                   		// 0x0014 (0x0004) [0x0000000000004000] [0x00000020] ( CPF_Config )
};

// ScriptStruct Engine.DecalComponent.DecalReceiver
// 0x0008
struct FDecalReceiver
{
	class UPrimitiveComponent*                         Component;                                        		// 0x0000 (0x0004) [0x000000000408000A]              ( CPF_Const | CPF_ExportObject | CPF_Component | CPF_EditInline )
	struct FPointer                                    RenderData;                                       		// 0x0004 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.DecalManager.ActiveDecalInfo
// 0x0008
struct FActiveDecalInfo
{
	class UDecalComponent*                             Decal;                                            		// 0x0000 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	float                                              LifetimeRemaining;                                		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Material.MaterialInput
// 0x001C
struct FMaterialInput
{
	class UMaterialExpression*                         Expression;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Mask;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                MaskR;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                MaskG;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                MaskB;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                MaskA;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                GCC64_Padding;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Material.ColorMaterialInput
// 0x0008(0x0024 - 0x001C)
struct FColorMaterialInput : FMaterialInput
{
	unsigned long                                      UseConstant : 1;                                  		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	struct FColor                                      Constant;                                         		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Material.ScalarMaterialInput
// 0x0008(0x0024 - 0x001C)
struct FScalarMaterialInput : FMaterialInput
{
	unsigned long                                      UseConstant : 1;                                  		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              Constant;                                         		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Material.VectorMaterialInput
// 0x0010(0x002C - 0x001C)
struct FVectorMaterialInput : FMaterialInput
{
	unsigned long                                      UseConstant : 1;                                  		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	struct FVector                                     Constant;                                         		// 0x0020 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.Material.Vector2MaterialInput
// 0x000C(0x0028 - 0x001C)
struct FVector2MaterialInput : FMaterialInput
{
	unsigned long                                      UseConstant : 1;                                  		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              ConstantX;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	float                                              ConstantY;                                        		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysicsVolume.CheckpointRecord
// 0x0004
struct APhysicsVolume_FCheckpointRecord
{
	unsigned long                                      bPainCausing : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.DynamicBlockingVolume.CheckpointRecord
// 0x001C
struct ADynamicBlockingVolume_FCheckpointRecord
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FRotator                                    Rotation;                                         		// 0x000C (0x000C) [0x0000000000000000]              
	unsigned long                                      bCollideActors : 1;                               		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bBlockActors : 1;                                 		// 0x0018 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bNeedsReplication : 1;                            		// 0x0018 (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleSysParam
// 0x0028
struct FParticleSysParam
{
	struct FName                                       Name;                                             		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ParamType;                                        		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              Scalar;                                           		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Vector;                                           		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FColor                                      Color;                                            		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AActor*                                      Actor;                                            		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UMaterialInterface*                          Material;                                         		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEventData
// 0x0034
struct FParticleEventData
{
	int                                                Type;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       EventName;                                        		// 0x0004 (0x0008) [0x0000000000000000]              
	float                                              EmitterTime;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	struct FVector                                     Location;                                         		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FVector                                     Direction;                                        		// 0x001C (0x000C) [0x0000000000000000]              
	struct FVector                                     Velocity;                                         		// 0x0028 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEventSpawnData
// 0x0000(0x0034 - 0x0034)
struct FParticleEventSpawnData : FParticleEventData
{
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEventDeathData
// 0x0004(0x0038 - 0x0034)
struct FParticleEventDeathData : FParticleEventData
{
	float                                              ParticleTime;                                     		// 0x0034 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEventCollideData
// 0x0020(0x0054 - 0x0034)
struct FParticleEventCollideData : FParticleEventData
{
	float                                              ParticleTime;                                     		// 0x0034 (0x0004) [0x0000000000000000]              
	struct FVector                                     Normal;                                           		// 0x0038 (0x000C) [0x0000000000000000]              
	float                                              Time;                                             		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                Item;                                             		// 0x0048 (0x0004) [0x0000000000000000]              
	struct FName                                       BoneName;                                         		// 0x004C (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEventKismetData
// 0x0010(0x0044 - 0x0034)
struct FParticleEventKismetData : FParticleEventData
{
	unsigned long                                      UsePSysCompLocation : 1;                          		// 0x0034 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FVector                                     Normal;                                           		// 0x0038 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.ParticleSystemComponent.ParticleEmitterInstance
// 0x0000
struct FParticleEmitterInstance
{
};

// ScriptStruct Engine.Emitter.CheckpointRecord
// 0x0004
struct AEmitter_FCheckpointRecord
{
	unsigned long                                      bIsActive : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.EmitterPool.EmitterBaseInfo
// 0x0020
struct FEmitterBaseInfo
{
	class UParticleSystemComponent*                    PSC;                                              		// 0x0000 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class AActor*                                      Base;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FVector                                     RelativeLocation;                                 		// 0x0008 (0x000C) [0x0000000000000000]              
	struct FRotator                                    RelativeRotation;                                 		// 0x0014 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.Engine.StatColorMapEntry
// 0x0008
struct FStatColorMapEntry
{
	float                                              In;                                               		// 0x0000 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	struct FColor                                      Out;                                              		// 0x0004 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
};

// ScriptStruct Engine.Engine.StatColorMapping
// 0x001C
struct FStatColorMapping
{
	struct FString                                     StatName;                                         		// 0x0000 (0x000C) [0x0000000000444000]              ( CPF_Config | CPF_GlobalConfig | CPF_NeedCtorLink )
	TArray< struct FStatColorMapEntry >                ColorMap;                                         		// 0x000C (0x000C) [0x0000000000444000]              ( CPF_Config | CPF_GlobalConfig | CPF_NeedCtorLink )
	unsigned long                                      DisableBlend : 1;                                 		// 0x0018 (0x0004) [0x0000000000044000] [0x00000001] ( CPF_Config | CPF_GlobalConfig )
};

// ScriptStruct Engine.Engine.DropNoteInfo
// 0x0024
struct FDropNoteInfo
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FRotator                                    Rotation;                                         		// 0x000C (0x000C) [0x0000000000000000]              
	struct FString                                     Comment;                                          		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.FluidSurfaceComponent.LightMapRef
// 0x0004
struct UFluidSurfaceComponent_FLightMapRef
{
	struct FPointer                                    Reference;                                        		// 0x0000 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.FogVolumeDensityInfo.CheckpointRecord
// 0x0004
struct AFogVolumeDensityInfo_FCheckpointRecord
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.FoliageComponent.FoliageInstanceBase
// 0x0034
struct FFoliageInstanceBase
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FVector                                     XAxis;                                            		// 0x000C (0x000C) [0x0000000000000000]              
	struct FVector                                     YAxis;                                            		// 0x0018 (0x000C) [0x0000000000000000]              
	struct FVector                                     ZAxis;                                            		// 0x0024 (0x000C) [0x0000000000000000]              
	float                                              DistanceFactorSquared;                            		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.FoliageComponent.GatheredFoliageInstance
// 0x0010(0x0044 - 0x0034)
struct FGatheredFoliageInstance : FFoliageInstanceBase
{
	struct FColor                                      StaticLighting[ 0x4 ];                            		// 0x0034 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.FoliageFactory.FoliageMesh
// 0x0048
struct FFoliageMesh
{
	class UStaticMesh*                                 InstanceStaticMesh;                               		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UMaterialInterface*                          Material;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxDrawRadius;                                    		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinTransitionRadius;                              		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinThinningRadius;                                		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     MinScale;                                         		// 0x0014 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     MaxScale;                                         		// 0x0020 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinUniformScale;                                  		// 0x002C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxUniformScale;                                  		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SwayScale;                                        		// 0x0034 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                Seed;                                             		// 0x0038 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SurfaceAreaPerInstance;                           		// 0x003C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bCreateInstancesOnBSP : 1;                        		// 0x0040 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bCreateInstancesOnStaticMeshes : 1;               		// 0x0040 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bCreateInstancesOnTerrain : 1;                    		// 0x0040 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	class UFoliageComponent*                           Component;                                        		// 0x0044 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
};

// ScriptStruct Engine.ForceFeedbackWaveform.WaveformSample
// 0x0008
struct FWaveformSample
{
	unsigned char                                      LeftAmplitude;                                    		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      RightAmplitude;                                   		// 0x0001 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      LeftFunction;                                     		// 0x0002 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      RightFunction;                                    		// 0x0003 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              Duration;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.FracturedStaticMeshComponent.FragmentGroup
// 0x0010
struct FFragmentGroup
{
	TArray< int >                                      FragmentIndices;                                  		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bGroupIsRooted : 1;                               		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.FracturedStaticMeshActor.DeferredPartToSpawn
// 0x0024
struct FDeferredPartToSpawn
{
	int                                                ChunkIndex;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FVector                                     InitialVel;                                       		// 0x0004 (0x000C) [0x0000000000000000]              
	struct FVector                                     InitialAngVel;                                    		// 0x0010 (0x000C) [0x0000000000000000]              
	float                                              RelativeScale;                                    		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      bExplosion : 1;                                   		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.GameEngine.LevelStreamingStatus
// 0x000C
struct FLevelStreamingStatus
{
	struct FName                                       PackageName;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned long                                      bShouldBeLoaded : 1;                              		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bShouldBeVisible : 1;                             		// 0x0008 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.GameEngine.FullyLoadedPackagesInfo
// 0x0028
struct FFullyLoadedPackagesInfo
{
	unsigned char                                      FullyLoadType;                                    		// 0x0000 (0x0001) [0x0000000000000000]              
	struct FString                                     Tag;                                              		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FName >                             PackagesToLoad;                                   		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< class UObject* >                           LoadedObjects;                                    		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.GameEngine.NamedNetDriver
// 0x000C
struct FNamedNetDriver
{
	struct FName                                       NetDriverName;                                    		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FPointer                                    NetDriver;                                        		// 0x0008 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.GameEngine.URL
// 0x0044
struct FURL
{
	struct FString                                     Protocol;                                         		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FString                                     Host;                                             		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                Port;                                             		// 0x0018 (0x0004) [0x0000000000100000]              
	struct FString                                     Map;                                              		// 0x001C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           Op;                                               		// 0x0028 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FString                                     Portal;                                           		// 0x0034 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                Valid;                                            		// 0x0040 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.GameViewportClient.PerPlayerSplitscreenData
// 0x0010
struct FPerPlayerSplitscreenData
{
	float                                              SizeX;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              SizeY;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              OriginX;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              OriginY;                                          		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.GameViewportClient.SplitscreenData
// 0x000C
struct FSplitscreenData
{
	TArray< struct FPerPlayerSplitscreenData >         PlayerData;                                       		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.GameViewportClient.DebugDisplayProperty
// 0x0010
struct FDebugDisplayProperty
{
	class UObject*                                     Obj;                                              		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       PropertyName;                                     		// 0x0004 (0x0008) [0x0000000000000000]              
	unsigned long                                      bSpecialProperty : 1;                             		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.GameViewportClient.TitleSafeZoneArea
// 0x0010
struct FTitleSafeZoneArea
{
	float                                              MaxPercentX;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              MaxPercentY;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              RecommendedPercentX;                              		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              RecommendedPercentY;                              		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.IniLocPatcher.IniLocFileEntry
// 0x000D
struct FIniLocFileEntry
{
	struct FString                                     Filename;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned char                                      ReadState;                                        		// 0x000C (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.InterpActor.CheckpointRecord
// 0x0020
struct AInterpActor_FCheckpointRecord
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FRotator                                    Rotation;                                         		// 0x000C (0x000C) [0x0000000000000000]              
	unsigned char                                      CollisionType;                                    		// 0x0018 (0x0001) [0x0000000000000000]              
	unsigned long                                      bHidden : 1;                                      		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bIsShutdown : 1;                                  		// 0x001C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bNeedsPositionReplication : 1;                    		// 0x001C (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct Engine.InterpCurveEdSetup.CurveEdEntry
// 0x002C
struct FCurveEdEntry
{
	class UObject*                                     CurveObject;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FColor                                      CurveColor;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FString                                     CurveName;                                        		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                bHideCurve;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                bColorCurve;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                bFloatingPointColorCurve;                         		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                bClamp;                                           		// 0x0020 (0x0004) [0x0000000000000000]              
	float                                              ClampLow;                                         		// 0x0024 (0x0004) [0x0000000000000000]              
	float                                              ClampHigh;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.InterpCurveEdSetup.CurveEdTab
// 0x0028
struct FCurveEdTab
{
	struct FString                                     TabName;                                          		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FCurveEdEntry >                     Curves;                                           		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              ViewStartInput;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
	float                                              ViewEndInput;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	float                                              ViewStartOutput;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	float                                              ViewEndOutput;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.InterpGroup.InterpEdSelKey
// 0x0010
struct FInterpEdSelKey
{
	class UInterpGroup*                                Group;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                TrackIndex;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                KeyIndex;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              UnsnappedPosition;                                		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.InterpTrackAnimControl.AnimControlTrackKey
// 0x001C
struct FAnimControlTrackKey
{
	float                                              StartTime;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       AnimSeqName;                                      		// 0x0004 (0x0008) [0x0000000000000000]              
	float                                              AnimStartOffset;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              AnimEndOffset;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	float                                              AnimPlayRate;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	unsigned long                                      bLooping : 1;                                     		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bReverse : 1;                                     		// 0x0018 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.InterpTrackDirector.DirectorTrackCut
// 0x0010
struct FDirectorTrackCut
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              TransitionTime;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FName                                       TargetCamGroup;                                   		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.InterpTrackEvent.EventTrackKey
// 0x000C
struct FEventTrackKey
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       EventName;                                        		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.InterpTrackFaceFX.FaceFXTrackKey
// 0x001C
struct FFaceFXTrackKey
{
	float                                              StartTime;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     FaceFXGroupName;                                  		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     FaceFXSeqName;                                    		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.InterpTrackFaceFX.FaceFXSoundCueKey
// 0x0004
struct FFaceFXSoundCueKey
{
	class USoundCue*                                   FaceFXSoundCue;                                   		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.InterpTrackMove.InterpLookupPoint
// 0x000C
struct FInterpLookupPoint
{
	struct FName                                       GroupName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	float                                              Time;                                             		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.InterpTrackMove.InterpLookupTrack
// 0x000C
struct FInterpLookupTrack
{
	TArray< struct FInterpLookupPoint >                Points;                                           		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.InterpTrackParticleReplay.ParticleReplayTrackKey
// 0x000C
struct FParticleReplayTrackKey
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              Duration;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ClipIDNumber;                                     		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.InterpTrackSound.SoundTrackKey
// 0x0010
struct FSoundTrackKey
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              Volume;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              Pitch;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   Sound;                                            		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.InterpTrackToggle.ToggleTrackKey
// 0x0005
struct FToggleTrackKey
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned char                                      ToggleAction;                                     		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.InterpTrackVisibility.VisibilityTrackKey
// 0x0006
struct FVisibilityTrackKey
{
	float                                              Time;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned char                                      Action;                                           		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      ActiveCondition;                                  		// 0x0005 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.LensFlare.LensFlareElement
// 0x0140
struct FLensFlareElement
{
	struct FName                                       ElementName;                                      		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              RayDistance;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bIsEnabled : 1;                                   		// 0x000C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bUseSourceDistance : 1;                           		// 0x000C (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bNormalizeRadialDistance : 1;                     		// 0x000C (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bModulateColorBySource : 1;                       		// 0x000C (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	struct FVector                                     Size;                                             		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	TArray< class UMaterialInterface* >                LFMaterials;                                      		// 0x001C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       LFMaterialIndex;                                  		// 0x0028 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       Scaling;                                          		// 0x0044 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionVector                      AxisScaling;                                      		// 0x0060 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       Rotation;                                         		// 0x007C (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionVector                      Color;                                            		// 0x0098 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       Alpha;                                            		// 0x00B4 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionVector                      Offset;                                           		// 0x00D0 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionVector                      DistMap_Scale;                                    		// 0x00EC (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionVector                      DistMap_Color;                                    		// 0x0108 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       DistMap_Alpha;                                    		// 0x0124 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
};

// ScriptStruct Engine.LensFlare.LensFlareElementCurvePair
// 0x0010
struct FLensFlareElementCurvePair
{
	struct FString                                     CurveName;                                        		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UObject*                                     CurveObject;                                      		// 0x000C (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.LensFlareComponent.LensFlareElementInstance
// 0x0000
struct FLensFlareElementInstance
{
};

// ScriptStruct Engine.LevelStreamingVolume.CheckpointRecord
// 0x0004
struct ALevelStreamingVolume_FCheckpointRecord
{
	unsigned long                                      bDisabled : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.LocalPlayer.CurrentPostProcessVolumeInfo
// 0x008C
struct FCurrentPostProcessVolumeInfo
{
	struct FPostProcessSettings                        LastSettings;                                     		// 0x0000 (0x0080) [0x0000000000000000]              
	class APostProcessVolume*                          LastVolumeUsed;                                   		// 0x0080 (0x0004) [0x0000000000000000]              
	float                                              BlendStartTime;                                   		// 0x0084 (0x0004) [0x0000000000000000]              
	float                                              LastBlendTime;                                    		// 0x0088 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.LocalPlayer.SynchronizedActorVisibilityHistory
// 0x0008
struct FSynchronizedActorVisibilityHistory
{
	struct FPointer                                    State;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FPointer                                    CriticalSection;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialExpression.ExpressionInput
// 0x001C
struct FExpressionInput
{
	class UMaterialExpression*                         Expression;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Mask;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                MaskR;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                MaskG;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                MaskB;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                MaskA;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                GCC64_Padding;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialInstanceConstant.FontParameterValue
// 0x0020
struct FFontParameterValue
{
	struct FName                                       ParameterName;                                    		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UFont*                                       FontValue;                                        		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                FontPage;                                         		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       ExpressionGUID;                                   		// 0x0010 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialInstanceConstant.ScalarParameterValue
// 0x001C
struct FScalarParameterValue
{
	struct FName                                       ParameterName;                                    		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              ParameterValue;                                   		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       ExpressionGUID;                                   		// 0x000C (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialInstanceConstant.TextureParameterValue
// 0x001C
struct FTextureParameterValue
{
	struct FName                                       ParameterName;                                    		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UTexture*                                    ParameterValue;                                   		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       ExpressionGUID;                                   		// 0x000C (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialInstanceConstant.VectorParameterValue
// 0x0028
struct FVectorParameterValue
{
	struct FName                                       ParameterName;                                    		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FLinearColor                                ParameterValue;                                   		// 0x0008 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FGuid                                       ExpressionGUID;                                   		// 0x0018 (0x0010) [0x0000000000000000]              
};

// ScriptStruct Engine.MaterialInstanceTimeVarying.ParameterValueOverTime
// 0x0030
struct FParameterValueOverTime
{
	struct FGuid                                       ExpressionGUID;                                   		// 0x0000 (0x0010) [0x0000000000000000]              
	float                                              StartTime;                                        		// 0x0010 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	struct FName                                       ParameterName;                                    		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bLoop : 1;                                        		// 0x001C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bAutoActivate : 1;                                		// 0x001C (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	float                                              CycleTime;                                        		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bNormalizeTime : 1;                               		// 0x0024 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              OffsetTime;                                       		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bOffsetFromEnd : 1;                               		// 0x002C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.MaterialInstanceTimeVarying.FontParameterValueOverTime
// 0x0008(0x0038 - 0x0030)
struct FFontParameterValueOverTime : FParameterValueOverTime
{
	class UFont*                                       FontValue;                                        		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                FontPage;                                         		// 0x0034 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.MaterialInstanceTimeVarying.ScalarParameterValueOverTime
// 0x0014(0x0044 - 0x0030)
struct FScalarParameterValueOverTime : FParameterValueOverTime
{
	float                                              ParameterValue;                                   		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FInterpCurveFloat                           ParameterValueCurve;                              		// 0x0034 (0x0010) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.MaterialInstanceTimeVarying.TextureParameterValueOverTime
// 0x0004(0x0034 - 0x0030)
struct FTextureParameterValueOverTime : FParameterValueOverTime
{
	class UTexture*                                    ParameterValue;                                   		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.MaterialInstanceTimeVarying.VectorParameterValueOverTime
// 0x0020(0x0050 - 0x0030)
struct FVectorParameterValueOverTime : FParameterValueOverTime
{
	struct FLinearColor                                ParameterValue;                                   		// 0x0030 (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FInterpCurveVector                          ParameterValueCurve;                              		// 0x0040 (0x0010) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.MorphNodeWeightBase.MorphNodeConn
// 0x0018
struct FMorphNodeConn
{
	TArray< class UMorphNodeBase* >                    ChildNodes;                                       		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FName                                       ConnName;                                         		// 0x000C (0x0008) [0x0000000000000000]              
	int                                                DrawY;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.MorphNodeWeightByBoneAngle.BoneAngleMorph
// 0x0008
struct FBoneAngleMorph
{
	float                                              Angle;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              TargetWeight;                                     		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.NxGenericForceFieldBrush.RBCollisionChannelContainer
// 0x0004
struct ANxGenericForceFieldBrush_FRBCollisionChannelContainer
{
	unsigned long                                      Default : 1;                                      		// 0x0000 (0x0004) [0x0000000000000003] [0x00000001] ( CPF_Edit | CPF_Const )
	unsigned long                                      Nothing : 1;                                      		// 0x0000 (0x0004) [0x0000000000000002] [0x00000002] ( CPF_Const )
	unsigned long                                      Pawn : 1;                                         		// 0x0000 (0x0004) [0x0000000000000003] [0x00000004] ( CPF_Edit | CPF_Const )
	unsigned long                                      Vehicle : 1;                                      		// 0x0000 (0x0004) [0x0000000000000003] [0x00000008] ( CPF_Edit | CPF_Const )
	unsigned long                                      Water : 1;                                        		// 0x0000 (0x0004) [0x0000000000000003] [0x00000010] ( CPF_Edit | CPF_Const )
	unsigned long                                      GameplayPhysics : 1;                              		// 0x0000 (0x0004) [0x0000000000000003] [0x00000020] ( CPF_Edit | CPF_Const )
	unsigned long                                      EffectPhysics : 1;                                		// 0x0000 (0x0004) [0x0000000000000003] [0x00000040] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled1 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000080] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled2 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000100] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled3 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000200] ( CPF_Edit | CPF_Const )
	unsigned long                                      Untitled4 : 1;                                    		// 0x0000 (0x0004) [0x0000000000000003] [0x00000400] ( CPF_Edit | CPF_Const )
	unsigned long                                      FluidDrain : 1;                                   		// 0x0000 (0x0004) [0x0000000000000003] [0x00000800] ( CPF_Edit | CPF_Const )
	unsigned long                                      Cloth : 1;                                        		// 0x0000 (0x0004) [0x0000000000000003] [0x00001000] ( CPF_Edit | CPF_Const )
	unsigned long                                      SoftBody : 1;                                     		// 0x0000 (0x0004) [0x0000000000000003] [0x00002000] ( CPF_Edit | CPF_Const )
};

// ScriptStruct Engine.OnlineGameplayEvents.PlayerInformation
// 0x0028
struct FPlayerInformation
{
	struct FString                                     ControllerName;                                   		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     PlayerName;                                       		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FUniqueNetId                                UniqueId;                                         		// 0x0018 (0x0008) [0x0000000000000000]              
	unsigned long                                      bIsBot : 1;                                       		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                LastPlayerEventIdx;                               		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineGameplayEvents.GameplayEvent
// 0x0008
struct FGameplayEvent
{
	int                                                PlayerEventAndTarget;                             		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                EventNameAndDesc;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineGameplayEvents.PlayerEvent
// 0x0018
struct FPlayerEvent
{
	float                                              EventTime;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FVector                                     EventLocation;                                    		// 0x0004 (0x000C) [0x0000000000000000]              
	int                                                PlayerIndexAndYaw;                                		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                PlayerPitchAndRoll;                               		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Settings.StringIdToStringMapping
// 0x0010
struct FStringIdToStringMapping
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       Name;                                             		// 0x0004 (0x0008) [0x0000000000008002]              ( CPF_Const | CPF_Localized )
	unsigned long                                      bIsWildcard : 1;                                  		// 0x000C (0x0004) [0x0000000000000002] [0x00000001] ( CPF_Const )
};

// ScriptStruct Engine.Settings.LocalizedStringSettingMetaData
// 0x0024
struct FLocalizedStringSettingMetaData
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       Name;                                             		// 0x0004 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     ColumnHeaderText;                                 		// 0x000C (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
	TArray< struct FStringIdToStringMapping >          ValueMappings;                                    		// 0x0018 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
};

// ScriptStruct Engine.Settings.IdToStringMapping
// 0x000C
struct FIdToStringMapping
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       Name;                                             		// 0x0004 (0x0008) [0x0000000000008002]              ( CPF_Const | CPF_Localized )
};

// ScriptStruct Engine.Settings.SettingsPropertyPropertyMetaData
// 0x0040
struct FSettingsPropertyPropertyMetaData
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       Name;                                             		// 0x0004 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     ColumnHeaderText;                                 		// 0x000C (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
	unsigned char                                      MappingType;                                      		// 0x0018 (0x0001) [0x0000000000000002]              ( CPF_Const )
	TArray< struct FIdToStringMapping >                ValueMappings;                                    		// 0x001C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	TArray< struct FSettingsData >                     PredefinedValues;                                 		// 0x0028 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	float                                              MinVal;                                           		// 0x0034 (0x0004) [0x0000000000000002]              ( CPF_Const )
	float                                              MaxVal;                                           		// 0x0038 (0x0004) [0x0000000000000002]              ( CPF_Const )
	float                                              RangeIncrement;                                   		// 0x003C (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.OnlineSubsystem.AchievementDetails
// 0x0034
struct FAchievementDetails
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     AchievementName;                                  		// 0x0004 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	struct FString                                     Description;                                      		// 0x0010 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	struct FString                                     HowTo;                                            		// 0x001C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	class USurface*                                    Image;                                            		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                GamerPoints;                                      		// 0x002C (0x0004) [0x0000000000000002]              ( CPF_Const )
	unsigned long                                      bIsSecret : 1;                                    		// 0x0030 (0x0004) [0x0000000000000002] [0x00000001] ( CPF_Const )
	unsigned long                                      bWasAchievedOnline : 1;                           		// 0x0030 (0x0004) [0x0000000000000002] [0x00000002] ( CPF_Const )
	unsigned long                                      bWasAchievedOffline : 1;                          		// 0x0030 (0x0004) [0x0000000000000002] [0x00000004] ( CPF_Const )
};

// ScriptStruct Engine.OnlinePlaylistManager.ConfiguredGameSetting
// 0x0020
struct FConfiguredGameSetting
{
	int                                                GameSettingId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     GameSettingsClassName;                            		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     URL;                                              		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UOnlineGameSettings*                         GameSettings;                                     		// 0x001C (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.OnlinePlaylistManager.Playlist
// 0x003C
struct FPlaylist
{
	TArray< struct FConfiguredGameSetting >            ConfiguredGames;                                  		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                PlaylistId;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	struct FString                                     LocalizationString;                               		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      ContentIds;                                       		// 0x001C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                TeamSize;                                         		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                TeamCount;                                        		// 0x002C (0x0004) [0x0000000000000000]              
	struct FString                                     Name;                                             		// 0x0030 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineProfileSettings.OnlineProfileSetting
// 0x0018
struct FOnlineProfileSetting
{
	unsigned char                                      Owner;                                            		// 0x0000 (0x0001) [0x0000000000000000]              
	struct FSettingsProperty                           ProfileSetting;                                   		// 0x0004 (0x0014) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineRecentPlayersList.RecentParty
// 0x0014
struct FRecentParty
{
	struct FUniqueNetId                                PartyLeader;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	TArray< struct FUniqueNetId >                      PartyMembers;                                     		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineRecentPlayersList.CurrentPlayerMet
// 0x0010
struct FCurrentPlayerMet
{
	int                                                TeamNum;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Skill;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FUniqueNetId                                NetId;                                            		// 0x0008 (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineStatsRead.OnlineStatsColumn
// 0x0010
struct FOnlineStatsColumn
{
	int                                                ColumnNo;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FSettingsData                               StatValue;                                        		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.OnlineStatsRead.OnlineStatsRow
// 0x002C
struct FOnlineStatsRow
{
	struct FUniqueNetId                                PlayerID;                                         		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FSettingsData                               Rank;                                             		// 0x0008 (0x000C) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     NickName;                                         		// 0x0014 (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
	TArray< struct FOnlineStatsColumn >                Columns;                                          		// 0x0020 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.OnlineStatsRead.ColumnMetaData
// 0x0018
struct FColumnMetaData
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       Name;                                             		// 0x0004 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     ColumnName;                                       		// 0x000C (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
};

// ScriptStruct Engine.ParticleEmitter.ParticleBurst
// 0x000C
struct FParticleBurst
{
	int                                                Count;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                CountLow;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Time;                                             		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.ParticleModule.ParticleCurvePair
// 0x0010
struct FParticleCurvePair
{
	struct FString                                     CurveName;                                        		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UObject*                                     CurveObject;                                      		// 0x000C (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.ParticleModuleBeamModifier.BeamModifierOptions
// 0x0004
struct FBeamModifierOptions
{
	unsigned long                                      bModify : 1;                                      		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bScale : 1;                                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bLock : 1;                                        		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
};

// ScriptStruct Engine.ParticleModuleEventGenerator.ParticleEvent_GenerateInfo
// 0x001C
struct FParticleEvent_GenerateInfo
{
	unsigned char                                      Type;                                             		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	int                                                Frequency;                                        		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                LowFreq;                                          		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                ParticleFrequency;                                		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      FirstTimeOnly : 1;                                		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      LastTimeOnly : 1;                                 		// 0x0010 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      UseReflectedImpactVector : 1;                     		// 0x0010 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	struct FName                                       CustomName;                                       		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.ParticleModuleOrbit.OrbitOptions
// 0x0004
struct FOrbitOptions
{
	unsigned long                                      bProcessDuringSpawn : 1;                          		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bProcessDuringUpdate : 1;                         		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bUseEmitterTime : 1;                              		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
};

// ScriptStruct Engine.ParticleModuleParameterDynamic.EmitterDynamicParameter
// 0x0028
struct FEmitterDynamicParameter
{
	struct FName                                       ParamName;                                        		// 0x0000 (0x0008) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	unsigned long                                      bUseEmitterTime : 1;                              		// 0x0008 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bSpawnTimeOnly : 1;                               		// 0x0008 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	struct FRawDistributionFloat                       ParamValue;                                       		// 0x000C (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
};

// ScriptStruct Engine.ParticleModuleTypeDataBeam2.BeamTargetData
// 0x000C
struct FBeamTargetData
{
	struct FName                                       TargetName;                                       		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              TargetPercentage;                                 		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.ParticleModuleTypeDataPhysX.PhysXEmitterVerticalLodProperties
// 0x0010
struct FPhysXEmitterVerticalLodProperties
{
	float                                              WeightForFifo;                                    		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              WeightForSpawnLod;                                		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SpawnLodRateVsLifeBias;                           		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              RelativeFadeoutTime;                              		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.ParticleSystemReplay.ParticleEmitterReplayFrame
// 0x000C
struct FParticleEmitterReplayFrame
{
	int                                                EmitterType;                                      		// 0x0000 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	int                                                OriginalEmitterIndex;                             		// 0x0004 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    FrameState;                                       		// 0x0008 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.ParticleSystemReplay.ParticleSystemReplayFrame
// 0x000C
struct FParticleSystemReplayFrame
{
	TArray< struct FParticleEmitterReplayFrame >       Emitters;                                         		// 0x0000 (0x000C) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.PhysXDestructible.PhysXDestructibleDepthParameters
// 0x0004
struct FPhysXDestructibleDepthParameters
{
	unsigned long                                      bTakeImpactDamage : 1;                            		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPlaySoundEffect : 1;                             		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bPlayParticleEffect : 1;                          		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bDoNotTimeOut : 1;                                		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      bNoKillDummy : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
};

// ScriptStruct Engine.PhysXDestructible.PhysXDestructibleParameters
// 0x0030
struct FPhysXDestructibleParameters
{
	float                                              DamageThreshold;                                  		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DamageToRadius;                                   		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DamageCap;                                        		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              ForceToDamage;                                    		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USoundCue*                                   FractureSound;                                    		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UParticleSystem*                             CrumbleParticleSystem;                            		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              CrumbleParticleSize;                              		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bAccumulateDamage : 1;                            		// 0x001C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              ScaledDamageToRadius;                             		// 0x0020 (0x0004) [0x0000000000000000]              
	TArray< struct FPhysXDestructibleDepthParameters > DepthParameters;                                  		// 0x0024 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
};

// ScriptStruct Engine.PhysXDestructibleActor.SpawnBasis
// 0x001C
struct FSpawnBasis
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FRotator                                    Rotation;                                         		// 0x000C (0x000C) [0x0000000000000000]              
	float                                              Scale;                                            		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysXDestructibleAsset.PhysXDestructibleAssetChunk
// 0x0034
struct FPhysXDestructibleAssetChunk
{
	int                                                Index;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                FragmentIndex;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              Volume;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              Size;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                Depth;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                ParentIndex;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                FirstChildIndex;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                NumChildren;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                MeshIndex;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                BoneIndex;                                        		// 0x0024 (0x0004) [0x0000000000000000]              
	struct FName                                       BoneName;                                         		// 0x0028 (0x0008) [0x0000000000000000]              
	int                                                BodyIndex;                                        		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysXDestructibleStructure.PhysXDestructibleChunk
// 0x0100
struct FPhysXDestructibleChunk
{
	unsigned long                                      WorldCentroidValid : 1;                           		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      WorldMatrixValid : 1;                             		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bCrumble : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      IsEnvironmentSupported : 1;                       		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      IsRouting : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      IsRouteValid : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      IsRouteBlocker : 1;                               		// 0x0000 (0x0004) [0x0000000000000000] [0x00000040] 
	int                                                ActorIndex;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                FragmentIndex;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                Index;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                MeshIndex;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                BoneIndex;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	struct FName                                       BoneName;                                         		// 0x0018 (0x0008) [0x0000000000000000]              
	int                                                BodyIndex;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	struct FVector                                     RelativeCentroid;                                 		// 0x0024 (0x000C) [0x0000000000000000]              
	struct FVector                                     WorldCentroid;                                    		// 0x0030 (0x000C) [0x0000000000000000]              
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x003C (0x0004) MISSED OFFSET
	struct FMatrix                                     RelativeMatrix;                                   		// 0x0040 (0x0040) [0x0000000000000000]              
	struct FMatrix                                     WorldMatrix;                                      		// 0x0080 (0x0040) [0x0000000000000000]              
	float                                              Radius;                                           		// 0x00C0 (0x0004) [0x0000000000000000]              
	int                                                ParentIndex;                                      		// 0x00C4 (0x0004) [0x0000000000000000]              
	int                                                FirstChildIndex;                                  		// 0x00C8 (0x0004) [0x0000000000000000]              
	int                                                NumChildren;                                      		// 0x00CC (0x0004) [0x0000000000000000]              
	int                                                Depth;                                            		// 0x00D0 (0x0004) [0x0000000000000000]              
	float                                              Age;                                              		// 0x00D4 (0x0004) [0x0000000000000000]              
	float                                              Damage;                                           		// 0x00D8 (0x0004) [0x0000000000000000]              
	float                                              Size;                                             		// 0x00DC (0x0004) [0x0000000000000000]              
	unsigned char                                      CurrentState;                                     		// 0x00E0 (0x0001) [0x0000000000000000]              
	struct FPointer                                    Structure;                                        		// 0x00E4 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                FIFOIndex;                                        		// 0x00E8 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                FirstOverlapIndex;                                		// 0x00EC (0x0004) [0x0000000000000000]              
	int                                                NumOverlaps;                                      		// 0x00F0 (0x0004) [0x0000000000000000]              
	int                                                ShortestRoute;                                    		// 0x00F4 (0x0004) [0x0000000000000000]              
	int                                                NumSupporters;                                    		// 0x00F8 (0x0004) [0x0000000000000000]              
	int                                                NumChildrenDup;                                   		// 0x00FC (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysXDestructibleStructure.PhysXDestructibleOverlap
// 0x000C
struct FPhysXDestructibleOverlap
{
	int                                                ChunkIndex0;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                ChunkIndex1;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                Adjacent;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysXEmitterSpawnable.IndexedRBState
// 0x0028
struct FIndexedRBState
{
	struct FVector                                     CenterOfMass;                                     		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FVector                                     LinearVelocity;                                   		// 0x000C (0x000C) [0x0000000000000000]              
	struct FVector                                     AngularVelocity;                                  		// 0x0018 (0x000C) [0x0000000000000000]              
	int                                                Index;                                            		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.PhysXEmitterSpawnable.RBVolumeFill
// 0x0018
struct FRBVolumeFill
{
	TArray< struct FIndexedRBState >                   RBStates;                                         		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FVector >                           Positions;                                        		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.PlayerOwnerDataStore.PlayerDataProviderTypes
// 0x0010
struct FPlayerDataProviderTypes
{
	class UClass*                                      PlayerOwnerDataProviderClass;                     		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UClass*                                      CurrentWeaponDataProviderClass;                   		// 0x0004 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UClass*                                      WeaponDataProviderClass;                          		// 0x0008 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UClass*                                      PowerupDataProviderClass;                         		// 0x000C (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.PointLightToggleable.CheckpointRecord
// 0x0004
struct APointLightToggleable_FCheckpointRecord
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.Sequence.ActivateOp
// 0x0010
struct FActivateOp
{
	class USequenceOp*                                 ActivatorOp;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class USequenceOp*                                 Op;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                InputIdx;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              RemainingDelay;                                   		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.RB_BodySetup.KCachedConvexDataElement
// 0x000C
struct FKCachedConvexDataElement
{
	TArray< unsigned char >                            ConvexElementData;                                		// 0x0000 (0x000C) [0x0000000000001000]              ( CPF_Native )
};

// ScriptStruct Engine.RB_BodySetup.KCachedConvexData
// 0x000C
struct FKCachedConvexData
{
	TArray< struct FKCachedConvexDataElement >         CachedConvexElements;                             		// 0x0000 (0x000C) [0x0000000000001000]              ( CPF_Native )
};

// ScriptStruct Engine.RB_ConstraintSetup.LinearDOFSetup
// 0x0008
struct FLinearDOFSetup
{
	unsigned char                                      bLimited;                                         		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              LimitSize;                                        		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.Scout.PathSizeInfo
// 0x0015
struct FPathSizeInfo
{
	struct FName                                       Desc;                                             		// 0x0000 (0x0008) [0x0000000000000000]              
	float                                              Radius;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              Height;                                           		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              CrouchHeight;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned char                                      PathColor;                                        		// 0x0014 (0x0001) [0x0000000000000000]              
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdAnimInfo
// 0x000C
struct FCrowdAnimInfo
{
	struct FName                                       m_nmAnimName;                                     		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      m_bInterruptable : 1;                             		// 0x0008 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdTargetActionInfo
// 0x000C
struct FCrowdTargetActionInfo
{
	struct FName                                       AnimName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bFireEffects : 1;                                 		// 0x0008 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdAttachmentInfo
// 0x0014
struct FCrowdAttachmentInfo
{
	class UStaticMesh*                                 StaticMesh;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Chance;                                           		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Scale3D;                                          		// 0x0008 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdAttachmentList
// 0x0014
struct FCrowdAttachmentList
{
	struct FName                                       SocketName;                                       		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FCrowdAttachmentInfo >              List;                                             		// 0x0008 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.ParticleEffect
// 0x000C
struct FParticleEffect
{
	struct FName                                       m_nmSocket;                                       		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UParticleSystem*                             m_ParticleSystem;                                 		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdSuit
// 0x0024
struct FCrowdSuit
{
	class USkeletalMesh*                               m_SuitMesh;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< class UMaterialInstanceConstant* >         m_SuitMaterials;                                  		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	unsigned long                                      m_bLoadSeperateHead : 1;                          		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      m_bLoadSeperateHair : 1;                          		// 0x0010 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      m_bAllowBald : 1;                                 		// 0x0010 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	TArray< struct FParticleEffect >                   m_ParticleEffects;                                		// 0x0014 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	class USoundCue*                                   m_SoundEffect;                                    		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SeqAct_CrowdSpawner.CrowdMeshParts
// 0x0028
struct FCrowdMeshParts
{
	TArray< struct FCrowdSuit >                        m_CrowdSuits;                                     		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	class USkeletalMesh*                               m_CrowdHead;                                      		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< class UStaticMesh* >                       m_CrowdHairs;                                     		// 0x0010 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FLinearColor >                      m_CrowdHairColors;                                		// 0x001C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SeqAct_Interp.CameraCutInfo
// 0x0010
struct FCameraCutInfo
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	float                                              TimeStamp;                                        		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.SeqAct_Interp.SavedTransform
// 0x0018
struct FSavedTransform
{
	struct FVector                                     Location;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FRotator                                    Rotation;                                         		// 0x000C (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.SeqAct_MultiLevelStreaming.LevelStreamingNameCombo
// 0x000C
struct FLevelStreamingNameCombo
{
	class ULevelStreaming*                             Level;                                            		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       LevelName;                                        		// 0x0004 (0x0008) [0x0000000000000003]              ( CPF_Edit | CPF_Const )
};

// ScriptStruct Engine.SeqAct_RangeSwitch.SwitchRange
// 0x0008
struct FSwitchRange
{
	int                                                Min;                                              		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                Max;                                              		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SeqCond_SwitchClass.SwitchClassInfo
// 0x0009
struct FSwitchClassInfo
{
	struct FName                                       ClassName;                                        		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      bFallThru;                                        		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SeqCond_SwitchName.SwitchNameCase
// 0x000C
struct FSwitchNameCase
{
	struct FName                                       NameValue;                                        		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bFallThru : 1;                                    		// 0x0008 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct Engine.SeqCond_SwitchObject.SwitchObjectCase
// 0x0008
struct FSwitchObjectCase
{
	class UObject*                                     ObjectValue;                                      		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bFallThru : 1;                                    		// 0x0004 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bDefaultValue : 1;                                		// 0x0004 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMesh.BoneMirrorInfo
// 0x0005
struct FBoneMirrorInfo
{
	int                                                SourceIndex;                                      		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      BoneFlipAxis;                                     		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMesh.SkeletalMeshLODInfo
// 0x0020
struct FSkeletalMeshLODInfo
{
	float                                              DisplayFactor;                                    		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              LODHysteresis;                                    		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      LODMaterialMap;                                   		// 0x0008 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
	TArray< unsigned long >                            bEnableShadowCasting;                             		// 0x0014 (0x000C) [0x0000000000400041]              ( CPF_Edit | CPF_EditConstArray | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMesh.AbsoluteMorphTargetInfo
// 0x000C
struct FAbsoluteMorphTargetInfo
{
	struct FName                                       TargetName;                                       		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              TargetWeight;                                     		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMesh.ClothSpecialBoneInfo
// 0x0018
struct FClothSpecialBoneInfo
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      BoneType;                                         		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      AttachedVertexIndices;                            		// 0x000C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMesh.SoftBodyTetraLink
// 0x0010
struct FSoftBodyTetraLink
{
	int                                                Index;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FVector                                     Bary;                                             		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.SkeletalMesh.SoftBodySpecialBoneInfo
// 0x0018
struct FSoftBodySpecialBoneInfo
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      BoneType;                                         		// 0x0008 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      AttachedVertexIndices;                            		// 0x000C (0x000C) [0x0000000000400002]              ( CPF_Const | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMesh.TgMeshBoneTransform
// 0x0024
struct FTgMeshBoneTransform
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bUseRotation : 1;                                 		// 0x0008 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bUseLocation : 1;                                 		// 0x0008 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	struct FRotator                                    RelativeTargetRotation;                           		// 0x000C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RelativeTargetLocation;                           		// 0x0018 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMesh.TgMeshBoneTransformStage
// 0x0018
struct FTgMeshBoneTransformStage
{
	TArray< struct FTgMeshBoneTransform >              TgMeshBoneTransforms;                             		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	float                                              InterpTime;                                       		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DelayTime;                                        		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Rate;                                             		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMesh.TgMeshBoneTransformSet
// 0x0014
struct FTgMeshBoneTransformSet
{
	struct FName                                       SetName;                                          		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FTgMeshBoneTransformStage >         TgMeshBoneTransformsStages;                       		// 0x0008 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SkeletalMesh.BoneMirrorExport
// 0x0011
struct FBoneMirrorExport
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       SourceBoneName;                                   		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      BoneFlipAxis;                                     		// 0x0010 (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMeshActor.SkelMeshActorControlTarget
// 0x000C
struct FSkelMeshActorControlTarget
{
	struct FName                                       ControlName;                                      		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class AActor*                                      TargetActor;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SkeletalMeshActor.CheckpointRecord
// 0x0010
struct ASkeletalMeshActor_FCheckpointRecord
{
	unsigned long                                      bHidden : 1;                                      		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FRotator                                    Rotation;                                         		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct Engine.SkeletalMeshActorBasedOnExtremeContent.SkelMaterialSetterDatum
// 0x0008
struct FSkelMaterialSetterDatum
{
	int                                                MaterialIndex;                                    		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UMaterialInterface*                          TheMaterial;                                      		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundMode.SoundClassAdjuster
// 0x0014
struct FSoundClassAdjuster
{
	unsigned char                                      SoundClassName;                                   		// 0x0000 (0x0001) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	struct FName                                       SoundClass;                                       		// 0x0004 (0x0008) [0x0000000000000000]              
	float                                              VolumeAdjuster;                                   		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              PitchAdjuster;                                    		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundMode.AudioEQEffect
// 0x0024
struct FAudioEQEffect
{
	struct FDouble                                     RootTime;                                         		// 0x0000 (0x0008) [0x0000000000003000]              ( CPF_Native | CPF_Transient )
	float                                              HFFrequency;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              HFGain;                                           		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MFCutoffFrequency;                                		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MFBandwidthFrequency;                             		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MFGain;                                           		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              LFFrequency;                                      		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              LFGain;                                           		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundNodeDistanceCrossFade.DistanceDatum
// 0x003C
struct FDistanceDatum
{
	struct FRawDistributionFloat                       FadeInDistance;                                   		// 0x0000 (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	struct FRawDistributionFloat                       FadeOutDistance;                                  		// 0x001C (0x001C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	float                                              Volume;                                           		// 0x0038 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundNodeWave.SubtitleCue
// 0x0014
struct FSubtitleCue
{
	struct FString                                     Text;                                             		// 0x0000 (0x000C) [0x0000000000408003]              ( CPF_Edit | CPF_Const | CPF_Localized | CPF_NeedCtorLink )
	float                                              Time;                                             		// 0x000C (0x0004) [0x0000000000008003]              ( CPF_Edit | CPF_Const | CPF_Localized )
	int                                                m_nMsgId;                                         		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SoundNodeWave.LocalizedSubtitle
// 0x0010
struct FLocalizedSubtitle
{
	TArray< struct FSubtitleCue >                      Subtitles;                                        		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bMature : 1;                                      		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bManualWordWrap : 1;                              		// 0x000C (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct Engine.SpeechRecognition.RecognisableWord
// 0x001C
struct FRecognisableWord
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FString                                     ReferenceWord;                                    		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     PhoneticWord;                                     		// 0x0010 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct Engine.SpeechRecognition.RecogVocabulary
// 0x0048
struct FRecogVocabulary
{
	TArray< struct FRecognisableWord >                 WhoDictionary;                                    		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FRecognisableWord >                 WhatDictionary;                                   		// 0x000C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FRecognisableWord >                 WhereDictionary;                                  		// 0x0018 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     VocabName;                                        		// 0x0024 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            VocabData;                                        		// 0x0030 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< unsigned char >                            WorkingVocabData;                                 		// 0x003C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.SpeechRecognition.RecogUserData
// 0x0010
struct FRecogUserData
{
	int                                                ActiveVocabularies;                               		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< unsigned char >                            UserData;                                         		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.SpeedTreeComponent.SpeedTreeStaticLight
// 0x0020
struct FSpeedTreeStaticLight
{
	struct FGuid                                       Guid;                                             		// 0x0000 (0x0010) [0x0000000000000002]              ( CPF_Const )
	class UShadowMap1D*                                BranchAndFrondShadowMap;                          		// 0x0010 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UShadowMap1D*                                LeafMeshShadowMap;                                		// 0x0014 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UShadowMap1D*                                LeafCardShadowMap;                                		// 0x0018 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UShadowMap1D*                                BillboardShadowMap;                               		// 0x001C (0x0004) [0x0000000000000002]              ( CPF_Const )
};

// ScriptStruct Engine.SpeedTreeComponent.LightMapRef
// 0x0004
struct USpeedTreeComponent_FLightMapRef
{
	struct FPointer                                    Reference;                                        		// 0x0000 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.SpotLightToggleable.CheckpointRecord
// 0x0004
struct ASpotLightToggleable_FCheckpointRecord
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.StaticMeshActorBasedOnExtremeContent.SMMaterialSetterDatum
// 0x0008
struct FSMMaterialSetterDatum
{
	int                                                MaterialIndex;                                    		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UMaterialInterface*                          TheMaterial;                                      		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.SVehicle.VehicleState
// 0x0044
struct FVehicleState
{
	struct FRigidBodyState                             RBState;                                          		// 0x0000 (0x0038) [0x0000000000000000]              
	unsigned char                                      ServerBrake;                                      		// 0x0038 (0x0001) [0x0000000000000000]              
	unsigned char                                      ServerGas;                                        		// 0x0039 (0x0001) [0x0000000000000000]              
	unsigned char                                      ServerSteering;                                   		// 0x003A (0x0001) [0x0000000000000000]              
	unsigned char                                      ServerRise;                                       		// 0x003B (0x0001) [0x0000000000000000]              
	unsigned long                                      bServerHandbrake : 1;                             		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                ServerView;                                       		// 0x0040 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Terrain.TerrainHeight
// 0x0000
struct FTerrainHeight
{
};

// ScriptStruct Engine.Terrain.TerrainInfoData
// 0x0000
struct FTerrainInfoData
{
};

// ScriptStruct Engine.Terrain.TerrainLayer
// 0x0030
struct FTerrainLayer
{
	struct FString                                     Name;                                             		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	class UTerrainLayerSetup*                          Setup;                                            		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                AlphaMapIndex;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      Highlighted : 1;                                  		// 0x0014 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      WireframeHighlighted : 1;                         		// 0x0014 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      Hidden : 1;                                       		// 0x0014 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	struct FColor                                      HighlightColor;                                   		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FColor                                      WireframeColor;                                   		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                MinX;                                             		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                MinY;                                             		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                MaxX;                                             		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                MaxY;                                             		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Terrain.TerrainDecorationInstance
// 0x0014
struct FTerrainDecorationInstance
{
	class UPrimitiveComponent*                         Component;                                        		// 0x0000 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	float                                              X;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              Y;                                                		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              Scale;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                Yaw;                                              		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Terrain.TerrainDecoration
// 0x0024
struct FTerrainDecoration
{
	class UPrimitiveComponentFactory*                  Factory;                                          		// 0x0000 (0x0004) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	float                                              MinScale;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxScale;                                         		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Density;                                          		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SlopeRotationBlend;                               		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                RandSeed;                                         		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FTerrainDecorationInstance >        Instances;                                        		// 0x0018 (0x000C) [0x0000000000480000]              ( CPF_Component | CPF_NeedCtorLink )
};

// ScriptStruct Engine.Terrain.TerrainDecoLayer
// 0x001C
struct FTerrainDecoLayer
{
	struct FString                                     Name;                                             		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FTerrainDecoration >                Decorations;                                      		// 0x000C (0x000C) [0x0000000000480001]              ( CPF_Edit | CPF_Component | CPF_NeedCtorLink )
	int                                                AlphaMapIndex;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Terrain.AlphaMap
// 0x0000
struct FAlphaMap
{
};

// ScriptStruct Engine.Terrain.TerrainWeightedMaterial
// 0x0000
struct ATerrain_FTerrainWeightedMaterial
{
};

// ScriptStruct Engine.Terrain.SelectedTerrainVertex
// 0x000C
struct FSelectedTerrainVertex
{
	int                                                X;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Y;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                Weight;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Terrain.TerrainMaterialResource
// 0x0000
struct FTerrainMaterialResource
{
};

// ScriptStruct Engine.Terrain.CachedTerrainMaterialArray
// 0x000C
struct FCachedTerrainMaterialArray
{
	TArray< struct FPointer >                          CachedMaterials;                                  		// 0x0000 (0x000C) [0x0000000000201002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.TerrainComponent.TerrainBVTree
// 0x000C
struct FTerrainBVTree
{
	TArray< int >                                      Nodes;                                            		// 0x0000 (0x000C) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.TerrainComponent.TerrainkDOPTree
// 0x0018
struct FTerrainkDOPTree
{
	TArray< int >                                      Nodes;                                            		// 0x0000 (0x000C) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	TArray< int >                                      Triangles;                                        		// 0x000C (0x000C) [0x0000000000001002]              ( CPF_Const | CPF_Native )
};

// ScriptStruct Engine.TerrainLayerSetup.FilterLimit
// 0x0010
struct FFilterLimit
{
	unsigned long                                      Enabled : 1;                                      		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              Base;                                             		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              NoiseScale;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              NoiseAmount;                                      		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.TerrainLayerSetup.TerrainFilteredMaterial
// 0x0054
struct FTerrainFilteredMaterial
{
	unsigned long                                      UseNoise : 1;                                     		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	float                                              NoiseScale;                                       		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              NoisePercent;                                     		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FFilterLimit                                MinHeight;                                        		// 0x000C (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FFilterLimit                                MaxHeight;                                        		// 0x001C (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FFilterLimit                                MinSlope;                                         		// 0x002C (0x0010) [0x0000000000000001]              ( CPF_Edit )
	struct FFilterLimit                                MaxSlope;                                         		// 0x003C (0x0010) [0x0000000000000001]              ( CPF_Edit )
	float                                              Alpha;                                            		// 0x004C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UTerrainMaterial*                            Material;                                         		// 0x0050 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.TerrainMaterial.TerrainFoliageMesh
// 0x0038
struct FTerrainFoliageMesh
{
	class UStaticMesh*                                 StaticMesh;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UMaterialInterface*                          Material;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                Density;                                          		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxDrawRadius;                                    		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinTransitionRadius;                              		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinScale;                                         		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxScale;                                         		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinUniformScale;                                  		// 0x001C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxUniformScale;                                  		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MinThinningRadius;                                		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                Seed;                                             		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SwayScale;                                        		// 0x002C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              AlphaMapThreshold;                                		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              SlopeRotationBlend;                               		// 0x0034 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.TerrainWeightMapTexture.TerrainWeightedMaterial
// 0x0000
struct UTerrainWeightMapTexture_FTerrainWeightedMaterial
{
};

// ScriptStruct Engine.Texture2DComposite.SourceTexture2DRegion
// 0x0014
struct FSourceTexture2DRegion
{
	int                                                OffsetX;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                OffsetY;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                SizeX;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                SizeY;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  Texture2D;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.Trigger.CheckpointRecord
// 0x0004
struct ATrigger_FCheckpointRecord
{
	unsigned long                                      bCollideActors : 1;                               		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct Engine.TriggerStreamingLevel.LevelStreamingData
// 0x0008
struct FLevelStreamingData
{
	unsigned long                                      bShouldBeLoaded : 1;                              		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bShouldBeVisible : 1;                             		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bShouldBlockOnLoad : 1;                           		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	class ULevelStreaming*                             Level;                                            		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIComp_ListPresenterBase.UIListItemDataBinding
// 0x0014
struct FUIListItemDataBinding
{
	class UUIListElementCellProvider*                  DataSourceProvider;                               		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x0004 (0x0004) FIX WRONG TYPE SIZE OF PREVIUS PROPERTY
	struct FName                                       DataSourceTag;                                    		// 0x0008 (0x0008) [0x0000000000000000]              
	int                                                DataSourceIndex;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIComp_ListPresenter.UIListElementCell
// 0x008C
struct FUIListElementCell
{
	int                                                ContainerElementIndex;                            		// 0x0000 (0x0004) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
	class UUIList*                                     OwnerList;                                        		// 0x0004 (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )
	struct FUIStyleReference                           CellStyle[ 0x4 ];                                 		// 0x0008 (0x0080) [0x0000000000000000]              
	class UObject*                                     ValueObject;                                      		// 0x0088 (0x0004) [0x0000000000802000]              ( CPF_Transient | CPF_NoExport )
};

// ScriptStruct Engine.UIComp_ListPresenter.UIListItem
// 0x0028
struct FUIListItem
{
	struct FUIListItemDataBinding                      DataSource;                                       		// 0x0000 (0x0014) [0x0000000000000002]              ( CPF_Const )
	TArray< struct FUIListElementCell >                Cells;                                            		// 0x0014 (0x000C) [0x0000000004420041]              ( CPF_Edit | CPF_EditConstArray | CPF_EditConst | CPF_NeedCtorLink | CPF_EditInline )
	unsigned char                                      ElementState;                                     		// 0x0020 (0x0001) [0x0000000001022001]              ( CPF_Edit | CPF_Transient | CPF_EditConst )
	class UUIObject*                                   ElementWidget;                                    		// 0x0024 (0x0004) [0x0000000004020001]              ( CPF_Edit | CPF_EditConst | CPF_EditInline )
};

// ScriptStruct Engine.UIComp_ListPresenter.UIListElementCellTemplate
// 0x0020(0x00AC - 0x008C)
struct FUIListElementCellTemplate : FUIListElementCell
{
	struct FName                                       CellDataField;                                    		// 0x008C (0x0008) [0x0000000004000001]              ( CPF_Edit | CPF_EditInline )
	struct FString                                     ColumnHeaderText;                                 		// 0x0094 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FUIScreenValue_Extent                       CellSize;                                         		// 0x00A0 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              CellPosition;                                     		// 0x00A8 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIComp_ListPresenter.UIElementCellSchema
// 0x000C
struct FUIElementCellSchema
{
	TArray< struct FUIListElementCellTemplate >        Cells;                                            		// 0x0000 (0x000C) [0x0000000004400001]              ( CPF_Edit | CPF_NeedCtorLink | CPF_EditInline )
};

// ScriptStruct Engine.UIComp_ListElementSorter.UIListSortingParameters
// 0x000C
struct FUIListSortingParameters
{
	int                                                PrimaryIndex;                                     		// 0x0000 (0x0004) [0x0000000000100000]              
	int                                                SecondaryIndex;                                   		// 0x0004 (0x0004) [0x0000000000100000]              
	unsigned long                                      bReversePrimarySorting : 1;                       		// 0x0008 (0x0004) [0x0000000000100000] [0x00000001] 
	unsigned long                                      bReverseSecondarySorting : 1;                     		// 0x0008 (0x0004) [0x0000000000100000] [0x00000002] 
	unsigned long                                      bCaseSensitive : 1;                               		// 0x0008 (0x0004) [0x0000000000100000] [0x00000004] 
	unsigned long                                      bIntSortPrimary : 1;                              		// 0x0008 (0x0004) [0x0000000000100000] [0x00000008] 
	unsigned long                                      bIntSortSecondary : 1;                            		// 0x0008 (0x0004) [0x0000000000100000] [0x00000010] 
	unsigned long                                      bFloatSortPrimary : 1;                            		// 0x0008 (0x0004) [0x0000000000100000] [0x00000020] 
	unsigned long                                      bFloatSortSecondary : 1;                          		// 0x0008 (0x0004) [0x0000000000100000] [0x00000040] 
};

// ScriptStruct Engine.UIList.CellHitDetectionInfo
// 0x0010
struct FCellHitDetectionInfo
{
	int                                                HitColumn;                                        		// 0x0000 (0x0004) [0x0000000000100000]              
	int                                                HitRow;                                           		// 0x0004 (0x0004) [0x0000000000100000]              
	int                                                ResizeColumn;                                     		// 0x0008 (0x0004) [0x0000000000100000]              
	int                                                ResizeRow;                                        		// 0x000C (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.UIContextMenu.ContextMenuItem
// 0x001C
struct FContextMenuItem
{
	class UUIContextMenu*                              OwnerMenu;                                        		// 0x0000 (0x0004) [0x0000000000102002]              ( CPF_Const | CPF_Transient )
	struct FPointer                                    ParentItem;                                       		// 0x0004 (0x0004) [0x0000000000103002]              ( CPF_Const | CPF_Native | CPF_Transient )
	unsigned char                                      ItemType;                                         		// 0x0008 (0x0001) [0x0000000000100000]              
	struct FString                                     ItemText;                                         		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                ItemId;                                           		// 0x0018 (0x0004) [0x0000000000100000]              
};

// ScriptStruct Engine.UISkin.UISoundCue
// 0x000C
struct FUISoundCue
{
	struct FName                                       SoundName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	class USoundCue*                                   SoundToPlay;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIDataProvider_OnlineProfileSettings.ProfileSettingsArrayProvider
// 0x0010
struct FProfileSettingsArrayProvider
{
	int                                                ProfileSettingsId;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       ProfileSettingsName;                              		// 0x0004 (0x0008) [0x0000000000000000]              
	class UUIDataProvider_OnlineProfileSettingsArray*  Provider;                                         		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIDataProvider_Settings.SettingsArrayProvider
// 0x0010
struct FSettingsArrayProvider
{
	int                                                SettingsId;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       SettingsName;                                     		// 0x0004 (0x0008) [0x0000000000000000]              
	class UUIDataProvider_SettingsArray*               Provider;                                         		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.UIDataStore_DynamicResource.DynamicResourceProviderDefinition
// 0x0018
struct FDynamicResourceProviderDefinition
{
	struct FName                                       ProviderTag;                                      		// 0x0000 (0x0008) [0x0000000000004000]              ( CPF_Config )
	struct FString                                     ProviderClassName;                                		// 0x0008 (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
	class UClass*                                      ProviderClass;                                    		// 0x0014 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIDataStore_GameResource.GameResourceDataProvider
// 0x001C
struct FGameResourceDataProvider
{
	struct FName                                       ProviderTag;                                      		// 0x0000 (0x0008) [0x0000000000004000]              ( CPF_Config )
	struct FString                                     ProviderClassName;                                		// 0x0008 (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
	unsigned long                                      bExpandProviders : 1;                             		// 0x0014 (0x0004) [0x0000000000004000] [0x00000001] ( CPF_Config )
	class UClass*                                      ProviderClass;                                    		// 0x0018 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIDataStore_InputAlias.UIInputKeyData
// 0x0018
struct FUIInputKeyData
{
	struct FRawInputKeyEventData                       InputKeyData;                                     		// 0x0000 (0x000C) [0x0000000000004000]              ( CPF_Config )
	struct FString                                     ButtonFontMarkupString;                           		// 0x000C (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIDataStore_InputAlias.UIDataStoreInputAlias
// 0x0050
struct FUIDataStoreInputAlias
{
	struct FName                                       AliasName;                                        		// 0x0000 (0x0008) [0x0000000000004000]              ( CPF_Config )
	struct FUIInputKeyData                             PlatformInputKeys[ 0x3 ];                         		// 0x0008 (0x0048) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIDataStore_OnlineGameSearch.GameSearchCfg
// 0x0028
struct FGameSearchCfg
{
	class UClass*                                      GameSearchClass;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	class UClass*                                      DefaultGameSettingsClass;                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UClass*                                      SearchResultsProviderClass;                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIDataProvider_Settings*                    DesiredSettingsProvider;                          		// 0x000C (0x0004) [0x0000000000000000]              
	TArray< class UUIDataProvider_Settings* >          SearchResults;                                    		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UOnlineGameSearch*                           Search;                                           		// 0x001C (0x0004) [0x0000000000000000]              
	struct FName                                       SearchName;                                       		// 0x0020 (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.UIDataStore_OnlineGameSettings.GameSettingsCfg
// 0x0014
struct FGameSettingsCfg
{
	class UClass*                                      GameSettingsClass;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIDataProvider_Settings*                    Provider;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UOnlineGameSettings*                         GameSettings;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FName                                       SettingsName;                                     		// 0x000C (0x0008) [0x0000000000000000]              
};

// ScriptStruct Engine.UIDataStore_OnlineStats.RankMetaData
// 0x0014
struct FRankMetaData
{
	struct FName                                       RankName;                                         		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     RankColumnName;                                   		// 0x0008 (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIDataStore_OnlineStats.PlayerNickMetaData
// 0x0014
struct FPlayerNickMetaData
{
	struct FName                                       PlayerNickName;                                   		// 0x0000 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FString                                     PlayerNickColumnName;                             		// 0x0008 (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIDataStore_StringAliasMap.UIMenuInputMap
// 0x001C
struct FUIMenuInputMap
{
	struct FName                                       FieldName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FName                                       Set;                                              		// 0x0008 (0x0008) [0x0000000000000000]              
	struct FString                                     MappedText;                                       		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct Engine.UIFrameBox.CornerSizes
// 0x0030
struct FCornerSizes
{
	float                                              TopLeft[ 0x2 ];                                   		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              TopRight[ 0x2 ];                                  		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              BottomLeft[ 0x2 ];                                		// 0x0010 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              BottomRight[ 0x2 ];                               		// 0x0018 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              TopHeight;                                        		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              BottomHeight;                                     		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              CenterLeftWidth;                                  		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              CenterRightWidth;                                 		// 0x002C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct Engine.UIInteraction.UIKeyRepeatData
// 0x0010
struct FUIKeyRepeatData
{
	struct FName                                       CurrentRepeatKey;                                 		// 0x0000 (0x0008) [0x0000000000100000]              
	struct FDouble                                     NextRepeatTime;                                   		// 0x0008 (0x0008) [0x0000000000100000]              
};

// ScriptStruct Engine.UIInteraction.UIAxisEmulationData
// 0x0004(0x0014 - 0x0010)
struct FUIAxisEmulationData : FUIKeyRepeatData
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0010 (0x0004) [0x0000000000100000] [0x00000001] 
};

// ScriptStruct Engine.UIPrefab.ArchetypeInstancePair
// 0x0028
struct FArchetypeInstancePair
{
	class UUIObject*                                   WidgetArchetype;                                  		// 0x0000 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	class UUIObject*                                   WidgetInstance;                                   		// 0x0004 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	float                                              ArchetypeBounds[ 0x4 ];                           		// 0x0008 (0x0010) [0x0000000000102000]              ( CPF_Transient )
	float                                              InstanceBounds[ 0x4 ];                            		// 0x0018 (0x0010) [0x0000000000102000]              ( CPF_Transient )
};

// ScriptStruct Engine.UIStyle_Combo.StyleDataReference
// 0x0020
struct FStyleDataReference
{
	class UUIStyle*                                    OwnerStyle;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FSTYLE_ID                                   SourceStyleID;                                    		// 0x0004 (0x0010) [0x0000000000000000]              
	class UUIStyle*                                    SourceStyle;                                      		// 0x0014 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UUIState*                                    SourceState;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIStyle_Data*                               CustomStyleData;                                  		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct Engine.StaticMesh.StaticMeshLODElement
// 0x000C
struct FStaticMeshLODElement
{
	class UMaterialInterface*                          Material;                                         		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bEnableShadowCasting : 1;                         		// 0x0004 (0x0004) [0x0000000000001001] [0x00000001] ( CPF_Edit | CPF_Native )
	unsigned long                                      bEnableCollision : 1;                             		// 0x0008 (0x0004) [0x0000000000001001] [0x00000001] ( CPF_Edit | CPF_Native )
};

// ScriptStruct Engine.StaticMesh.StaticMeshLODInfo
// 0x000C
struct FStaticMeshLODInfo
{
	TArray< struct FStaticMeshLODElement >             Elements;                                         		// 0x0000 (0x000C) [0x0000000000001041]              ( CPF_Edit | CPF_EditConstArray | CPF_Native )
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif