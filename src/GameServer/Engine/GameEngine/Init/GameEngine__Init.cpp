#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/LoadAssemblyDat/AssemblyDatManager__LoadAssemblyDat.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/LoadStartupPackages/LoadStartupPackages.hpp"
#include "src/TcpServer/TcpServerInit/TcpServerInit.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool GameEngine__Init::bInitTcpServer = false;

void GameEngine__Init::FixGlobals() {
	*Globals::Get().GIsClient = 0;
	*Globals::Get().GIsServer = 1;
	*Globals::Get().GIsEditor = 0;
	*Globals::Get().GIsUCC = 0;
	*Globals::Get().GUseNewOptimizedServerTick = 0;
	// *Globals::Get().GUseSeekFreeLoading = 1;
}

void GameEngine__Init::Call(void* GameEngine) {
	Logger::Log("debug", "MINE GameEngine__Init START\n");

	FixGlobals();

	Logger::Log("debug", "FixGlobals called\n");

	AssemblyDatManager__LoadAssemblyDat::CallOriginal(Globals::Get().GAssemblyDatManager);

	Logger::Log("debug", "LoadAssemblyDat called\n");

	*Globals::Get().UObjectGObjFirstGCIndex = 0;

	LoadStartupPackages::CallOriginal();

	Logger::Log("debug", "LoadStartupPackages called\n");

	if (bInitTcpServer) {
		TcpServerInit::CreateTcpServerThread();
		Logger::Log("debug", "TcpServerInit::CreateTcpServerThread called\n");
	}

	GameEngine__Init::CallOriginal(GameEngine);

	Logger::Log("debug", "MINE GameEngine__Init END\n");
}

