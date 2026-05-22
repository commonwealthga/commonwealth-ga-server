#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/LoadAssemblyDat/AssemblyDatManager__LoadAssemblyDat.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/LoadStartupPackages/LoadStartupPackages.hpp"
#include "src/GameServer/Utils/EngineLoad/EngineLoad.hpp"
#include "src/Utils/Logger/Logger.hpp"

void GameEngine__Init::FixGlobals() {
	*Globals::Get().GIsClient = 0;
	*Globals::Get().GIsServer = 1;
	*Globals::Get().GIsEditor = 0;
	*Globals::Get().GIsUCC = 0;
	*Globals::Get().GUseNewOptimizedServerTick = 0;
	*Globals::Get().GIsNonCombat = 0;
	// *Globals::Get().GUseSeekFreeLoading = 1;
}

void GameEngine__Init::Call(void* GameEngine) {
	LogCallBegin();

	FixGlobals();

	Logger::Log("debug", "FixGlobals called\n");

	AssemblyDatManager__LoadAssemblyDat::CallOriginal(Globals::Get().GAssemblyDatManager);

	Logger::Log("debug", "LoadAssemblyDat called\n");

	// *Globals::Get().UObjectGObjFirstGCIndex = 0;

	LoadStartupPackages::CallOriginal();

	Logger::Log("debug", "LoadStartupPackages called\n");

	// Force-load script classes the server needs but never references on its own.
	// UE3 creates package exports lazily; a script-only class that nothing on the
	// server touches is never instantiated (absent from GObjObjects) even though
	// its export is present and RF_LoadForServer-flagged. TgEffectBuff is the
	// canonical buff-routing effect class — load + root it here so it's resident
	// before any effect is constructed.
	EngineLoad::PreloadClass("TgGame.TgEffectBuff");

	GameEngine__Init::CallOriginal(GameEngine);

	LogCallEnd();
}

