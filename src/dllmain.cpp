#include "src/pch.hpp"

#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"
#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/ActorChannel/ReceivedBunch/ActorChannel__ReceivedBunch__CanExecute.hpp"
#include "src/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"

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
	MarshalChannel__NotifyControlMessage::Install();
	ActorChannel__ReceivedBunch__CanExecute::Install();

	// game functions
	TgPlayerController__IsReadyForStart::Install();
	TgGame__SpawnPlayerCharacter::Install();
	TgGame__LoadGameConfig::Install();
	TgGame_Arena__LoadGameConfig::Install();

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

