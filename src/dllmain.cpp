#include "src/pch.hpp"

#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"
#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"

unsigned long ModuleThread( void* ) {
	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());

	// low-level engine functions
	GameEngine__Init::Install();
	ConstructCommandletObject::Install();
	ServerCommandlet__Main::Install();
	GameEngine__SpawnServerActors::Install();
	UdpNetDriver__InitListen::Install();
	UdpNetDriver__TickDispatch::Install();
	NetConnection__LowLevelSend::Install();



		// Real_UObject_StaticConstructObject         = atomic::memory::find<decltype(Real_UObject_StaticConstructObject)>(base, size, "6AFF681DC1611164A10000000050", 0, 0);
		// #if IS_SERVER
		//
		// 	if (pOriginalProcessEvent != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalProcessEvent, (PVOID)hkProcessEventServer);
		// 	}
		//
		// 	if (pOriginalLoadAssemblyDat != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalLoadAssemblyDat, (PVOID)Mine_LoadAssemblyDat);
		// 	}
		// 	if (pOriginalUGameEngineInit != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUGameEngineInit, (PVOID)Mine_UGameEngineInit);
		// 	}
		// 	if (pOriginalConstructCommandletObject != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalConstructCommandletObject, (PVOID)Mine_ConstructCommandletObject);
		// 	}
		// 	if (pOriginalServerCommandletMain != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalServerCommandletMain, (PVOID)Mine_ServerCommandletMain);
		// 	}
		// 	if (pOriginalGameEngineSpawnServerActors != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalGameEngineSpawnServerActors, (PVOID)Mine_GameEngineSpawnServerActors);
		// 	}
		//
		// 	if (pOriginalUWorld_SpawnActor != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUWorld_SpawnActor, (PVOID)Mine_UWorld_SpawnActor);
		// 	}
		//
		// 	if (pOriginalSocketInit != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalSocketInit, (PVOID)Mine_SocketInit);
		// 	}
		// 	if (pOriginalSocketBind != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalSocketBind, (PVOID)Mine_SocketBind);
		// 	}
		// 	if (pOriginalSocketClose != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalSocketClose, (PVOID)Mine_SocketClose);
		// 	}
		// 	if (pOriginalUdpNetDriver_InitListen != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUdpNetDriver_InitListen, (PVOID)Mine_UdpNetDriver_InitListen);
		// 	}
		//
		// 	if (pOriginalUdpNetDriverTickDispatch != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUdpNetDriverTickDispatch, (PVOID)Mine_UdpNetDriverTickDispatch);
		// 	}
		//
		// 	if (pOriginalConstructObjectInternal != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalConstructObjectInternal, (PVOID)Mine_ConstructObjectInternal);
		// 	}
		// 	if (pOriginalUNetConnectionLowLevelSend != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUNetConnectionLowLevelSend, (PVOID)Mine_UNetConnectionLowLevelSend);
		// 	}
		//
		// 	if (pOriginalTgMarshalChannel_NotifyControlMessage != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgMarshalChannel_NotifyControlMessage, (PVOID)Mine_TgMarshalChannel_NotifyControlMessage);
		// 	}
		//
		// 	if (pOriginalCanExecuteFunction != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalCanExecuteFunction, (PVOID)Mine_CanExecuteFunction);
		// 	}
		//
		// 	if (pOriginalActor_GetOptimizedRepList != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalActor_GetOptimizedRepList, (PVOID)Mine_Actor_GetOptimizedRepList);
		// 	}
		//
		// 	if (pOriginalLoadStartupPackages != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalLoadStartupPackages, (PVOID)Mine_LoadStartupPackages);
		// 	}
		//
		// 	if (pOriginalPlayerController_IsReadyForStart != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalPlayerController_IsReadyForStart, (PVOID)Mine_PlayerController_IsReadyForStart);
		// 	}
		//
		// 	if (pOriginalTgGame_SpawnPlayerCharacter != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgGame_SpawnPlayerCharacter, (PVOID)Mine_TgGame_SpawnPlayerCharacter);
		// 	}
		//
		// 	if (pOriginalTgPawn_InitializeDefaultProps != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgPawn_InitializeDefaultProps, (PVOID)Mine_TgPawn_InitializeDefaultProps);
		// 	}
		//
		// 	if (pOriginalUObject_CollectGarbage != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalUObject_CollectGarbage, (PVOID)Mine_UObject_CollectGarbage);
		// 	}
		//
		// 	if (pOriginalTgPawn_GetProperty != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgPawn_GetProperty, (PVOID)Mine_TgPawn_GetProperty);
		// 	}
		//
		// 	if (pOriginalTgTeamBeaconManager__SpawnNewBeaconForTeam != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgTeamBeaconManager__SpawnNewBeaconForTeam, (PVOID)Mine_TgTeamBeaconManager__SpawnNewBeaconForTeam);
		// 	}
		//
		// 	if (pOriginalTgBeaconFactory__SpawnObject != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgBeaconFactory__SpawnObject, (PVOID)Mine_TgBeaconFactory__SpawnObject);
		// 	}
		//
		// 	if (pOriginalTgInventoryManager__NonPersistAddDevice != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgInventoryManager__NonPersistAddDevice, (PVOID)Mine_TgInventoryManager__NonPersistAddDevice);
		// 	}
		//
		// 	if (pOriginalTgGame_InitGameRepInfo != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgGame_InitGameRepInfo, (PVOID)Mine_TgGame_InitGameRepInfo);
		// 	}
		// 	if (pOriginalTgGame_LoadGameConfig != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgGame_LoadGameConfig, (PVOID)Mine_TgGame_LoadGameConfig);
		// 	}
		// 	if (pOriginalTgGame_Arena_LoadGameConfig != nullptr) {
		// 		::DetourAttach(&(PVOID&)pOriginalTgGame_Arena_LoadGameConfig, (PVOID)Mine_TgGame_Arena_LoadGameConfig);
		// 	}
		//
		//
		//
		// #endif

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
