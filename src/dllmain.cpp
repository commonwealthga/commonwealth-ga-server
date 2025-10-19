#include "src/pch.hpp"

#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"
#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.hpp"
#include "src/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.hpp"
#include "src/GameServer/TgGame/TgPlayerController/SetSoundMode/TgPlayerController__SetSoundMode.hpp"
#include "src/GameServer/TgGame/TgPlayerController/CanPlayerUseVolume/TgPlayerController__CanPlayerUseVolume.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/GameServer/Engine/Actor/Spawn/Actor__Spawn.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"


unsigned long ModuleThread( void* ) {
	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());

	// low-level engine functions
	GameEngine__Init::bInitTcpServer = true;
	GameEngine__Init::Install();
	// UObject__CollectGarbage::bDisableGarbageCollection = true;
	UObject__CollectGarbage::Install();
	ConstructCommandletObject::Install();
	ServerCommandlet__Main::Install();
	GameEngine__SpawnServerActors::Install();
	UdpNetDriver__InitListen::Install();
	UdpNetDriver__TickDispatch::Install();
	NetConnection__LowLevelSend::Install();
	MarshalChannel__NotifyControlMessage::Install();
	ActorChannel__ReceivedBunch__CanExecute::Install();
	Actor__GetOptimizedRepList::Install();
	Actor__Spawn::Install();

	// game functions
	TgPlayerController__IsReadyForStart::Install();
	TgPlayerController__SetSoundMode::Install();
	TgPlayerController__CanPlayerUseVolume::Install();
	TgGame__SpawnPlayerCharacter::Install();
	TgGame__SpawnBotPawn::Install();
	TgGame__LoadGameConfig::Install();
	TgGame__InitGameRepInfo::Install();
	TgGame_Arena__LoadGameConfig::Install();
	TgPawn__InitializeDefaultProps::Install();
	TgPawn__GetProperty::Install();
	TgTeamBeaconManager__SpawnNewBeaconForTeam::Install();
	TgBeaconFactory__SpawnObject::Install();
	TgInventoryManager__NonPersistAddDevice::Install();
	TgBotFactory__SpawnBot::Install();
	TgGame__SpawnBot::Install();
	TgGame__SpawnBotById::Install();
	TgGame__RegisterForWaveRevive::Install();
	TgGame__GetReviveTimeRemaining::Install();
	TgGame__ReviveAttackersTimer::Install();
	CMarshal__GetByte::Install();
	CMarshal__GetInt32t::Install();
	CAmBot__LoadBotMarshal::Install();
	CAmBot__LoadBotBehaviorMarshal::Install();
	CAmOmegaVolume__LoadOmegaVolumeMarshal::Install();

	::DetourTransactionCommit();

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ModuleThread), 0, 0, 0 );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

