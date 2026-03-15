#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Globals.hpp"

// TgPawn__SetProperty @ 0x109bf420 — __thiscall(pawn, nPropertyId, fNewValue)
// Called as __fastcall with dummy EDX to avoid inline asm.
static void SetPawnProperty(ATgPawn* Pawn, int nPropertyId, float fNewValue) {
	((void(__fastcall*)(ATgPawn*, void*, int, float))0x109bf420)(Pawn, nullptr, nPropertyId, fNewValue);
}

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
		|| strcmp("Function TgGame.TgPawn.IsCrewed", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDevice.IsOffhand", name.c_str()) == 0
		|| strcmp("Function TgGame.TgAIController.SeePlayer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgSkeletalMeshActorNPC.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgSkeletalMeshActor_CharacterBuilder.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgInterpolatingCameraActor.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.SequenceOp.Activated", name.c_str()) == 0) {
			CallOriginal(Object, edx, Function, Params, Result);
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceStartFiringNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceStartFiringNow_Parms* CanDeviceStartFiringNowParams = (ATgDevice_eventCanDeviceStartFiringNow_Parms*)Params;
		// 	CanDeviceStartFiringNowParams->ReturnValue = true;
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceFireNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceFireNow_Parms* CanDeviceFireNowParams = (ATgDevice_eventCanDeviceFireNow_Parms*)Params;
		// 	CanDeviceFireNowParams->ReturnValue = true;


		// } else if (strcmp("Function TgDevice.DeviceFiring.BeginState", name.c_str()) == 0) {
		// 	CallOriginal(Object, edx, Function, Params, Result);
		// 	ATgDevice* Device = (ATgDevice*)Object;
		// 	if (Device->IsOffhandJetpack() && Device->Instigator) {
		// 		ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
		// 		SetPawnProperty(Pawn, 59, 3.0f);  // TGPID_FLIGHT_ACCELERATION
		// 		Pawn->bNetDirty = 1;
		// 		Pawn->bForceNetUpdate = 1;
		// 	}
		} else if (strcmp("Function TgDevice.DeviceFiring.EndState", name.c_str()) == 0) {
			CallOriginal(Object, edx, Function, Params, Result);
			ATgDevice* Device = (ATgDevice*)Object;
			if (Device->IsOffhandJetpack() && Device->Instigator) {
				ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
				SetPawnProperty(Pawn, 59, 0.0f);  // clear TGPID_FLIGHT_ACCELERATION
				Pawn->bNetDirty = 1;
				Pawn->bForceNetUpdate = 1;
			}


		} else {
			Logger::Log(GetLogChannel(), "├─ %s [%s]\n", name.c_str(), objname.c_str());
			Logger::ChannelIndents[GetLogChannel()]++;

			CallOriginal(Object, edx, Function, Params, Result);

			Logger::ChannelIndents[GetLogChannel()]--;
			// Logger::Log(GetLogChannel(), "}\n");
		}
	}
}

