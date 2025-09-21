#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/LoadAssemblyDat/AssemblyDatManager__LoadAssemblyDat.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/LoadStartupPackages/LoadStartupPackages.hpp"

void GameEngine__Init::FixGlobals() {
	*Globals::Get().GIsClient = 0;
	*Globals::Get().GIsServer = 1;
	*Globals::Get().GIsEditor = 0;
	*Globals::Get().GIsUCC = 0;
	*Globals::Get().GUseNewOptimizedServerTick = 0;
}

void GameEngine__Init::Call(void* GameEngine) {
	FixGlobals();

	AssemblyDatManager__LoadAssemblyDat::CallOriginal(Globals::Get().GAssemblyDatManager);

	*Globals::Get().UObjectGObjFirstGCIndex = 0;

	LoadStartupPackages::CallOriginal();

	// todo: start custom TCP server

	GameEngine__Init::CallOriginal(GameEngine);
}

