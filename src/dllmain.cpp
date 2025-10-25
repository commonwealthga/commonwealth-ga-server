#include "src/pch.hpp"

#include "src/Database/Database.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"
#include "src/GameServer/Engine/World/BeginPlay/World__BeginPlay.hpp"
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
#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.hpp"
#include "src/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/GameServer/TgGame/TgPawn/SwapAttachedDeviceMaterials/TgPawn__SwapAttachedDeviceMaterials.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/GameServer/Engine/Actor/Spawn/Actor__Spawn.hpp"
#include "src/GameServer/Engine/Actor/Tick/Actor__Tick.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.hpp"
#include "src/GameServer/TgGame/TgDevice/HasMinimumPowerPool/TgDevice__HasMinimumPowerPool.hpp"
#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotSpawnTableMarshal/CAmBot__LoadBotSpawnTableMarshal.hpp"
#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceMarshal/CAmDeviceModel__LoadDeviceMarshal.hpp"
#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceModeMarshal/CAmDeviceModel__LoadDeviceModeMarshal.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"


unsigned long ModuleThread( void* ) {
	Database::Init();

	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());

	// low-level engine functions
	GameEngine__Init::bInitTcpServer = true;
	GameEngine__Init::Install();
	// UObject__CollectGarbage::bDisableGarbageCollection = true;
	UObject__CollectGarbage::Install();
	// UObject__ProcessEvent::Install();
	World__BeginPlay::Install();
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
	// Actor__Tick::Install();

	// game functions
	TgPlayerController__IsReadyForStart::Install();
	TgPlayerController__SetSoundMode::Install();
	TgPlayerController__CanPlayerUseVolume::Install();
	// TgGame__TgFindPlayerStart::Install();
	TgGame__SpawnPlayerCharacter::Install();
	TgGame__SpawnBotPawn::Install();
	TgGame__LoadGameConfig::Install();
	TgGame__InitGameRepInfo::Install();
	TgGame_Arena__LoadGameConfig::Install();
	TgPawn__InitializeDefaultProps::Install();
	TgPawn__GetProperty::Install();
	// TgPawn__SwapAttachedDeviceMaterials::Install();
	TgTeamBeaconManager__SpawnNewBeaconForTeam::Install();
	TgBeaconFactory__SpawnObject::Install();
	TgInventoryManager__NonPersistAddDevice::Install();
	TgBotFactory__LoadObjectConfig::Install();
	// TgBotFactory__SpawnBot::Install();
	TgBotFactory__SpawnNextBot::Install();
	TgBotFactory__SpawnWave::Install();
	TgBotFactory__ResetQueue::Install();
	TgGame__SpawnBot::Install();
	TgGame__SpawnBotById::Install();
	TgGame__RegisterForWaveRevive::Install();
	TgGame__GetReviveTimeRemaining::Install();
	TgGame__ReviveAttackersTimer::Install();
	TgGame__ReviveDefendersTimer::Install();
	TgGame__MissionTimeRemaining::Install();
	TgGame__SendMissionTimerEvent::Install();
	// TgDevice__HasMinimumPowerPool::Install();
	// TgDevice__HasEnoughPowerPool::Install();
	TgMissionObjective_Bot__SpawnObjectiveBot::Install();

	// data collection
	CMarshal__GetByte::Install();
	CMarshal__GetInt32t::Install();
	CMarshal__GetString2::Install();
	CMarshal__GetFloat::Install();
	CMarshal__Translate::Install();
	CAmBot__LoadBotMarshal::bPopulateDatabaseBots = false;
	CAmBot__LoadBotMarshal::bPopulateDatabaseBotDevices = false;
	CAmBot__LoadBotMarshal::Install();
	CAmBot__LoadBotBehaviorMarshal::Install();
	CAmBot__LoadBotSpawnTableMarshal::bPopulateDatabase = false;
	CAmBot__LoadBotSpawnTableMarshal::Install();
	CAmDeviceModel__LoadDeviceMarshal::bPopulateDatabaseDevices = false;
	CAmDeviceModel__LoadDeviceMarshal::Install();
	CAmDeviceModel__LoadDeviceModeMarshal::bPopulateDatabaseDeviceModes = false;
	CAmDeviceModel__LoadDeviceModeMarshal::Install();
	CAmItem__LoadItemMarshal::bPopulateDatabaseItems = false;
	CAmItem__LoadItemMarshal::Install();
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

