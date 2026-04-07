#pragma once
#include "src/pch.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"

/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: GameFramework_functions.h
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
# Functions
# ========================================================================================= #
*/

// Function GameFramework.GameAIController.SetDesiredRotation
// [0x00024002] 
// Parameters infos:
// struct FRotator                TargetDesiredRotation          ( CPF_Parm )
// unsigned long                  bZeroPitch                     ( CPF_OptionalParm | CPF_Parm )

void AGameAIController::SetDesiredRotation ( struct FRotator TargetDesiredRotation, unsigned long bZeroPitch )
{
	static UFunction* pFnSetDesiredRotation = NULL;

	if ( ! pFnSetDesiredRotation )
		pFnSetDesiredRotation = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.SetDesiredRotation" );

	AGameAIController_execSetDesiredRotation_Parms SetDesiredRotation_Parms;
	memcpy ( &SetDesiredRotation_Parms.TargetDesiredRotation, &TargetDesiredRotation, 0xC );
	SetDesiredRotation_Parms.bZeroPitch = bZeroPitch;

	this->ProcessEvent ( pFnSetDesiredRotation, &SetDesiredRotation_Parms, NULL );
};

// Function GameFramework.GameAIController.AILog_Internal
// [0x00024800] ( FUNC_Event )
// Parameters infos:
// struct FString                 LogText                        ( CPF_Parm | CPF_CoerceParm | CPF_NeedCtorLink )
// struct FName                   LogCategory                    ( CPF_OptionalParm | CPF_Parm )
// unsigned long                  bForce                         ( CPF_OptionalParm | CPF_Parm )

void AGameAIController::eventAILog_Internal ( struct FString LogText, struct FName LogCategory, unsigned long bForce )
{
	static UFunction* pFnAILog_Internal = NULL;

	if ( ! pFnAILog_Internal )
		pFnAILog_Internal = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.AILog_Internal" );

	AGameAIController_eventAILog_Internal_Parms AILog_Internal_Parms;
	memcpy ( &AILog_Internal_Parms.LogText, &LogText, 0xC );
	memcpy ( &AILog_Internal_Parms.LogCategory, &LogCategory, 0x8 );
	AILog_Internal_Parms.bForce = bForce;

	this->ProcessEvent ( pFnAILog_Internal, &AILog_Internal_Parms, NULL );
};

// Function GameFramework.GameAIController.RecordDemoAILog
// [0x00080000] 
// Parameters infos:
// struct FString                 LogText                        ( CPF_Parm | CPF_CoerceParm | CPF_NeedCtorLink )

void AGameAIController::RecordDemoAILog ( struct FString LogText )
{
	static UFunction* pFnRecordDemoAILog = NULL;

	if ( ! pFnRecordDemoAILog )
		pFnRecordDemoAILog = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.RecordDemoAILog" );

	AGameAIController_execRecordDemoAILog_Parms RecordDemoAILog_Parms;
	memcpy ( &RecordDemoAILog_Parms.LogText, &LogText, 0xC );

	this->ProcessEvent ( pFnRecordDemoAILog, &RecordDemoAILog_Parms, NULL );
};

// Function GameFramework.GameAIController.Destroyed
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void AGameAIController::eventDestroyed ( )
{
	static UFunction* pFnDestroyed = NULL;

	if ( ! pFnDestroyed )
		pFnDestroyed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.Destroyed" );

	AGameAIController_eventDestroyed_Parms Destroyed_Parms;

	this->ProcessEvent ( pFnDestroyed, &Destroyed_Parms, NULL );
};

// Function GameFramework.GameAIController.FindCommandOfClass
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UGameAICommand*          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_CoerceParm )
// class UClass*                  SearchClass                    ( CPF_Parm )

class UGameAICommand* AGameAIController::FindCommandOfClass ( class UClass* SearchClass )
{
	static UFunction* pFnFindCommandOfClass = NULL;

	if ( ! pFnFindCommandOfClass )
		pFnFindCommandOfClass = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.FindCommandOfClass" );

	AGameAIController_execFindCommandOfClass_Parms FindCommandOfClass_Parms;
	FindCommandOfClass_Parms.SearchClass = SearchClass;

	pFnFindCommandOfClass->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnFindCommandOfClass, &FindCommandOfClass_Parms, NULL );

	pFnFindCommandOfClass->FunctionFlags |= 0x400;

	return FindCommandOfClass_Parms.ReturnValue;
};

// Function GameFramework.GameAIController.DumpCommandStack
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:

void AGameAIController::DumpCommandStack ( )
{
	static UFunction* pFnDumpCommandStack = NULL;

	if ( ! pFnDumpCommandStack )
		pFnDumpCommandStack = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.DumpCommandStack" );

	AGameAIController_execDumpCommandStack_Parms DumpCommandStack_Parms;

	pFnDumpCommandStack->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnDumpCommandStack, &DumpCommandStack_Parms, NULL );

	pFnDumpCommandStack->FunctionFlags |= 0x400;
};

// Function GameFramework.GameAIController.CheckCommandCount
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:

void AGameAIController::CheckCommandCount ( )
{
	static UFunction* pFnCheckCommandCount = NULL;

	if ( ! pFnCheckCommandCount )
		pFnCheckCommandCount = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.CheckCommandCount" );

	AGameAIController_execCheckCommandCount_Parms CheckCommandCount_Parms;

	pFnCheckCommandCount->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnCheckCommandCount, &CheckCommandCount_Parms, NULL );

	pFnCheckCommandCount->FunctionFlags |= 0x400;
};

// Function GameFramework.GameAIController.GetActiveCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UGameAICommand*          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

class UGameAICommand* AGameAIController::GetActiveCommand ( )
{
	static UFunction* pFnGetActiveCommand = NULL;

	if ( ! pFnGetActiveCommand )
		pFnGetActiveCommand = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.GetActiveCommand" );

	AGameAIController_execGetActiveCommand_Parms GetActiveCommand_Parms;

	pFnGetActiveCommand->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnGetActiveCommand, &GetActiveCommand_Parms, NULL );

	pFnGetActiveCommand->FunctionFlags |= 0x400;

	return GetActiveCommand_Parms.ReturnValue;
};

// Function GameFramework.GameAIController.AbortCommand
// [0x00024401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class UGameAICommand*          AbortCmd                       ( CPF_Parm )
// class UClass*                  AbortClass                     ( CPF_OptionalParm | CPF_Parm )

bool AGameAIController::AbortCommand ( class UGameAICommand* AbortCmd, class UClass* AbortClass )
{
	static UFunction* pFnAbortCommand = NULL;

	if ( ! pFnAbortCommand )
		pFnAbortCommand = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.AbortCommand" );

	AGameAIController_execAbortCommand_Parms AbortCommand_Parms;
	AbortCommand_Parms.AbortCmd = AbortCmd;
	AbortCommand_Parms.AbortClass = AbortClass;

	pFnAbortCommand->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnAbortCommand, &AbortCommand_Parms, NULL );

	pFnAbortCommand->FunctionFlags |= 0x400;

	return AbortCommand_Parms.ReturnValue;
};

// Function GameFramework.GameAIController.PopCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UGameAICommand*          NewCommand                     ( CPF_Parm )

void AGameAIController::PopCommand ( class UGameAICommand* NewCommand )
{
	static UFunction* pFnPopCommand = NULL;

	if ( ! pFnPopCommand )
		pFnPopCommand = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.PopCommand" );

	AGameAIController_execPopCommand_Parms PopCommand_Parms;
	PopCommand_Parms.NewCommand = NewCommand;

	pFnPopCommand->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnPopCommand, &PopCommand_Parms, NULL );

	pFnPopCommand->FunctionFlags |= 0x400;
};

// Function GameFramework.GameAIController.PushCommand
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UGameAICommand*          NewCommand                     ( CPF_Parm )

void AGameAIController::PushCommand ( class UGameAICommand* NewCommand )
{
	static UFunction* pFnPushCommand = NULL;

	if ( ! pFnPushCommand )
		pFnPushCommand = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAIController.PushCommand" );

	AGameAIController_execPushCommand_Parms PushCommand_Parms;
	PushCommand_Parms.NewCommand = NewCommand;

	pFnPushCommand->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnPushCommand, &PushCommand_Parms, NULL );

	pFnPushCommand->FunctionFlags |= 0x400;
};

// Function GameFramework.GameAICommand.GetDumpString
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UGameAICommand::eventGetDumpString ( )
{
	static UFunction* pFnGetDumpString = NULL;

	if ( ! pFnGetDumpString )
		pFnGetDumpString = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.GetDumpString" );

	UGameAICommand_eventGetDumpString_Parms GetDumpString_Parms;

	this->ProcessEvent ( pFnGetDumpString, &GetDumpString_Parms, NULL );

	return GetDumpString_Parms.ReturnValue;
};

// Function GameFramework.GameAICommand.Resumed
// [0x00020002] 
// Parameters infos:
// struct FName                   OldCommandName                 ( CPF_Parm )

void UGameAICommand::Resumed ( struct FName OldCommandName )
{
	static UFunction* pFnResumed = NULL;

	if ( ! pFnResumed )
		pFnResumed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.Resumed" );

	UGameAICommand_execResumed_Parms Resumed_Parms;
	memcpy ( &Resumed_Parms.OldCommandName, &OldCommandName, 0x8 );

	this->ProcessEvent ( pFnResumed, &Resumed_Parms, NULL );
};

// Function GameFramework.GameAICommand.Paused
// [0x00020002] 
// Parameters infos:
// class UGameAICommand*          NewCommand                     ( CPF_Parm )

void UGameAICommand::Paused ( class UGameAICommand* NewCommand )
{
	static UFunction* pFnPaused = NULL;

	if ( ! pFnPaused )
		pFnPaused = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.Paused" );

	UGameAICommand_execPaused_Parms Paused_Parms;
	Paused_Parms.NewCommand = NewCommand;

	this->ProcessEvent ( pFnPaused, &Paused_Parms, NULL );
};

// Function GameFramework.GameAICommand.Popped
// [0x00020002] 
// Parameters infos:

void UGameAICommand::Popped ( )
{
	static UFunction* pFnPopped = NULL;

	if ( ! pFnPopped )
		pFnPopped = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.Popped" );

	UGameAICommand_execPopped_Parms Popped_Parms;

	this->ProcessEvent ( pFnPopped, &Popped_Parms, NULL );
};

// Function GameFramework.GameAICommand.Pushed
// [0x00020002] 
// Parameters infos:

void UGameAICommand::Pushed ( )
{
	static UFunction* pFnPushed = NULL;

	if ( ! pFnPushed )
		pFnPushed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.Pushed" );

	UGameAICommand_execPushed_Parms Pushed_Parms;

	this->ProcessEvent ( pFnPushed, &Pushed_Parms, NULL );
};

// Function GameFramework.GameAICommand.PostPopped
// [0x00020000] 
// Parameters infos:

void UGameAICommand::PostPopped ( )
{
	static UFunction* pFnPostPopped = NULL;

	if ( ! pFnPostPopped )
		pFnPostPopped = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.PostPopped" );

	UGameAICommand_execPostPopped_Parms PostPopped_Parms;

	this->ProcessEvent ( pFnPostPopped, &PostPopped_Parms, NULL );
};

// Function GameFramework.GameAICommand.PrePushed
// [0x00020000] 
// Parameters infos:
// class AGameAIController*       AI                             ( CPF_Parm )

void UGameAICommand::PrePushed ( class AGameAIController* AI )
{
	static UFunction* pFnPrePushed = NULL;

	if ( ! pFnPrePushed )
		pFnPrePushed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.PrePushed" );

	UGameAICommand_execPrePushed_Parms PrePushed_Parms;
	PrePushed_Parms.AI = AI;

	this->ProcessEvent ( pFnPrePushed, &PrePushed_Parms, NULL );
};

// Function GameFramework.GameAICommand.AllowTransitionTo
// [0x00020002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class UClass*                  AttemptCommand                 ( CPF_Parm )

bool UGameAICommand::AllowTransitionTo ( class UClass* AttemptCommand )
{
	static UFunction* pFnAllowTransitionTo = NULL;

	if ( ! pFnAllowTransitionTo )
		pFnAllowTransitionTo = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.AllowTransitionTo" );

	UGameAICommand_execAllowTransitionTo_Parms AllowTransitionTo_Parms;
	AllowTransitionTo_Parms.AttemptCommand = AttemptCommand;

	this->ProcessEvent ( pFnAllowTransitionTo, &AllowTransitionTo_Parms, NULL );

	return AllowTransitionTo_Parms.ReturnValue;
};

// Function GameFramework.GameAICommand.Tick
// [0x00020000] 
// Parameters infos:
// float                          DeltaTime                      ( CPF_Parm )

void UGameAICommand::Tick ( float DeltaTime )
{
	static UFunction* pFnTick = NULL;

	if ( ! pFnTick )
		pFnTick = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.Tick" );

	UGameAICommand_execTick_Parms Tick_Parms;
	Tick_Parms.DeltaTime = DeltaTime;

	this->ProcessEvent ( pFnTick, &Tick_Parms, NULL );
};

// Function GameFramework.GameAICommand.ShouldIgnoreNotifies
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

bool UGameAICommand::ShouldIgnoreNotifies ( )
{
	static UFunction* pFnShouldIgnoreNotifies = NULL;

	if ( ! pFnShouldIgnoreNotifies )
		pFnShouldIgnoreNotifies = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.ShouldIgnoreNotifies" );

	UGameAICommand_execShouldIgnoreNotifies_Parms ShouldIgnoreNotifies_Parms;

	pFnShouldIgnoreNotifies->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnShouldIgnoreNotifies, &ShouldIgnoreNotifies_Parms, NULL );

	pFnShouldIgnoreNotifies->FunctionFlags |= 0x400;

	return ShouldIgnoreNotifies_Parms.ReturnValue;
};

// Function GameFramework.GameAICommand.InternalTick
// [0x00020803] ( FUNC_Final | FUNC_Event )
// Parameters infos:
// float                          DeltaTime                      ( CPF_Parm )

void UGameAICommand::eventInternalTick ( float DeltaTime )
{
	static UFunction* pFnInternalTick = NULL;

	if ( ! pFnInternalTick )
		pFnInternalTick = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalTick" );

	UGameAICommand_eventInternalTick_Parms InternalTick_Parms;
	InternalTick_Parms.DeltaTime = DeltaTime;

	this->ProcessEvent ( pFnInternalTick, &InternalTick_Parms, NULL );
};

// Function GameFramework.GameAICommand.InternalResumed
// [0x00020803] ( FUNC_Final | FUNC_Event )
// Parameters infos:
// struct FName                   OldCommandName                 ( CPF_Parm )

void UGameAICommand::eventInternalResumed ( struct FName OldCommandName )
{
	static UFunction* pFnInternalResumed = NULL;

	if ( ! pFnInternalResumed )
		pFnInternalResumed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalResumed" );

	UGameAICommand_eventInternalResumed_Parms InternalResumed_Parms;
	memcpy ( &InternalResumed_Parms.OldCommandName, &OldCommandName, 0x8 );

	this->ProcessEvent ( pFnInternalResumed, &InternalResumed_Parms, NULL );
};

// Function GameFramework.GameAICommand.InternalPaused
// [0x00020803] ( FUNC_Final | FUNC_Event )
// Parameters infos:
// class UGameAICommand*          NewCommand                     ( CPF_Parm )

void UGameAICommand::eventInternalPaused ( class UGameAICommand* NewCommand )
{
	static UFunction* pFnInternalPaused = NULL;

	if ( ! pFnInternalPaused )
		pFnInternalPaused = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalPaused" );

	UGameAICommand_eventInternalPaused_Parms InternalPaused_Parms;
	InternalPaused_Parms.NewCommand = NewCommand;

	this->ProcessEvent ( pFnInternalPaused, &InternalPaused_Parms, NULL );
};

// Function GameFramework.GameAICommand.InternalPopped
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void UGameAICommand::eventInternalPopped ( )
{
	static UFunction* pFnInternalPopped = NULL;

	if ( ! pFnInternalPopped )
		pFnInternalPopped = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalPopped" );

	UGameAICommand_eventInternalPopped_Parms InternalPopped_Parms;

	this->ProcessEvent ( pFnInternalPopped, &InternalPopped_Parms, NULL );
};

// Function GameFramework.GameAICommand.InternalPushed
// [0x00020803] ( FUNC_Final | FUNC_Event )
// Parameters infos:

void UGameAICommand::eventInternalPushed ( )
{
	static UFunction* pFnInternalPushed = NULL;

	if ( ! pFnInternalPushed )
		pFnInternalPushed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalPushed" );

	UGameAICommand_eventInternalPushed_Parms InternalPushed_Parms;

	this->ProcessEvent ( pFnInternalPushed, &InternalPushed_Parms, NULL );
};

// Function GameFramework.GameAICommand.InternalPrePushed
// [0x00020803] ( FUNC_Final | FUNC_Event )
// Parameters infos:
// class AGameAIController*       AI                             ( CPF_Parm )

void UGameAICommand::eventInternalPrePushed ( class AGameAIController* AI )
{
	static UFunction* pFnInternalPrePushed = NULL;

	if ( ! pFnInternalPrePushed )
		pFnInternalPrePushed = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InternalPrePushed" );

	UGameAICommand_eventInternalPrePushed_Parms InternalPrePushed_Parms;
	InternalPrePushed_Parms.AI = AI;

	this->ProcessEvent ( pFnInternalPrePushed, &InternalPrePushed_Parms, NULL );
};

// Function GameFramework.GameAICommand.InitCommand
// [0x00022002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class AGameAIController*       AI                             ( CPF_Parm )

bool UGameAICommand::InitCommand ( class AGameAIController* AI )
{
	static UFunction* pFnInitCommand = NULL;

	if ( ! pFnInitCommand )
		pFnInitCommand = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InitCommand" );

	UGameAICommand_execInitCommand_Parms InitCommand_Parms;
	InitCommand_Parms.AI = AI;

	this->ProcessEvent ( pFnInitCommand, &InitCommand_Parms, NULL );

	return InitCommand_Parms.ReturnValue;
};

// Function GameFramework.GameAICommand.InitCommandUserActor
// [0x00022002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class AGameAIController*       AI                             ( CPF_Parm )
// class AActor*                  UserActor                      ( CPF_Parm )

bool UGameAICommand::InitCommandUserActor ( class AGameAIController* AI, class AActor* UserActor )
{
	static UFunction* pFnInitCommandUserActor = NULL;

	if ( ! pFnInitCommandUserActor )
		pFnInitCommandUserActor = (UFunction*) ObjectCache::Find( "Function GameFramework.GameAICommand.InitCommandUserActor" );

	UGameAICommand_execInitCommandUserActor_Parms InitCommandUserActor_Parms;
	InitCommandUserActor_Parms.AI = AI;
	InitCommandUserActor_Parms.UserActor = UserActor;

	this->ProcessEvent ( pFnInitCommandUserActor, &InitCommandUserActor_Parms, NULL );

	return InitCommandUserActor_Parms.ReturnValue;
};

// Function GameFramework.GameExplosionActor.Tick
// [0x00020102] 
// Parameters infos:
// float                          DeltaTime                      ( CPF_Parm )

void AGameExplosionActor::Tick ( float DeltaTime )
{
	static UFunction* pFnTick = NULL;

	if ( ! pFnTick )
		pFnTick = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.Tick" );

	AGameExplosionActor_execTick_Parms Tick_Parms;
	Tick_Parms.DeltaTime = DeltaTime;

	this->ProcessEvent ( pFnTick, &Tick_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.DoExplosionDamage
// [0x00020102] 
// Parameters infos:

void AGameExplosionActor::DoExplosionDamage ( )
{
	static UFunction* pFnDoExplosionDamage = NULL;

	if ( ! pFnDoExplosionDamage )
		pFnDoExplosionDamage = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.DoExplosionDamage" );

	AGameExplosionActor_execDoExplosionDamage_Parms DoExplosionDamage_Parms;

	this->ProcessEvent ( pFnDoExplosionDamage, &DoExplosionDamage_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.Explode
// [0x00020902] ( FUNC_Event )
// Parameters infos:
// class UGameExplosion*          NewExplosionTemplate           ( CPF_Parm )

void AGameExplosionActor::eventExplode ( class UGameExplosion* NewExplosionTemplate )
{
	static UFunction* pFnExplode = NULL;

	if ( ! pFnExplode )
		pFnExplode = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.Explode" );

	AGameExplosionActor_eventExplode_Parms Explode_Parms;
	Explode_Parms.NewExplosionTemplate = NewExplosionTemplate;

	this->ProcessEvent ( pFnExplode, &Explode_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.SpawnExplosionFogVolume
// [0x00020100] 
// Parameters infos:

void AGameExplosionActor::SpawnExplosionFogVolume ( )
{
	static UFunction* pFnSpawnExplosionFogVolume = NULL;

	if ( ! pFnSpawnExplosionFogVolume )
		pFnSpawnExplosionFogVolume = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.SpawnExplosionFogVolume" );

	AGameExplosionActor_execSpawnExplosionFogVolume_Parms SpawnExplosionFogVolume_Parms;

	this->ProcessEvent ( pFnSpawnExplosionFogVolume, &SpawnExplosionFogVolume_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.DoExplosionCameraShakes
// [0x00020100] 
// Parameters infos:

void AGameExplosionActor::DoExplosionCameraShakes ( )
{
	static UFunction* pFnDoExplosionCameraShakes = NULL;

	if ( ! pFnDoExplosionCameraShakes )
		pFnDoExplosionCameraShakes = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.DoExplosionCameraShakes" );

	AGameExplosionActor_execDoExplosionCameraShakes_Parms DoExplosionCameraShakes_Parms;

	this->ProcessEvent ( pFnDoExplosionCameraShakes, &DoExplosionCameraShakes_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.SpawnExplosionDecal
// [0x00020100] 
// Parameters infos:

void AGameExplosionActor::SpawnExplosionDecal ( )
{
	static UFunction* pFnSpawnExplosionDecal = NULL;

	if ( ! pFnSpawnExplosionDecal )
		pFnSpawnExplosionDecal = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.SpawnExplosionDecal" );

	AGameExplosionActor_execSpawnExplosionDecal_Parms SpawnExplosionDecal_Parms;

	this->ProcessEvent ( pFnSpawnExplosionDecal, &SpawnExplosionDecal_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.SpawnExplosionParticleSystem
// [0x00020100] 
// Parameters infos:
// class UParticleSystem*         Template                       ( CPF_Parm )

void AGameExplosionActor::SpawnExplosionParticleSystem ( class UParticleSystem* Template )
{
	static UFunction* pFnSpawnExplosionParticleSystem = NULL;

	if ( ! pFnSpawnExplosionParticleSystem )
		pFnSpawnExplosionParticleSystem = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.SpawnExplosionParticleSystem" );

	AGameExplosionActor_execSpawnExplosionParticleSystem_Parms SpawnExplosionParticleSystem_Parms;
	SpawnExplosionParticleSystem_Parms.Template = Template;

	this->ProcessEvent ( pFnSpawnExplosionParticleSystem, &SpawnExplosionParticleSystem_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.UpdateExplosionTemplateWithPerMaterialFX
// [0x00080100] 
// Parameters infos:
// class UPhysicalMaterial*       PhysMaterial                   ( CPF_Parm )

void AGameExplosionActor::UpdateExplosionTemplateWithPerMaterialFX ( class UPhysicalMaterial* PhysMaterial )
{
	static UFunction* pFnUpdateExplosionTemplateWithPerMaterialFX = NULL;

	if ( ! pFnUpdateExplosionTemplateWithPerMaterialFX )
		pFnUpdateExplosionTemplateWithPerMaterialFX = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.UpdateExplosionTemplateWithPerMaterialFX" );

	AGameExplosionActor_execUpdateExplosionTemplateWithPerMaterialFX_Parms UpdateExplosionTemplateWithPerMaterialFX_Parms;
	UpdateExplosionTemplateWithPerMaterialFX_Parms.PhysMaterial = PhysMaterial;

	this->ProcessEvent ( pFnUpdateExplosionTemplateWithPerMaterialFX, &UpdateExplosionTemplateWithPerMaterialFX_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.KnockdownPawn
// [0x00080002] 
// Parameters infos:
// class AGamePawn*               Victim                         ( CPF_Parm )
// float                          DistFromExplosion              ( CPF_Parm )

void AGameExplosionActor::KnockdownPawn ( class AGamePawn* Victim, float DistFromExplosion )
{
	static UFunction* pFnKnockdownPawn = NULL;

	if ( ! pFnKnockdownPawn )
		pFnKnockdownPawn = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.KnockdownPawn" );

	AGameExplosionActor_execKnockdownPawn_Parms KnockdownPawn_Parms;
	KnockdownPawn_Parms.Victim = Victim;
	KnockdownPawn_Parms.DistFromExplosion = DistFromExplosion;

	this->ProcessEvent ( pFnKnockdownPawn, &KnockdownPawn_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.DoCringesAndKnockdowns
// [0x00880002] 
// Parameters infos:

void AGameExplosionActor::DoCringesAndKnockdowns ( )
{
	static UFunction* pFnDoCringesAndKnockdowns = NULL;

	if ( ! pFnDoCringesAndKnockdowns )
		pFnDoCringesAndKnockdowns = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.DoCringesAndKnockdowns" );

	AGameExplosionActor_execDoCringesAndKnockdowns_Parms DoCringesAndKnockdowns_Parms;

	this->ProcessEvent ( pFnDoCringesAndKnockdowns, &DoCringesAndKnockdowns_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.ShouldDoCringeFor
// [0x00080002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class AGamePawn*               Victim                         ( CPF_Parm )

bool AGameExplosionActor::ShouldDoCringeFor ( class AGamePawn* Victim )
{
	static UFunction* pFnShouldDoCringeFor = NULL;

	if ( ! pFnShouldDoCringeFor )
		pFnShouldDoCringeFor = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.ShouldDoCringeFor" );

	AGameExplosionActor_execShouldDoCringeFor_Parms ShouldDoCringeFor_Parms;
	ShouldDoCringeFor_Parms.Victim = Victim;

	this->ProcessEvent ( pFnShouldDoCringeFor, &ShouldDoCringeFor_Parms, NULL );

	return ShouldDoCringeFor_Parms.ReturnValue;
};

// Function GameFramework.GameExplosionActor.DoBreakFracturedMeshes
// [0x00080102] 
// Parameters infos:
// struct FVector                 ExploOrigin                    ( CPF_Parm )
// float                          DamageRadius                   ( CPF_Parm )
// float                          RBStrength                     ( CPF_Parm )
// class UClass*                  dmgType                        ( CPF_Parm )

void AGameExplosionActor::DoBreakFracturedMeshes ( struct FVector ExploOrigin, float DamageRadius, float RBStrength, class UClass* dmgType )
{
	static UFunction* pFnDoBreakFracturedMeshes = NULL;

	if ( ! pFnDoBreakFracturedMeshes )
		pFnDoBreakFracturedMeshes = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.DoBreakFracturedMeshes" );

	AGameExplosionActor_execDoBreakFracturedMeshes_Parms DoBreakFracturedMeshes_Parms;
	memcpy ( &DoBreakFracturedMeshes_Parms.ExploOrigin, &ExploOrigin, 0xC );
	DoBreakFracturedMeshes_Parms.DamageRadius = DamageRadius;
	DoBreakFracturedMeshes_Parms.RBStrength = RBStrength;
	DoBreakFracturedMeshes_Parms.dmgType = dmgType;

	this->ProcessEvent ( pFnDoBreakFracturedMeshes, &DoBreakFracturedMeshes_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.HurtExplosion
// [0x00880103] ( FUNC_Final )
// Parameters infos:
// float                          BaseDamage                     ( CPF_Parm )
// float                          DamageRadius                   ( CPF_Parm )
// float                          DamageFalloffExp               ( CPF_Parm )
// class UClass*                  DamageType                     ( CPF_Parm )
// float                          MomentumScale                  ( CPF_Parm )
// struct FVector                 ExploOrigin                    ( CPF_Parm )
// class AActor*                  IgnoredActor                   ( CPF_Parm )
// class UClass*                  ActorClassToIgnoreForDamage    ( CPF_Parm )
// class AController*             InstigatedByController         ( CPF_Parm )
// unsigned long                  bDoFullDamage                  ( CPF_Parm )

void AGameExplosionActor::HurtExplosion ( float BaseDamage, float DamageRadius, float DamageFalloffExp, class UClass* DamageType, float MomentumScale, struct FVector ExploOrigin, class AActor* IgnoredActor, class UClass* ActorClassToIgnoreForDamage, class AController* InstigatedByController, unsigned long bDoFullDamage )
{
	static UFunction* pFnHurtExplosion = NULL;

	if ( ! pFnHurtExplosion )
		pFnHurtExplosion = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.HurtExplosion" );

	AGameExplosionActor_execHurtExplosion_Parms HurtExplosion_Parms;
	HurtExplosion_Parms.BaseDamage = BaseDamage;
	HurtExplosion_Parms.DamageRadius = DamageRadius;
	HurtExplosion_Parms.DamageFalloffExp = DamageFalloffExp;
	HurtExplosion_Parms.DamageType = DamageType;
	HurtExplosion_Parms.MomentumScale = MomentumScale;
	memcpy ( &HurtExplosion_Parms.ExploOrigin, &ExploOrigin, 0xC );
	HurtExplosion_Parms.IgnoredActor = IgnoredActor;
	HurtExplosion_Parms.ActorClassToIgnoreForDamage = ActorClassToIgnoreForDamage;
	HurtExplosion_Parms.InstigatedByController = InstigatedByController;
	HurtExplosion_Parms.bDoFullDamage = bDoFullDamage;

	this->ProcessEvent ( pFnHurtExplosion, &HurtExplosion_Parms, NULL );
};

// Function GameFramework.GameExplosionActor.DoFullDamageToActor
// [0x00020102] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// class AActor*                  Victim                         ( CPF_Parm )

bool AGameExplosionActor::DoFullDamageToActor ( class AActor* Victim )
{
	static UFunction* pFnDoFullDamageToActor = NULL;

	if ( ! pFnDoFullDamageToActor )
		pFnDoFullDamageToActor = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.DoFullDamageToActor" );

	AGameExplosionActor_execDoFullDamageToActor_Parms DoFullDamageToActor_Parms;
	DoFullDamageToActor_Parms.Victim = Victim;

	this->ProcessEvent ( pFnDoFullDamageToActor, &DoFullDamageToActor_Parms, NULL );

	return DoFullDamageToActor_Parms.ReturnValue;
};

// Function GameFramework.GameExplosionActor.GetPhysicalMaterial
// [0x00880102] 
// Parameters infos:
// class UPhysicalMaterial*       ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

class UPhysicalMaterial* AGameExplosionActor::GetPhysicalMaterial ( )
{
	static UFunction* pFnGetPhysicalMaterial = NULL;

	if ( ! pFnGetPhysicalMaterial )
		pFnGetPhysicalMaterial = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.GetPhysicalMaterial" );

	AGameExplosionActor_execGetPhysicalMaterial_Parms GetPhysicalMaterial_Parms;

	this->ProcessEvent ( pFnGetPhysicalMaterial, &GetPhysicalMaterial_Parms, NULL );

	return GetPhysicalMaterial_Parms.ReturnValue;
};

// Function GameFramework.GameExplosionActor.PreBeginPlay
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void AGameExplosionActor::eventPreBeginPlay ( )
{
	static UFunction* pFnPreBeginPlay = NULL;

	if ( ! pFnPreBeginPlay )
		pFnPreBeginPlay = (UFunction*) ObjectCache::Find( "Function GameFramework.GameExplosionActor.PreBeginPlay" );

	AGameExplosionActor_eventPreBeginPlay_Parms PreBeginPlay_Parms;

	this->ProcessEvent ( pFnPreBeginPlay, &PreBeginPlay_Parms, NULL );
};

// Function GameFramework.GameHUD.StopDrawingChapterTitle
// [0x00020102] 
// Parameters infos:

void AGameHUD::StopDrawingChapterTitle ( )
{
	static UFunction* pFnStopDrawingChapterTitle = NULL;

	if ( ! pFnStopDrawingChapterTitle )
		pFnStopDrawingChapterTitle = (UFunction*) ObjectCache::Find( "Function GameFramework.GameHUD.StopDrawingChapterTitle" );

	AGameHUD_execStopDrawingChapterTitle_Parms StopDrawingChapterTitle_Parms;

	this->ProcessEvent ( pFnStopDrawingChapterTitle, &StopDrawingChapterTitle_Parms, NULL );
};

// Function GameFramework.GameHUD.StartDrawingChapterTitle
// [0x00020102] 
// Parameters infos:
// struct FString                 ChapterName                    ( CPF_Parm | CPF_NeedCtorLink )
// struct FString                 ActName                        ( CPF_Parm | CPF_NeedCtorLink )
// float                          TotalDrawTime                  ( CPF_Parm )
// float                          TotalFadeTime                  ( CPF_Parm )

void AGameHUD::StartDrawingChapterTitle ( struct FString ChapterName, struct FString ActName, float TotalDrawTime, float TotalFadeTime )
{
	static UFunction* pFnStartDrawingChapterTitle = NULL;

	if ( ! pFnStartDrawingChapterTitle )
		pFnStartDrawingChapterTitle = (UFunction*) ObjectCache::Find( "Function GameFramework.GameHUD.StartDrawingChapterTitle" );

	AGameHUD_execStartDrawingChapterTitle_Parms StartDrawingChapterTitle_Parms;
	memcpy ( &StartDrawingChapterTitle_Parms.ChapterName, &ChapterName, 0xC );
	memcpy ( &StartDrawingChapterTitle_Parms.ActName, &ActName, 0xC );
	StartDrawingChapterTitle_Parms.TotalDrawTime = TotalDrawTime;
	StartDrawingChapterTitle_Parms.TotalFadeTime = TotalFadeTime;

	this->ProcessEvent ( pFnStartDrawingChapterTitle, &StartDrawingChapterTitle_Parms, NULL );
};

// Function GameFramework.GameHUD.SetHUDDrawColor
// [0x00024002] 
// Parameters infos:
// unsigned char                  eColor                         ( CPF_Parm )
// unsigned char                  Alpha                          ( CPF_OptionalParm | CPF_Parm )

void AGameHUD::SetHUDDrawColor ( unsigned char eColor, unsigned char Alpha )
{
	static UFunction* pFnSetHUDDrawColor = NULL;

	if ( ! pFnSetHUDDrawColor )
		pFnSetHUDDrawColor = (UFunction*) ObjectCache::Find( "Function GameFramework.GameHUD.SetHUDDrawColor" );

	AGameHUD_execSetHUDDrawColor_Parms SetHUDDrawColor_Parms;
	SetHUDDrawColor_Parms.eColor = eColor;
	SetHUDDrawColor_Parms.Alpha = Alpha;

	this->ProcessEvent ( pFnSetHUDDrawColor, &SetHUDDrawColor_Parms, NULL );
};

// Function GameFramework.GamePawn.Cringe
// [0x00024800] ( FUNC_Event )
// Parameters infos:
// float                          Duration                       ( CPF_OptionalParm | CPF_Parm )

void AGamePawn::eventCringe ( float Duration )
{
	static UFunction* pFnCringe = NULL;

	if ( ! pFnCringe )
		pFnCringe = (UFunction*) ObjectCache::Find( "Function GameFramework.GamePawn.Cringe" );

	AGamePawn_eventCringe_Parms Cringe_Parms;
	Cringe_Parms.Duration = Duration;

	this->ProcessEvent ( pFnCringe, &Cringe_Parms, NULL );
};

// Function GameFramework.GamePawn.ServerKnockdown
// [0x002240C0] 
// Parameters infos:
// struct FVector                 RBLinearVelocity               ( CPF_OptionalParm | CPF_Parm )
// struct FVector                 RBAngularVelocity              ( CPF_OptionalParm | CPF_Parm )
// struct FVector                 RadialOrigin                   ( CPF_OptionalParm | CPF_Parm )
// float                          RadialRadius                   ( CPF_OptionalParm | CPF_Parm )
// float                          RadialStrength                 ( CPF_OptionalParm | CPF_Parm )
// struct FVector                 PointImpulse                   ( CPF_OptionalParm | CPF_Parm )
// struct FVector                 PointImpulsePosition           ( CPF_OptionalParm | CPF_Parm )
// struct FName                   PointImpulseBoneName           ( CPF_OptionalParm | CPF_Parm )

void AGamePawn::ServerKnockdown ( struct FVector RBLinearVelocity, struct FVector RBAngularVelocity, struct FVector RadialOrigin, float RadialRadius, float RadialStrength, struct FVector PointImpulse, struct FVector PointImpulsePosition, struct FName PointImpulseBoneName )
{
	static UFunction* pFnServerKnockdown = NULL;

	if ( ! pFnServerKnockdown )
		pFnServerKnockdown = (UFunction*) ObjectCache::Find( "Function GameFramework.GamePawn.ServerKnockdown" );

	AGamePawn_execServerKnockdown_Parms ServerKnockdown_Parms;
	memcpy ( &ServerKnockdown_Parms.RBLinearVelocity, &RBLinearVelocity, 0xC );
	memcpy ( &ServerKnockdown_Parms.RBAngularVelocity, &RBAngularVelocity, 0xC );
	memcpy ( &ServerKnockdown_Parms.RadialOrigin, &RadialOrigin, 0xC );
	ServerKnockdown_Parms.RadialRadius = RadialRadius;
	ServerKnockdown_Parms.RadialStrength = RadialStrength;
	memcpy ( &ServerKnockdown_Parms.PointImpulse, &PointImpulse, 0xC );
	memcpy ( &ServerKnockdown_Parms.PointImpulsePosition, &PointImpulsePosition, 0xC );
	memcpy ( &ServerKnockdown_Parms.PointImpulseBoneName, &PointImpulseBoneName, 0x8 );

	this->ProcessEvent ( pFnServerKnockdown, &ServerKnockdown_Parms, NULL );
};

// Function GameFramework.GamePawn.GetTargetFrictionCylinder
// [0x00420102] 
// Parameters infos:
// float                          CylinderRadius                 ( CPF_Parm | CPF_OutParm )
// float                          CylinderHeight                 ( CPF_Parm | CPF_OutParm )

void AGamePawn::GetTargetFrictionCylinder ( float* CylinderRadius, float* CylinderHeight )
{
	static UFunction* pFnGetTargetFrictionCylinder = NULL;

	if ( ! pFnGetTargetFrictionCylinder )
		pFnGetTargetFrictionCylinder = (UFunction*) ObjectCache::Find( "Function GameFramework.GamePawn.GetTargetFrictionCylinder" );

	AGamePawn_execGetTargetFrictionCylinder_Parms GetTargetFrictionCylinder_Parms;

	this->ProcessEvent ( pFnGetTargetFrictionCylinder, &GetTargetFrictionCylinder_Parms, NULL );

	if ( CylinderRadius )
		*CylinderRadius = GetTargetFrictionCylinder_Parms.CylinderRadius;

	if ( CylinderHeight )
		*CylinderHeight = GetTargetFrictionCylinder_Parms.CylinderHeight;
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
