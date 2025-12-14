#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {

	if (Object && Function) {
		std::string name = std::string(Function->GetFullName());
		std::string objname = std::string(Object->GetFullName());

		if (strcmp("Function Engine.Actor.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.GameInfoDataProvider.ProviderInstanceBound", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPlayerController.GetPlayerViewPoint", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn.ShouldRechargePowerPool", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn_Character.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeployable.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.SendClientAdjustment", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective_Proximity.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.ServerMove", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.OldServerMove", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective.IsLocalPlayerAttacker", name.c_str()) == 0
		|| strcmp("Function TgGame.TgProperty.Copy", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeploy_BeaconEntrance.Touch", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective_Bot.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.PreBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.SetInitialState", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.Emitter.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.LadderVolume.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.KAsset.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function TgGame.TgRepInfo_Player.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.GameRunning.Timer", name.c_str()) == 0
		|| strcmp("Function Engine.GameReplicationInfo.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn.GetCameraValues", name.c_str()) == 0
		|| strcmp("Function TgGame_Defense.RoundInProgress.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.Touch", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeploy_BeaconEntrance.RecheckActiveTimer", name.c_str()) == 0
		|| strcmp("Function TgGame_Arena.RoundInProgress.Tick", name.c_str()) == 0
		|| strcmp("Function TgPawn.Dying.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.GameInfo.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgRepInfo_Game.ServerUpdateTimer", name.c_str()) == 0
		|| strcmp("Function Engine.SequenceOp.Activated", name.c_str()) == 0) {
			CallOriginal(Object, edx, Function, Params, Result);
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceStartFiringNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceStartFiringNow_Parms* CanDeviceStartFiringNowParams = (ATgDevice_eventCanDeviceStartFiringNow_Parms*)Params;
		// 	CanDeviceStartFiringNowParams->ReturnValue = true;
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceFireNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceFireNow_Parms* CanDeviceFireNowParams = (ATgDevice_eventCanDeviceFireNow_Parms*)Params;
		// 	CanDeviceFireNowParams->ReturnValue = true;
		} else {
			Logger::Log(GetLogChannel(), "├─ %s [%s]\n", name.c_str(), objname.c_str());
			Logger::ChannelIndents[GetLogChannel()]++;

			CallOriginal(Object, edx, Function, Params, Result);

			Logger::ChannelIndents[GetLogChannel()]--;
			// Logger::Log(GetLogChannel(), "}\n");
		}
	}
}

