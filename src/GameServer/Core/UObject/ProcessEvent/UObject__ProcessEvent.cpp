#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {

	if (Object && Function) {
		if (strcmp("Function Engine.Actor.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.GameInfoDataProvider.ProviderInstanceBound", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgPlayerController.GetPlayerViewPoint", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgPawn.ShouldRechargePowerPool", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgPawn_Character.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgDeployable.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.PlayerController.SendClientAdjustment", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgMissionObjective_Proximity.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.PlayerController.ServerMove", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgMissionObjective.IsLocalPlayerAttacker", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgProperty.Copy", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgDeploy_BeaconEntrance.Touch", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgMissionObjective_Bot.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.Actor.PreBeginPlay", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.Actor.SetInitialState", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.Actor.PostBeginPlay", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.Emitter.PostBeginPlay", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.LadderVolume.PostBeginPlay", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.KAsset.PostBeginPlay", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgRepInfo_Player.Timer", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.GameRunning.Timer", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.GameReplicationInfo.Timer", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame.TgPawn.GetCameraValues", Function->GetFullName()) == 0) {
		} else if (strcmp("Function TgGame_Defense.RoundInProgress.Tick", Function->GetFullName()) == 0) {
		} else if (strcmp("Function Engine.SequenceOp.Activated", Function->GetFullName()) == 0) {
			Logger::Log("event", "%s", Function->GetFullName());
			Logger::Log("event", " %s\n", Object->GetFullName());
		} else {
			Logger::Log("event", "%s\n", Function->GetFullName());
		}


	}



	CallOriginal(Object, edx, Function, Params, Result);
}

