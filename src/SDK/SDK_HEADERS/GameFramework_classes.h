/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: GameFramework_classes.h
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
# Constants
# ========================================================================================= #
*/


/*
# ========================================================================================= #
# Enums
# ========================================================================================= #
*/

// Enum GameFramework.GameHUD.EGameHUDColor
/*enum EGameHUDColor
{
	GHD_WHITE                                          = 0,
	GHD_BLACK                                          = 1,
	GHD_MAX                                            = 2
};*/

// Enum GameFramework.GameSkelCtrl_Recoil.ERecoilStart
/*enum ERecoilStart
{
	ERS_Zero                                           = 0,
	ERS_Random                                         = 1,
	ERS_MAX                                            = 2
};*/


/*
# ========================================================================================= #
# Classes
# ========================================================================================= #
*/

// Class GameFramework.GameAIController
// 0x0018 (0x0384 - 0x036C)
class AGameAIController : public AAIController
{
public:
	class UGameAICommand*                              CommandList;                                      		// 0x036C (0x0004) [0x0000000000000002]              ( CPF_Const )
	unsigned long                                      bHasRunawayCommandList : 1;                       		// 0x0370 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bAILogging : 1;                                   		// 0x0370 (0x0004) [0x0000000000004001] [0x00000002] ( CPF_Edit | CPF_Config )
	unsigned long                                      bAILogToWindow : 1;                               		// 0x0370 (0x0004) [0x0000000000004001] [0x00000004] ( CPF_Edit | CPF_Config )
	unsigned long                                      bFlushAILogEachLine : 1;                          		// 0x0370 (0x0004) [0x0000000000004001] [0x00000008] ( CPF_Edit | CPF_Config )
	unsigned long                                      bMapBasedLogName : 1;                             		// 0x0370 (0x0004) [0x0000000000004001] [0x00000010] ( CPF_Edit | CPF_Config )
	class AFileLog*                                    AILogFile;                                        		// 0x0374 (0x0004) [0x0000000000000000]              
	TArray< struct FName >                             AILogFilter;                                      		// 0x0378 (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1158 ];

		return pClassPointer;
	};

	void SetDesiredRotation ( struct FRotator TargetDesiredRotation, unsigned long bZeroPitch );
	void eventAILog_Internal ( struct FString LogText, struct FName LogCategory, unsigned long bForce );
	void RecordDemoAILog ( struct FString LogText );
	void eventDestroyed ( );
	class UGameAICommand* FindCommandOfClass ( class UClass* SearchClass );
	void DumpCommandStack ( );
	void CheckCommandCount ( );
	class UGameAICommand* GetActiveCommand ( );
	bool AbortCommand ( class UGameAICommand* AbortCmd, class UClass* AbortClass );
	void PopCommand ( class UGameAICommand* NewCommand );
	void PushCommand ( class UGameAICommand* NewCommand );
};


// Class GameFramework.GameAICommand
// 0x001C (0x0058 - 0x003C)
class UGameAICommand : public UObject
{
public:
	class UGameAICommand*                              ChildCommand;                                     		// 0x003C (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       ChildStatus;                                      		// 0x0040 (0x0008) [0x0000000000000002]              ( CPF_Const )
	class AGameAIController*                           GameAIOwner;                                      		// 0x0048 (0x0004) [0x0000000000000000]              
	struct FName                                       Status;                                           		// 0x004C (0x0008) [0x0000000000000000]              
	unsigned long                                      bAllowNewSameClassInstance : 1;                   		// 0x0054 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bReplaceActiveSameClassInstance : 1;              		// 0x0054 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bAborted : 1;                                     		// 0x0054 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bIgnoreNotifies : 1;                              		// 0x0054 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      bPendingPop : 1;                                  		// 0x0054 (0x0004) [0x0000000000000000] [0x00000010] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1159 ];

		return pClassPointer;
	};

	struct FString eventGetDumpString ( );
	void Resumed ( struct FName OldCommandName );
	void Paused ( class UGameAICommand* NewCommand );
	void Popped ( );
	void Pushed ( );
	void PostPopped ( );
	void PrePushed ( class AGameAIController* AI );
	bool AllowTransitionTo ( class UClass* AttemptCommand );
	void Tick ( float DeltaTime );
	bool ShouldIgnoreNotifies ( );
	void eventInternalTick ( float DeltaTime );
	void eventInternalResumed ( struct FName OldCommandName );
	void eventInternalPaused ( class UGameAICommand* NewCommand );
	void eventInternalPopped ( );
	void eventInternalPushed ( );
	void eventInternalPrePushed ( class AGameAIController* AI );
	bool InitCommand ( class AGameAIController* AI );
	bool InitCommandUserActor ( class AGameAIController* AI, class AActor* UserActor );
};


// Class GameFramework.GameExplosion
// 0x0074 (0x00B0 - 0x003C)
class UGameExplosion : public UObject
{
public:
	float                                              DamageDelay;                                      		// 0x003C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              Damage;                                           		// 0x0040 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DamageRadius;                                     		// 0x0044 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              DamageFalloffExponent;                            		// 0x0048 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AActor*                                      ActorToIgnoreForDamage;                           		// 0x004C (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UClass*                                      ActorClassToIgnoreForDamage;                      		// 0x0050 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UClass*                                      ActorClassToIgnoreForKnockdownsAndCringes;        		// 0x0054 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	unsigned long                                      bAllowTeammateCringes : 1;                        		// 0x0058 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bFullDamageToAttachee : 1;                        		// 0x0058 (0x0004) [0x0000000000002000] [0x00000002] ( CPF_Transient )
	unsigned long                                      bAttachExplosionEmitterToAttachee : 1;            		// 0x0058 (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )
	unsigned long                                      bSuppressAudio : 1;                               		// 0x0058 (0x0004) [0x0000000000000001] [0x00000008] ( CPF_Edit )
	unsigned long                                      bCausesFracture : 1;                              		// 0x0058 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
	unsigned long                                      bAllowPerMaterialFX : 1;                          		// 0x0058 (0x0004) [0x0000000000000001] [0x00000020] ( CPF_Edit )
	unsigned long                                      bParticleSystemIsBeingOverriddenDontUsePhysMatVersion : 1;		// 0x0058 (0x0004) [0x0000000000000001] [0x00000040] ( CPF_Edit )
	unsigned long                                      bUseMapSpecificValues : 1;                        		// 0x0058 (0x0004) [0x0000000000000001] [0x00000080] ( CPF_Edit )
	unsigned long                                      bUseOverlapCheck : 1;                             		// 0x0058 (0x0004) [0x0000000000000001] [0x00000100] ( CPF_Edit )
	class UClass*                                      MyDamageType;                                     		// 0x005C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              KnockDownRadius;                                  		// 0x0060 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              KnockDownStrength;                                		// 0x0064 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              CringeRadius;                                     		// 0x0068 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              MomentumTransferScale;                            		// 0x006C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UParticleSystem*                             ParticleEmitterTemplate;                          		// 0x0070 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              ExplosionEmitterScale;                            		// 0x0074 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AActor*                                      HitActor;                                         		// 0x0078 (0x0004) [0x0000000000000000]              
	struct FVector                                     HitLocation;                                      		// 0x007C (0x000C) [0x0000000000000000]              
	struct FVector                                     HitNormal;                                        		// 0x0088 (0x000C) [0x0000000000000000]              
	class USoundCue*                                   ExplosionSound;                                   		// 0x0094 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class UPointLightComponent*                        ExploLight;                                       		// 0x0098 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	float                                              ExploLightFadeOutTime;                            		// 0x009C (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FractureMeshRadius;                               		// 0x00A0 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              FracturePartVel;                                  		// 0x00A4 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AActor*                                      Attachee;                                         		// 0x00A8 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	class AController*                                 AttacheeController;                               		// 0x00AC (0x0004) [0x0000000000000001]              ( CPF_Edit )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1160 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameExplosionActor
// 0x002C (0x01F8 - 0x01CC)
class AGameExplosionActor : public AActor
{
public:
	unsigned long                                      bHasExploded : 1;                                 		// 0x01CC (0x0004) [0x0000000000002000] [0x00000001] ( CPF_Transient )
	unsigned long                                      bActiveReloadBonusActive : 1;                     		// 0x01CC (0x0004) [0x0000000000000000] [0x00000002] 
	class UPointLightComponent*                        ExplosionLight;                                   		// 0x01D0 (0x0004) [0x0000000004082008]              ( CPF_ExportObject | CPF_Transient | CPF_Component | CPF_EditInline )
	float                                              LightFadeTime;                                    		// 0x01D4 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	float                                              LightInitialBrightness;                           		// 0x01D8 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UGameExplosion*                              ExplosionTemplate;                                		// 0x01DC (0x0004) [0x0000000000000000]              
	class URB_RadialImpulseComponent*                  RadialImpulseComponent;                           		// 0x01E0 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class AController*                                 InstigatorController;                             		// 0x01E4 (0x0004) [0x0000000000000000]              
	class AActor*                                      HitActorFromPhysMaterialTrace;                    		// 0x01E8 (0x0004) [0x0000000000000000]              
	struct FVector                                     HitLocationFromPhysMaterialTrace;                 		// 0x01EC (0x000C) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1161 ];

		return pClassPointer;
	};

	void Tick ( float DeltaTime );
	void DoExplosionDamage ( );
	void eventExplode ( class UGameExplosion* NewExplosionTemplate );
	void SpawnExplosionFogVolume ( );
	void DoExplosionCameraShakes ( );
	void SpawnExplosionDecal ( );
	void SpawnExplosionParticleSystem ( class UParticleSystem* Template );
	void UpdateExplosionTemplateWithPerMaterialFX ( class UPhysicalMaterial* PhysMaterial );
	void KnockdownPawn ( class AGamePawn* Victim, float DistFromExplosion );
	void DoCringesAndKnockdowns ( );
	bool ShouldDoCringeFor ( class AGamePawn* Victim );
	void DoBreakFracturedMeshes ( struct FVector ExploOrigin, float DamageRadius, float RBStrength, class UClass* dmgType );
	void HurtExplosion ( float BaseDamage, float DamageRadius, float DamageFalloffExp, class UClass* DamageType, float MomentumScale, struct FVector ExploOrigin, class AActor* IgnoredActor, class UClass* ActorClassToIgnoreForDamage, class AController* InstigatedByController, unsigned long bDoFullDamage );
	bool DoFullDamageToActor ( class AActor* Victim );
	class UPhysicalMaterial* GetPhysicalMaterial ( );
	void eventPreBeginPlay ( );
};


// Class GameFramework.GameHUD
// 0x0024 (0x0488 - 0x0464)
class AGameHUD : public AHUD
{
public:
	float                                              TotalTitleDrawTime;                               		// 0x0464 (0x0004) [0x0000000000000000]              
	float                                              TotalTitleFadeTime;                               		// 0x0468 (0x0004) [0x0000000000000000]              
	float                                              TitleDrawStartTime;                               		// 0x046C (0x0004) [0x0000000000000000]              
	struct FString                                     ChapterTitleString;                               		// 0x0470 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     ActTitleString;                                   		// 0x047C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1162 ];

		return pClassPointer;
	};

	void StopDrawingChapterTitle ( );
	void StartDrawingChapterTitle ( struct FString ChapterName, struct FString ActName, float TotalDrawTime, float TotalFadeTime );
	void SetHUDDrawColor ( unsigned char eColor, unsigned char Alpha );
};


// Class GameFramework.GamePawn
// 0x0004 (0x03D0 - 0x03CC)
class AGamePawn : public APawn
{
public:
	unsigned long                                      bRespondToExplosions : 1;                         		// 0x03CC (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1163 ];

		return pClassPointer;
	};

	void eventCringe ( float Duration );
	void ServerKnockdown ( struct FVector RBLinearVelocity, struct FVector RBAngularVelocity, struct FVector RadialOrigin, float RadialRadius, float RadialStrength, struct FVector PointImpulse, struct FVector PointImpulsePosition, struct FName PointImpulseBoneName );
	void GetTargetFrictionCylinder ( float* CylinderRadius, float* CylinderHeight );
};


// Class GameFramework.GamePlayerController
// 0x0000 (0x05C0 - 0x05C0)
class AGamePlayerController : public APlayerController
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1164 ];

		return pClassPointer;
	};

};


// Class GameFramework.GamePlayerInput
// 0x0000 (0x0168 - 0x0168)
class UGamePlayerInput : public UPlayerInput
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1165 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameProjectile
// 0x0000 (0x0208 - 0x0208)
class AGameProjectile : public AProjectile
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1166 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameTypes
// 0x0000 (0x003C - 0x003C)
class UGameTypes : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1167 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameVehicle
// 0x0000 (0x0550 - 0x0550)
class AGameVehicle : public ASVehicle
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1168 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameWeapon
// 0x0000 (0x0218 - 0x0218)
class AGameWeapon : public AWeapon
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1169 ];

		return pClassPointer;
	};

};


// Class GameFramework.SeqAct_ModifyProperty
// 0x000C (0x00F4 - 0x00E8)
class USeqAct_ModifyProperty : public USequenceAction
{
public:
	TArray< struct FPropertyInfo >                     Properties;                                       		// 0x00E8 (0x000C) [0x0000000004400001]              ( CPF_Edit | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1170 ];

		return pClassPointer;
	};

};


// Class GameFramework.GameSkelCtrl_Recoil
// 0x0098 (0x014C - 0x00B4)
class UGameSkelCtrl_Recoil : public USkelControlBase
{
public:
	unsigned long                                      bBoneSpaceRecoil : 1;                             		// 0x00B4 (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bPlayRecoil : 1;                                  		// 0x00B4 (0x0004) [0x0000000000002001] [0x00000002] ( CPF_Edit | CPF_Transient )
	unsigned long                                      bOldPlayRecoil : 1;                               		// 0x00B4 (0x0004) [0x0000000000002000] [0x00000004] ( CPF_Transient )
	unsigned long                                      bApplyControl : 1;                                		// 0x00B4 (0x0004) [0x0000000000002000] [0x00000008] ( CPF_Transient )
	unsigned long                                      m_bBlendIn : 1;                                   		// 0x00B4 (0x0004) [0x0000000000000001] [0x00000010] ( CPF_Edit )
	struct FRecoilDef                                  Recoil;                                           		// 0x00B8 (0x0070) [0x0000000000000001]              ( CPF_Edit )
	struct FVector2D                                   Aim;                                              		// 0x0128 (0x0008) [0x0000000000000001]              ( CPF_Edit )
	float                                              m_bBlendInRemainingTime;                          		// 0x0130 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FVector                                     m_BlendLoc;                                       		// 0x0134 (0x000C) [0x0000000000002000]              ( CPF_Transient )
	struct FRotator                                    m_BlendRot;                                       		// 0x0140 (0x000C) [0x0000000000002000]              ( CPF_Transient )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1171 ];

		return pClassPointer;
	};

};



#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
