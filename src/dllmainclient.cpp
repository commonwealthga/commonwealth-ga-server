#include "src/pch.hpp"

#include "src/Utils/DebugWindow/DebugWindow.hpp"
#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"

unsigned long ModuleThread( void* ) {
	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());
	Logger::EnabledChannels.push_back("stealth");
	Logger::EnabledChannels.push_back("hook_calltree");
	Logger::EnabledChannels.push_back("replicated_event");

	UObject__ProcessEvent::Install();

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
