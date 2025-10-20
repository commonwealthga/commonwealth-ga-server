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
		} else {
			Logger::Log("event", "%s\n", Function->GetFullName());
		}

	}

	CallOriginal(Object, edx, Function, Params, Result);
}

