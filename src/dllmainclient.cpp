#include "src/pch.hpp"

#include "src/Utils/DebugWindow/DebugWindow.hpp"
#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/TgDeployable/NotifyGroupChanged/TgDeployable__NotifyGroupChanged.hpp"
#include "src/GameServer/TgGame/TgRepInfo_Game/GetTaskForceFor/TgRepInfo_Game__GetTaskForceFor.hpp"
#include "src/GameServer/TgGame/TgDeployable/IsFriendlyWithLocalPawn/TgDeployable__IsFriendlyWithLocalPawn.hpp"

unsigned long ModuleThread( void* ) {
	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());
	Logger::EnabledChannels.push_back("stealth");
	Logger::EnabledChannels.push_back("hook_calltree");
	Logger::EnabledChannels.push_back("replicated_event");
	Logger::EnabledChannels.push_back("heal_tick");
	Logger::EnabledChannels.push_back("team_colors");

	UObject__ProcessEvent::Install();
	TgDeployable__NotifyGroupChanged::Install();
	// Client-side DIAGNOSTICS (not a real fix) — route deployable friendship
	// checks through the Instigator chain when the DRI path is unusable.
	// See each hook's hpp for why this isn't the solution; DRI replication
	// itself still needs to be repaired on the server side.
	TgRepInfo_Game__GetTaskForceFor::Install();        // DRI non-null but fields null
	TgDeployable__IsFriendlyWithLocalPawn::Install();  // r_DRI null entirely (medstation)

	::DetourTransactionCommit();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ModuleThread), 0, 0, 0 );

			DebugWindow::WindowTitle = "CLIENT";
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(DebugWindow::ModuleThread), 0, 0, 0 );
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
