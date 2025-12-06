#include "src/pch.hpp"

#include "src/Utils/Logger/Logger.hpp"

unsigned long ModuleThread( void* ) {
	// Logger::EnabledChannels.push_back("hook_calltree");
	Logger::EnabledChannels.push_back("test");

	Logger::Log("test", "It works!");

	// ::DetourTransactionBegin();
	// ::DetourUpdateThread(::GetCurrentThread());
	//
	//
	// ::DetourTransactionCommit();

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ModuleThread), 0, 0, 0 );
			// CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(DebugWindow::ModuleThread), 0, 0, 0 );
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

