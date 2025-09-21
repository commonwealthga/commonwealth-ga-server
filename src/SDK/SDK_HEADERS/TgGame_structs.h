/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: TgGame_structs.h
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

// ScriptStruct TgGame.TgAIController.ThreatStruct
// 0x0008
struct FThreatStruct
{
	class ATgPawn*                                     Pawn;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              fThreat;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.TgPlayAnimation
// 0x000C
struct FTgPlayAnimation
{
	int                                                nAnimationId;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned long                                      bFullBody : 1;                                    		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              fSeconds;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.m_PostureAIPauseTime
// 0x0008
struct Fm_PostureAIPauseTime
{
	unsigned char                                      TransFromPosture;                                 		// 0x0000 (0x0001) [0x0000000000000000]              
	unsigned char                                      TransToPosture;                                   		// 0x0001 (0x0001) [0x0000000000000000]              
	float                                              PauseTime;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgObject.CombatMessageInfo
// 0x0018
struct FCombatMessageInfo
{
	int                                                nMsgId;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nSourceId;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nSourceOwnerId;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nTargetId;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nAmount1;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nAmount2;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgObject.HairColorParams
// 0x00F8
struct FHairColorParams
{
	struct FLinearColor                                PrimaryColor;                                     		// 0x0000 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                SpecColor;                                        		// 0x0010 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                InnerColor;                                       		// 0x0020 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                LocalHairColor;                                   		// 0x0030 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                OuterColor;                                       		// 0x0040 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                TColor;                                           		// 0x0050 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                SecondaryColorNonHair;                            		// 0x0060 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                FacialHairColor;                                  		// 0x0070 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                ScalpColor;                                       		// 0x0080 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                SecondaryColor;                                   		// 0x0090 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                SpecColor_Secondary;                              		// 0x00A0 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                InnerColor_Secondary;                             		// 0x00B0 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                LocalHairColor_Secondary;                         		// 0x00C0 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                OuterColor_Secondary;                             		// 0x00D0 (0x0010) [0x0000000000000000]              
	float                                              TMask;                                            		// 0x00E0 (0x0004) [0x0000000000000000]              
	float                                              SpecularPower;                                    		// 0x00E4 (0x0004) [0x0000000000000000]              
	float                                              SpecContrast;                                     		// 0x00E8 (0x0004) [0x0000000000000000]              
	float                                              SpecSubtract;                                     		// 0x00EC (0x0004) [0x0000000000000000]              
	float                                              FacialHairSpec;                                   		// 0x00F0 (0x0004) [0x0000000000000000]              
	float                                              SecondaryHairColorToggle;                         		// 0x00F4 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgObject.MorphInfo
// 0x000D
struct FMorphInfo
{
	struct FName                                       NodeName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              NodeWeight;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      NodeHeadMorphPoseID;                              		// 0x000C (0x0001) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgObject.CustomCharacterAssembly
// 0x0044
struct FCustomCharacterAssembly
{
	int                                                SuitMeshId;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                HeadMeshId;                                       		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                HairMeshId;                                       		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                HelmetMeshId;                                     		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                SkinToneParameterId;                              		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                SkinRaceParameterId;                              		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                EyeColorParameterId;                              		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bBald : 1;                                        		// 0x001C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bHideHelmet : 1;                                  		// 0x001C (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bValidCustomAssembly : 1;                         		// 0x001C (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bHalfHelmet : 1;                                  		// 0x001C (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	int                                                nGenderTypeId;                                    		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                HeadFlairId;                                      		// 0x0024 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                SuitFlairId;                                      		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                JetpackTrailId;                                   		// 0x002C (0x0004) [0x0000000000000000]              
	int                                                DyeList[ 0x5 ];                                   		// 0x0030 (0x0014) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgObject.DeviceAttachmentAssembly
// 0x001C
struct FDeviceAttachmentAssembly
{
	int                                                BackpackMeshId;                                   		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                InHandDeviceMeshId;                               		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                InHandAnimSetDeviceId;                            		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                InHandLeftHandDeviceMeshId;                       		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                InHandLeftHandDeviceAnimSetId;                    		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       LeftHandDeviceSocketName;                         		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgObject.EquipDeviceInfo
// 0x000C
struct FEquipDeviceInfo
{
	int                                                nDeviceId;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nDeviceInstanceId;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nQualityValueId;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.TgOffhandAnimationOffset
// 0x0014
struct FTgOffhandAnimationOffset
{
	struct FName                                       AnimationType;                                    		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FVector                                     Offset;                                           		// 0x0008 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.AppliedMICFx
// 0x000C
struct FAppliedMICFx
{
	class UTgSpecialFx*                                MICFx;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nPriority;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      bActive : 1;                                      		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgPawn.DamageDisplay
// 0x001C
struct FDamageDisplay
{
	float                                              InitTime;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              ServerInitTime;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              Lifetime;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FColor                                      DisplayColor;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	struct FString                                     DamageString;                                     		// 0x0010 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgPawn.BuffHeader
// 0x0010
struct FBuffHeader
{
	int                                                nPropId;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nReqCategoryCode;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nReqSkillId;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nReqDeviceInstId;                                 		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.BuffInfo
// 0x0030
struct FBuffInfo
{
	struct FBuffHeader                                 BuffHeader;                                       		// 0x0000 (0x0010) [0x0000000000000000]              
	float                                              fSkillPercentModifier;                            		// 0x0010 (0x0004) [0x0000000000000000]              
	float                                              fSkillModifier;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              fItemPercentModifier;                             		// 0x0018 (0x0004) [0x0000000000000000]              
	float                                              fItemModifier;                                    		// 0x001C (0x0004) [0x0000000000000000]              
	float                                              fSelfPercentModifier;                             		// 0x0020 (0x0004) [0x0000000000000000]              
	float                                              fSelfModifier;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
	float                                              fPercentModifier;                                 		// 0x0028 (0x0004) [0x0000000000000000]              
	float                                              fModifier;                                        		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.TGEQUIP_SLOTS_STRUCT
// 0x00C8
struct FTGEQUIP_SLOTS_STRUCT
{
	int                                                SlotIndices[ 0x19 ];                              		// 0x0000 (0x0064) [0x0000000000000000]              
	int                                                MiscItems[ 0x19 ];                                		// 0x0064 (0x0064) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.BuffRequest
// 0x0018
struct FBuffRequest
{
	unsigned char                                      eRequestContext;                                  		// 0x0000 (0x0001) [0x0000000000000000]              
	int                                                nPropId;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nReqCategoryCode;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nReqSkillId;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nReqDeviceInstId;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      bUsePotencyModifier : 1;                          		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgPawn.SpawnedBotInfo
// 0x000C
struct FSpawnedBotInfo
{
	class ATgRepInfo_Player*                           pBotPRI;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nBotFireMode;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nResendInfo;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.Sensor_See_Actor
// 0x0008
struct FSensor_See_Actor
{
	class AActor*                                      aActor;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              fLastSeeTime;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.Sensor_See_Actor_List
// 0x000C
struct FSensor_See_Actor_List
{
	TArray< struct FSensor_See_Actor >                 ActorList;                                        		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgPawn.Scanner_Settings
// 0x004C
struct FScanner_Settings
{
	int                                                EffectGroupId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                Scanner_Range;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                Scanner_FOV;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned long                                      Scanner_RequiresLOS : 1;                          		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                Scanner_SeeFlag[ 0x8 ];                           		// 0x0010 (0x0020) [0x0000000000000000]              
	int                                                Scanner_DisplayFlag[ 0x7 ];                       		// 0x0030 (0x001C) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.Icon_Info
// 0x0010
struct FIcon_Info
{
	struct FString                                     sIconTexture;                                     		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nIconIndex;                                       		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.RappelInfo
// 0x0014
struct FRappelInfo
{
	struct FVector                                     vRappelFromLocation;                              		// 0x0000 (0x000C) [0x0000000000000000]              
	class UTgSpecialFx*                                RopeFX;                                           		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgAnimNodeBlendByRappelling*                RappelBlendList;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.DeathZoomInfo
// 0x001C
struct FDeathZoomInfo
{
	class ATgPawn*                                     Victim;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class ATgRepInfo_Player*                           VictimPRI;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class ATgPawn*                                     Killer;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	class ATgRepInfo_Player*                           KillerPRI;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class ATgRepInfo_Player*                           OwnerPRI;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nKillerHealthPct;                                 		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                nDeviceId;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPawn.TgCameraValues
// 0x005C
struct FTgCameraValues
{
	unsigned long                                      c_bUsesShootingReticle : 1;                       		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      c_bCameraAlignOnFire : 1;                         		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      c_bTargetAlignOnFire : 1;                         		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      c_bFirstPerson : 1;                               		// 0x0000 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	struct FVector                                     c_fInterpSpeed;                                   		// 0x0004 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vPitchRange;                                    		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vPivot;                                         		// 0x001C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vOffsetUp;                                      		// 0x0028 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vOffsetFlat;                                    		// 0x0034 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vOffsetDown;                                    		// 0x0040 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     c_vCrouchPivot;                                   		// 0x004C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              c_fPelvisDampenAmount;                            		// 0x0058 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimBlendByMelee.AnimationGroup
// 0x0028
struct FAnimationGroup
{
	struct FName                                       m_nAnimName;                                      		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_nDisplayGroupName;                              		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_RightHandWeaponAnimName;                        		// 0x0010 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      m_bPlayRightHandWeaponAnims : 1;                  		// 0x0018 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	struct FName                                       m_LeftHandWeaponAnimName;                         		// 0x001C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      m_bPlayLeftHandWeaponAnims : 1;                   		// 0x0024 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimBlendByMelee.TgMeleeOverrideInfo
// 0x0010
struct FTgMeleeOverrideInfo
{
	int                                                nDeviceId;                                        		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FAnimationGroup >                   OverrideAnims;                                    		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimBlendByObjectiveStatus.m_StatusTransitionInfo
// 0x000C
struct Fm_StatusTransitionInfo
{
	unsigned char                                      m_TransitionToStatus;                             		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_TransitionAnimName;                             		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimBlendByObjectiveStatus.m_StatusProfile
// 0x0010
struct Fm_StatusProfile
{
	unsigned char                                      m_Status;                                         		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct Fm_StatusTransitionInfo >           m_TransitionAnims;                                		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimBlendByPhase.m_PhaseTransitionAnimSet
// 0x000C
struct Fm_PhaseTransitionAnimSet
{
	int                                                m_TransitionToPhase;                              		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_TransitionAnimName;                             		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimBlendByPhase.m_PhaseProfile
// 0x0010
struct Fm_PhaseProfile
{
	int                                                m_Phase;                                          		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct Fm_PhaseTransitionAnimSet >         m_TransitionAnimSets;                             		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimBlendByPosture.m_TransitionAnimSet
// 0x000C
struct Fm_TransitionAnimSet
{
	unsigned char                                      m_TransitionToPosture;                            		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_TransitionAnimName;                             		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimBlendByPosture.m_PostureProfile
// 0x0010
struct Fm_PostureProfile
{
	unsigned char                                      m_Posture;                                        		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct Fm_TransitionAnimSet >              m_TransitionAnimSets;                             		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimBlendBySpeed.ResetSynchGroup
// 0x0020
struct FResetSynchGroup
{
	struct FName                                       AnimNodeSynchName;                                		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UAnimNodeSynch*                              SynchNode;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FName                                       GroupName;                                        		// 0x000C (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      ResetChannels;                                    		// 0x0014 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimNodeBlendByDeviceMode.TransitionAnimInfo
// 0x0010
struct FTransitionAnimInfo
{
	struct FName                                       DeployAnimName;                                   		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       CollapseAnimName;                                 		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNodeBlendByLandingRun.ImpactTimeScale
// 0x0010
struct FImpactTimeScale
{
	float                                              FullyBlendedTime;                                 		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              BlendInTime;                                      		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              BlendOutTime;                                     		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_fImpactVelocity;                                		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNodeBlendByOffhand.OffhandAnimation
// 0x0014
struct FOffhandAnimation
{
	struct FName                                       m_OffhandUseType;                                 		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FName                                       m_AnimName;                                       		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      m_bFullBody : 1;                                  		// 0x0010 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNodeBlendByPhysics.ChildBlendInfo
// 0x0008
struct FChildBlendInfo
{
	unsigned char                                      m_PhysicsState;                                   		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_fBlendTime;                                     		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNodeBlendByPhysics.ChildrenBlendInfo
// 0x0010
struct FChildrenBlendInfo
{
	unsigned char                                      m_PhysicsState;                                   		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FChildBlendInfo >                   m_ChildBlendInfo;                                 		// 0x0004 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgAnimNodeRandom.TgRandomAnimInfo
// 0x0020
struct FTgRandomAnimInfo
{
	float                                              Chance;                                           		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      LoopCountMin;                                     		// 0x0004 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      LoopCountMax;                                     		// 0x0005 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	float                                              BlendInTime;                                      		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      bCanRepeat : 1;                                   		// 0x000C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned char                                      LoopCount;                                        		// 0x0010 (0x0001) [0x0000000000000000]              
	unsigned long                                      bCallOnAnimEndOnParent : 1;                       		// 0x0014 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      m_bChooseNewNodeOnAnimEnd : 1;                    		// 0x0014 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	struct FVector2D                                   m_ChildActiveTime;                                		// 0x0018 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNodeSequenceByBoneRotation.AnimByRotation
// 0x0014
struct FAnimByRotation
{
	struct FRotator                                    DesiredRotation;                                  		// 0x0000 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AnimName;                                         		// 0x000C (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimNotify_NPCVoice.VoiceSoundCuePairing
// 0x0008
struct FVoiceSoundCuePairing
{
	unsigned char                                      Voice;                                            		// 0x0000 (0x0001) [0x0000000000000001]              ( CPF_Edit )
	class USoundCue*                                   SoundCue;                                         		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgObject.HeadMorphData
// 0x0520
struct FHeadMorphData
{
	struct FMorphInfo                                  HeadMorphInfo[ 0x52 ];                            		// 0x0000 (0x0520) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgObject.ObjectiveMICInfo
// 0x0014
struct FObjectiveMICInfo
{
	int                                                MyTeam;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              AmountFilled;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                ObjStatus;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                IsContested;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                Flip;                                             		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgObject.MeshAssets
// 0x0030
struct FMeshAssets
{
	struct FString                                     m_sSkeletalMesh;                                  		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     m_sPhysicsAsset;                                  		// 0x000C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     m_sAnimTreeTemplate;                              		// 0x0018 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FString                                     m_sAnimSets;                                      		// 0x0024 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgObject.RewardInfo
// 0x0028
struct FRewardInfo
{
	int                                                RequestedReward;                                  		// 0x0000 (0x0004) [0x0000000000000002]              ( CPF_Const )
	int                                                ActualReward;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                ActualRewardAccum;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                F2PReward;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                F2PRewardAccum;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                EliteAgentReward;                                 		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                EliteAgentRewardAccum;                            		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                BoostReward;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                BoostRewardAccum;                                 		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                ErrorMessage;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgAnimTurnInPlace.RotTransitionInfo
// 0x000C
struct FRotTransitionInfo
{
	float                                              RotationOffset;                                   		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       TransName;                                        		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnimTurnInPlace_Player.TIP_Transition
// 0x0010
struct FTIP_Transition
{
	struct FName                                       TransName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FName                                       AnimName;                                         		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgAnnouncer.CachedSound
// 0x000C
struct FCachedSound
{
	struct FName                                       CacheName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	class USoundCue*                                   CacheSound;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgAnnouncer.AnnouncerPackageInfo
// 0x0010
struct FAnnouncerPackageInfo
{
	struct FName                                       PackageName;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FName                                       CuePrefix;                                        		// 0x0008 (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgBotFactory.SpawnQueueEntry
// 0x0010
struct FSpawnQueueEntry
{
	int                                                nGroupNumber;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              fSpawnTime;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      bRespawn : 1;                                     		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nSpawnTableId;                                    		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgBotFactory.SpawnGroupDetail
// 0x0010
struct FSpawnGroupDetail
{
	int                                                nMinCount;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nMaxCount;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nCurrentCount;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nRespawnSeconds;                                  		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgDamageType.DamageTypeAnimInfo
// 0x000C
struct FDamageTypeAnimInfo
{
	struct FName                                       m_nmAnimName;                                     		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned long                                      m_bLoopAnim : 1;                                  		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRigidBodyAnim : 1;                             		// 0x0008 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgGame.TgImpactFx.IMPACT_FX
// 0x0024
struct FIMPACT_FX
{
	struct FName                                       nmMaterialType;                                   		// 0x0000 (0x0008) [0x0000000000000000]              
	class USoundCue*                                   Sound;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	class UMaterialInterface*                          DecalMaterial;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              fDecalWidth;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	float                                              fDecalHeight;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	unsigned long                                      bRandomRotation : 1;                              		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	class UParticleSystem*                             Particles;                                        		// 0x001C (0x0004) [0x0000000000000000]              
	class UTgSpecialFx*                                SpecialFx;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgDeploy_Sensor.DeploySensorConfig
// 0x0018
struct FDeploySensorConfig
{
	float                                              fProximityDistance;                               		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nTriggerEventFlag;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nActionFlag;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	TArray< class ATgPawn* >                           TouchedPlayers;                                   		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgDeploy_SweepSensor.TouchedPlayer
// 0x0008
struct FTouchedPlayer
{
	class ATgPawn*                                     TouchedPlayer;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              LastTouchedTime;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgSpecialFx.Fx_PSC
// 0x0028
struct FFx_PSC
{
	int                                                c_nId;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UParticleSystemComponent*                    c_PSC;                                            		// 0x0004 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UParticleSystem*                             c_PSCTemplate;                                    		// 0x0008 (0x0004) [0x0000000000000002]              ( CPF_Const )
	float                                              c_fScale;                                         		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                c_nScaleType;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      c_bRunning : 1;                                   		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FName                                       c_nmSocketDisplayGrp;                             		// 0x0018 (0x0008) [0x0000000000000000]              
	struct FName                                       c_nmSocket;                                       		// 0x0020 (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgDeviceForm.ModeData
// 0x0030
struct FModeData
{
	int                                                c_nFireModeIcon;                                  		// 0x0000 (0x0004) [0x0000000000100000]              
	float                                              c_fEquipModeTime;                                 		// 0x0004 (0x0004) [0x0000000000100000]              
	float                                              c_fBuildupTime;                                   		// 0x0008 (0x0004) [0x0000000000100000]              
	unsigned long                                      c_bContinuousFire : 1;                            		// 0x000C (0x0004) [0x0000000000100000] [0x00000001] 
	class UTgImpactFx*                                 c_FxImpact;                                       		// 0x0010 (0x0004) [0x0000000000100000]              
	unsigned long                                      c_FxInstantFire : 1;                              		// 0x0014 (0x0004) [0x0000000000100000] [0x00000001] 
	float                                              c_fReturnToIdleDelay;                             		// 0x0018 (0x0004) [0x0000000000100000]              
	struct FName                                       c_nmOffhandAnimationType;                         		// 0x001C (0x0008) [0x0000000000100000]              
	unsigned long                                      c_bScaleFireAnimsWithRefire : 1;                  		// 0x0024 (0x0004) [0x0000000000100000] [0x00000001] 
	unsigned long                                      c_bInterruptAnimOnRefire : 1;                     		// 0x0024 (0x0004) [0x0000000000100000] [0x00000002] 
	float                                              c_fEffectRadius;                                  		// 0x0028 (0x0004) [0x0000000000100000]              
	float                                              c_fFullBodyAnimTime;                              		// 0x002C (0x0004) [0x0000000000100000]              
};

// ScriptStruct TgGame.TgTimerManager.TGT_REG_LINK
// 0x000C
struct FTGT_REG_LINK
{
	struct FPointer                                    NextLink;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UObject*                                     RegObject;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UFunction*                                   RegFunction;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgDeviceFire.TGDFM_ANIM_SEQUENCE
// 0x0010
struct FTGDFM_ANIM_SEQUENCE
{
	int                                                nId;                                              		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FName                                       nmSeqName;                                        		// 0x0004 (0x0008) [0x0000000000000000]              
	unsigned long                                      bLeftHand : 1;                                    		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgDeviceForm.DeviceAnimInfo
// 0x0004
struct FDeviceAnimInfo
{
	unsigned long                                      bReady : 1;                                       		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bFire : 1;                                        		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bRetrieve : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bPutAway : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
};

// ScriptStruct TgGame.TgSpecialFx.Fx_Sound
// 0x000C
struct FFx_Sound
{
	int                                                c_nId;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UAudioComponent*                             c_acSound;                                        		// 0x0004 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	unsigned long                                      c_bAllowSoundToFinishFlag : 1;                    		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgDistributionFloatSoundAttenuation.AttenuationGroup
// 0x000C
struct FAttenuationGroup
{
	struct FName                                       GroupName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	float                                              AttenuationDistance;                              		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgDoor.DoorPart
// 0x0030
struct FDoorPart
{
	unsigned long                                      Enabled : 1;                                      		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	struct FRotator                                    Rotation;                                         		// 0x0004 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Start;                                            		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     End;                                              		// 0x001C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	unsigned long                                      Initialized : 1;                                  		// 0x0028 (0x0004) [0x0000000000000000] [0x00000001] 
	class UStaticMeshComponent*                        Mesh;                                             		// 0x002C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
};

// ScriptStruct TgGame.TgEffectManager.EffectQueueEntry
// 0x0008
struct FEffectQueueEntry
{
	int                                                nEffectGroupID;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nExtraInfo;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgEffectManager.EffectListEntry
// 0x0010
struct FEffectListEntry
{
	int                                                nEffectGroupID;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned char                                      byNumStacks;                                      		// 0x0004 (0x0001) [0x0000000000000000]              
	float                                              fInitTimeRemaining;                               		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nExtraInfo;                                       		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgGame.PlayerReward
// 0x0010
struct FPlayerReward
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nToken;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nXp;                                              		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nCurrency;                                        		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPlayerController.TG_TRAINING_END_INFO
// 0x0014
struct FTG_TRAINING_END_INFO
{
	int                                                m_nAwardedMedal;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              m_fCompletionTime;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_nSkillId;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                m_nSkillRank;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bAlreadyHaveSkill : 1;                          		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bCannotActuallyGetSkill : 1;                    		// 0x0010 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgGame.TgPlayerController.TgPostProcessTransitionInfo
// 0x000C
struct FTgPostProcessTransitionInfo
{
	class ATgPostProcessVolume*                        TgPostProcessVolume;                              		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              fRemainingTime;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              bFadeIn;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPlayerController.ViewShakeInfo
// 0x0038
struct FViewShakeInfo
{
	struct FVector                                     OffsetMag;                                        		// 0x0000 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     OffsetRate;                                       		// 0x000C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              OffsetTime;                                       		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RotMag;                                           		// 0x001C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RotRate;                                          		// 0x0028 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              RotTime;                                          		// 0x0034 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgPlayerController.UIDeviceModeStruct
// 0x0010
struct FUIDeviceModeStruct
{
	unsigned long                                      bActive : 1;                                      		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nDeviceId;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned char                                      eEqPoint;                                         		// 0x0008 (0x0001) [0x0000000000000000]              
	int                                                nModeNbr;                                         		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgInterpolatingCameraActor.InterpolatingCameraInfo
// 0x0040
struct FInterpolatingCameraInfo
{
	unsigned long                                      m_bInterpLocation : 1;                            		// 0x0000 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      m_bInterpRotation : 1;                            		// 0x0000 (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      m_bInterpFOVAngle : 1;                            		// 0x0000 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	struct FVector                                     m_BaseLocation;                                   		// 0x0004 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     m_TargetLocation;                                 		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FRotator                                    m_BaseRotation;                                   		// 0x001C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FRotator                                    m_TargetRotation;                                 		// 0x0028 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_fBaseFOVAngle;                                  		// 0x0034 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_fTargetFOVAngle;                                		// 0x0038 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_fInterpSpeed;                                   		// 0x003C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgInventoryManager.InventoryData
// 0x0040
struct FInventoryData
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000100000]              
	int                                                nItemId;                                          		// 0x0004 (0x0004) [0x0000000000100000]              
	int                                                nLocationCode;                                    		// 0x0008 (0x0004) [0x0000000000100000]              
	int                                                nInstanceCount;                                   		// 0x000C (0x0004) [0x0000000000100000]              
	int                                                nBlueprintId;                                     		// 0x0010 (0x0004) [0x0000000000100000]              
	int                                                nCreatedByCharacterId;                            		// 0x0014 (0x0004) [0x0000000000100000]              
	int                                                nCraftedQualityValueId;                           		// 0x0018 (0x0004) [0x0000000000100000]              
	unsigned long                                      bBoundFlag : 1;                                   		// 0x001C (0x0004) [0x0000000000100000] [0x00000001] 
	int                                                nDurability;                                      		// 0x0020 (0x0004) [0x0000000000100000]              
	float                                              fAcquiredDatetime;                                		// 0x0024 (0x0004) [0x0000000000100000]              
	unsigned long                                      bEquippedOnOtherChar : 1;                         		// 0x0028 (0x0004) [0x0000000000100000] [0x00000001] 
	int                                                m_nEquipSlotValueIdArray[ 0x5 ];                  		// 0x002C (0x0014) [0x0000000000100000]              
};

// ScriptStruct TgGame.TgInventoryObject.sInventoryReq
// 0x0010
struct FsInventoryReq
{
	unsigned long                                      m_bLevelFail : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bSkillFail : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nSkillLevelReq;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_nSkillIdReq;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bFlairFail : 1;                                 		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bLocationFail : 1;                              		// 0x000C (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgGame.TgMorphNodePose.MaxMorphWeightInfo
// 0x000C
struct FMaxMorphWeightInfo
{
	struct FName                                       MaxMorphWeightGroupName;                          		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              MaxMorphWeight;                                   		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeBlendList.TgTextureParameterInfo
// 0x0010
struct FTgTextureParameterInfo
{
	int                                                MaterialId;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       ParameterName;                                    		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	class UTexture*                                    TextureParameterValue;                            		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeBlendList.TgScalarParameterInfo
// 0x0010
struct FTgScalarParameterInfo
{
	int                                                MaterialId;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       ParameterName;                                    		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              ScalarParameterValue;                             		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeBlendList.TgVectorParameterInfo
// 0x0018
struct FTgVectorParameterInfo
{
	int                                                MaterialId;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       ParameterName;                                    		// 0x0004 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     VectorParameterValue;                             		// 0x000C (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeBlendList.TgMorphTargetInfo
// 0x0054
struct FTgMorphTargetInfo
{
	class UMorphTarget*                                Target;                                           		// 0x0000 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	float                                              TargetWeight;                                     		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       TargetName;                                       		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FTgTextureParameterInfo >           TextureParameters;                                		// 0x0010 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FTgScalarParameterInfo >            ScalarParameters;                                 		// 0x001C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FTgVectorParameterInfo >            VectorParameters;                                 		// 0x0028 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FMorphCorrection >                  MorphCorrections;                                 		// 0x0034 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FName                                       MaxMorphWeightMasterGroupName;                    		// 0x0040 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              RandomChance;                                     		// 0x0048 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       Comment;                                          		// 0x004C (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeMasterSlider.ControlledNodeInfo
// 0x000C
struct FControlledNodeInfo
{
	struct FName                                       m_nmNodeName;                                     		// 0x0000 (0x0008) [0x0000000000020000]              ( CPF_EditConst )
	float                                              m_fWeight;                                        		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgMorphNodeMasterSlider.ControlledNodes
// 0x000C
struct FControlledNodes
{
	TArray< struct FControlledNodeInfo >               m_ControlledNodeInfo;                             		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgMorphNodePresets.MorphPreset
// 0x000C
struct FMorphPreset
{
	TArray< struct FMorphInfo >                        Preset;                                           		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgSpecialFx.Fx_Material
// 0x0014
struct FFx_Material
{
	int                                                nFxMatId;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nFxMatTypeCode;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FName                                       nmMIC;                                            		// 0x0008 (0x0008) [0x0000000000000000]              
	unsigned long                                      bApplyIfSameTeam : 1;                             		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgPawn_Detonator.DeathInfo
// 0x0014
struct FDeathInfo
{
	class AController*                                 Killer;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class UClass*                                      dmgType;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FVector                                     HitLocation;                                      		// 0x0008 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgPlayerController.InventoryChangeFlags
// 0x0004
struct FInventoryChangeFlags
{
	unsigned long                                      bDevice : 1;                                      		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bImplant : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bFlair : 1;                                       		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bDye : 1;                                         		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bBlueprint : 1;                                   		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      bComponent : 1;                                   		// 0x0000 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      bQuest : 1;                                       		// 0x0000 (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      bJetpackTrail : 1;                                		// 0x0000 (0x0004) [0x0000000000000000] [0x00000080] 
	unsigned long                                      bEquipped : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000100] 
};

// ScriptStruct TgGame.TgRandomSMManager.RandomSMActorInfo
// 0x0008
struct FRandomSMActorInfo
{
	int                                                nType;                                            		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                nCount;                                           		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgRepInfo_Game.MiniMapInfo
// 0x001C
struct FMiniMapInfo
{
	float                                              MinX;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              MinY;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              MaxX;                                             		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              MaxY;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                TextureResId;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	float                                              TextureUL;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              TextureVL;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgRepInfo_Game.MapLogoResIdInfo
// 0x0008
struct FMapLogoResIdInfo
{
	int                                                m_MapOwnerTextureResId;                           		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_MapContenderTextureResId;                       		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgRepInfo_Player.DeviceStatInfo
// 0x002C
struct FDeviceStatInfo
{
	int                                                Stats[ 0xB ];                                     		// 0x0000 (0x002C) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgRepInfo_TaskForce.TGTEAM_ENTRY
// 0x0028
struct FTGTEAM_ENTRY
{
	struct FString                                     fsName;                                           		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     fsMapName;                                        		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nHealth;                                          		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nMaxHealth;                                       		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      bLeader : 1;                                      		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	class ATgRepInfo_Player*                           pPrep;                                            		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgRepInfo_TaskForce.sPredefinedLevelDeployInfo
// 0x0010
struct FsPredefinedLevelDeployInfo
{
	int                                                m_Id;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FVector                                     m_Location;                                       		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgSkelCon_FootPlacement.ClassBasedJointInfo
// 0x0018
struct FClassBasedJointInfo
{
	struct FVector                                     m_JointTargetLocation;                            		// 0x0000 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	unsigned char                                      m_JointTargetLocationSpace;                       		// 0x000C (0x0001) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       m_JointTargetSpaceBoneName;                       		// 0x0010 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgSkelCon_LookAt.LookAtExtendBoneData
// 0x0018
struct FLookAtExtendBoneData
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	int                                                BoneIndex;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              PitchPct;                                         		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              YawPct;                                           		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              RollPct;                                          		// 0x0014 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgSkelCon_Twist.TwistBoneData
// 0x001C
struct FTwistBoneData
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	int                                                BoneIndex;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                BoneYawOffset;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                BoneAdj;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                BoneYawTolerance;                                 		// 0x0014 (0x0004) [0x0000000000000000]              
	unsigned long                                      bInMotion : 1;                                    		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgGame.TgSkelCon_Twist.LeanBoneData
// 0x0014
struct FLeanBoneData
{
	struct FName                                       BoneName;                                         		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	int                                                BonePitchLowerTolerance;                          		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                BonePitchUpperTolerance;                          		// 0x000C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	int                                                BoneIndex;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgSkeletalMeshActor_Composite.CompositeCharacterInfo
// 0x0040
struct FCompositeCharacterInfo
{
	int                                                nMeshId_T;                                        		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_T;                                         		// 0x0004 (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_H;                                        		// 0x0008 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_H;                                         		// 0x000C (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_GL;                                       		// 0x0010 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_GL;                                        		// 0x0014 (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_GR;                                       		// 0x0018 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_GR;                                        		// 0x001C (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_L;                                        		// 0x0020 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_L;                                         		// 0x0024 (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_BL;                                       		// 0x0028 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_BL;                                        		// 0x002C (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_BR;                                       		// 0x0030 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_BR;                                        		// 0x0034 (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
	int                                                nMeshId_C;                                        		// 0x0038 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               m_Mesh_C;                                         		// 0x003C (0x0004) [0x0000000000002001]              ( CPF_Edit | CPF_Transient )
};

// ScriptStruct TgGame.TgSkeletalMeshActorNPC.NPCAttachmentInfo
// 0x001C
struct FNPCAttachmentInfo
{
	class UStaticMesh*                                 StaticMesh;                                       		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class USkeletalMesh*                               SkeletalMesh;                                     		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     Scale3D;                                          		// 0x0008 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       SocketName;                                       		// 0x0014 (0x0008) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgSocketOffsetInfo.SocketOffsetInfo
// 0x007C
struct FSocketOffsetInfo
{
	struct FName                                       SocketName;                                       		// 0x0000 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FName                                       AimProfileName;                                   		// 0x0008 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     LU;                                               		// 0x0010 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     LC;                                               		// 0x001C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     LD;                                               		// 0x0028 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     CU;                                               		// 0x0034 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     CC;                                               		// 0x0040 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     CD;                                               		// 0x004C (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RU;                                               		// 0x0058 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RC;                                               		// 0x0064 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     RD;                                               		// 0x0070 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct TgGame.TgSoundGroup.FootstepSoundInfo
// 0x000C
struct FFootstepSoundInfo
{
	struct FName                                       MaterialType;                                     		// 0x0000 (0x0008) [0x0000000000000000]              
	class USoundCue*                                   Sound;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgSpecialFxLightManager.TgFxLight
// 0x000C
struct FTgFxLight
{
	struct FPointer                                    m_FxEmitter;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgSpecialFx*                                m_OwnerSpecialFx;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              m_Score;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgTeamScoreboard.PlayerIndexList
// 0x000C
struct FPlayerIndexList
{
	TArray< int >                                      PlayerIndice;                                     		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgGame.TgUIDataStore_StringList.EStringListData
// 0x0034
struct FEStringListData
{
	struct FName                                       Tag;                                              		// 0x0000 (0x0008) [0x0000000000000000]              
	struct FString                                     ColumnHeaderText;                                 		// 0x0008 (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
	struct FString                                     CurrentValue;                                     		// 0x0014 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                DefaultValueIndex;                                		// 0x0020 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           Strings;                                          		// 0x0024 (0x000C) [0x0000000000408002]              ( CPF_Const | CPF_Localized | CPF_NeedCtorLink )
	class UTgUIDataProvider_StringArray*               DataProvider;                                     		// 0x0030 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct TgGame.TgUIHexWidget.HexData
// 0x00B4
struct FHexData
{
	class UTgStaticMeshComponent*                      UnderlayMesh;                                     		// 0x0000 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   UnderlayMIC;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UTgStaticMeshComponent*                      TileMesh;                                         		// 0x0008 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   TileMIC;                                          		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgStaticMeshComponent*                      ButtonMesh;                                       		// 0x0010 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   ButtonMIC;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UTgStaticMeshComponent*                      OverlayMesh;                                      		// 0x0018 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   OverlayMIC;                                       		// 0x001C (0x0004) [0x0000000000000000]              
	class UTgStaticMeshComponent*                      BorderMesh;                                       		// 0x0020 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   BorderMIC;                                        		// 0x0024 (0x0004) [0x0000000000000000]              
	class UTgStaticMeshComponent*                      IndicatorMesh;                                    		// 0x0028 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UMaterialInstanceConstant*                   IndicatorMIC;                                     		// 0x002C (0x0004) [0x0000000000000000]              
	struct FVector                                     ScreenLocation;                                   		// 0x0030 (0x000C) [0x0000000000000000]              
	unsigned long                                      bHidden : 1;                                      		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bFilteredOut : 1;                                 		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned char                                      BoundaryState;                                    		// 0x0040 (0x0001) [0x0000000000000000]              
	int                                                Position;                                         		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                BuildingTypeId;                                   		// 0x0048 (0x0004) [0x0000000000000000]              
	int                                                BuildingSubTypeId;                                		// 0x004C (0x0004) [0x0000000000000000]              
	int                                                BuildingId;                                       		// 0x0050 (0x0004) [0x0000000000000000]              
	int                                                AttackerTeam;                                     		// 0x0054 (0x0004) [0x0000000000000000]              
	int                                                DefenderTeam;                                     		// 0x0058 (0x0004) [0x0000000000000000]              
	unsigned long                                      Blocked : 1;                                      		// 0x005C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bClosed : 1;                                      		// 0x005C (0x0004) [0x0000000000000000] [0x00000002] 
	float                                              fPercentComplete;                                 		// 0x0060 (0x0004) [0x0000000000000000]              
	unsigned long                                      DefenderOwned : 1;                                		// 0x0064 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                BonusType;                                        		// 0x0068 (0x0004) [0x0000000000000000]              
	unsigned long                                      bRegionHighlight : 1;                             		// 0x006C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                RegionId;                                         		// 0x0070 (0x0004) [0x0000000000000000]              
	int                                                TerritoryId;                                      		// 0x0074 (0x0004) [0x0000000000000000]              
	int                                                OwnerAgencyId;                                    		// 0x0078 (0x0004) [0x0000000000000000]              
	int                                                AttackerSFId;                                     		// 0x007C (0x0004) [0x0000000000000000]              
	int                                                AttackerSFNumPlayers;                             		// 0x0080 (0x0004) [0x0000000000000000]              
	int                                                AttackerSFAgencyId;                               		// 0x0084 (0x0004) [0x0000000000000000]              
	int                                                DefenderSFId;                                     		// 0x0088 (0x0004) [0x0000000000000000]              
	int                                                DefenderSFNumPlayers;                             		// 0x008C (0x0004) [0x0000000000000000]              
	unsigned long                                      bBaseProtected : 1;                               		// 0x0090 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nDefenseOrigin;                                   		// 0x0094 (0x0004) [0x0000000000000000]              
	int                                                nDefensePosition;                                 		// 0x0098 (0x0004) [0x0000000000000000]              
	unsigned long                                      bAuction : 1;                                     		// 0x009C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                BidderSFId;                                       		// 0x00A0 (0x0004) [0x0000000000000000]              
	int                                                BidderSFAgencyId;                                 		// 0x00A4 (0x0004) [0x0000000000000000]              
	unsigned long                                      bHeadquarters : 1;                                		// 0x00A8 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nBonusResourceType;                               		// 0x00AC (0x0004) [0x0000000000000000]              
	unsigned long                                      bRareResource : 1;                                		// 0x00B0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bForge : 1;                                       		// 0x00B0 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bBiddingAllowed : 1;                              		// 0x00B0 (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct TgGame.TgUIHexWidget.ZoomLock
// 0x0004
struct FZoomLock
{
	unsigned long                                      bCanZoomOut : 1;                                  		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bCanZoomIn : 1;                                   		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bPanLock : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bCanPanLeft : 1;                                  		// 0x0000 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bCanPanRight : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      bCanPanUp : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      bCanPanDown : 1;                                  		// 0x0000 (0x0004) [0x0000000000000000] [0x00000040] 
};

// ScriptStruct TgGame.TgUIHexWidget.HexMICParams
// 0x0058
struct FHexMICParams
{
	struct FLinearColor                                Position;                                         		// 0x0000 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                Scale;                                            		// 0x0010 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                OwnerColor;                                       		// 0x0020 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                AttackerColor;                                    		// 0x0030 (0x0010) [0x0000000000000000]              
	unsigned long                                      bCombat : 1;                                      		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nStrategicState;                                  		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                nResourceType;                                    		// 0x0048 (0x0004) [0x0000000000000000]              
	int                                                nFacilityType;                                    		// 0x004C (0x0004) [0x0000000000000000]              
	unsigned long                                      bHeadquarters : 1;                                		// 0x0050 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bFill : 1;                                        		// 0x0050 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bClosed : 1;                                      		// 0x0050 (0x0004) [0x0000000000000000] [0x00000004] 
	float                                              fPercentComplete;                                 		// 0x0054 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIHexWidget.ScaleData
// 0x0014
struct FScaleData
{
	float                                              Scale;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              ResScale;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              Ratio;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FVector2D                                   ViewportSize;                                     		// 0x000C (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIHexWidget_Zone.HexAgencyData
// 0x0020
struct FHexAgencyData
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     Name;                                             		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FLinearColor                                Color;                                            		// 0x0010 (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIInvWidget.sTimedParameter
// 0x0009
struct FsTimedParameter
{
	float                                              fTimeStarted;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              fTimeDuration;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned char                                      eType;                                            		// 0x0008 (0x0001) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZoneMapStruct
// 0x0019
struct FZoneMapStruct
{
	struct FVector                                     ScreenLocation;                                   		// 0x0000 (0x000C) [0x0000000000000000]              
	struct FVector                                     ScreenSize;                                       		// 0x000C (0x000C) [0x0000000000000000]              
	unsigned char                                      eType;                                            		// 0x0018 (0x0001) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZoneObjectiveStruct
// 0x0013(0x002C - 0x0019)
struct FZoneObjectiveStruct : FZoneMapStruct
{
	int                                                Priority;                                         		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      Visible : 1;                                      		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      Selectable : 1;                                   		// 0x0020 (0x0004) [0x0000000000000000] [0x00000002] 
	class UMaterialInstanceConstant*                   IconMIC;                                          		// 0x0024 (0x0004) [0x0000000000000000]              
	class ATgMissionObjective*                         Objective;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZonePlayerStruct
// 0x0007(0x0020 - 0x0019)
struct FZonePlayerStruct : FZoneMapStruct
{
	class ATgRepInfo_Player*                           PRI;                                              		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZoneVolumeStruct
// 0x0007(0x0020 - 0x0019)
struct FZoneVolumeStruct : FZoneMapStruct
{
	class ATgOmegaVolume*                              UIVol;                                            		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZoneQuestVolumeStruct
// 0x0018(0x0038 - 0x0020)
struct FZoneQuestVolumeStruct : FZoneVolumeStruct
{
	TArray< int >                                      AssociatedQuests;                                 		// 0x0020 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned long                                      bTracked : 1;                                     		// 0x002C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bComplete : 1;                                    		// 0x002C (0x0004) [0x0000000000000000] [0x00000002] 
	struct FPointer                                    pIcon;                                            		// 0x0030 (0x0004) [0x0000000000001000]              ( CPF_Native )
	class UTexture2D*                                  Texture;                                          		// 0x0034 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZoneQuestPOIStruct
// 0x001F(0x0038 - 0x0019)
struct FZoneQuestPOIStruct : FZoneMapStruct
{
	class ATgPointOfInterest*                          POI;                                              		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      bTracked : 1;                                     		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bComplete : 1;                                    		// 0x0020 (0x0004) [0x0000000000000000] [0x00000002] 
	TArray< int >                                      AssociatedQuests;                                 		// 0x0024 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FPointer                                    pIcon;                                            		// 0x0030 (0x0004) [0x0000000000001000]              ( CPF_Native )
	class UTexture2D*                                  Texture;                                          		// 0x0034 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgUIMapWidget.ZonePortalStruct
// 0x000B(0x0024 - 0x0019)
struct FZonePortalStruct : FZoneMapStruct
{
	class ATgStartPoint*                               pStartPoint;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   IconMIC;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgWindManager.TgWindGustInfo
// 0x0010
struct FTgWindGustInfo
{
	float                                              m_fTimeTillActive;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              m_fActiveTime;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              m_fWindForceStrength;                             		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              m_fWindFrequency;                                 		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgGame.TgWindManager.TgAffectedWindAssetInfo
// 0x0014
struct FTgAffectedWindAssetInfo
{
	class ATgKAsset_ClientSideSim*                     m_TgKAsset;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              m_fDistanceAway;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	TArray< struct FTgWindGustInfo >                   m_GustInfo;                                       		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif