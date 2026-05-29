#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/LoadAssemblyDat/AssemblyDatManager__LoadAssemblyDat.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/LoadStartupPackages/LoadStartupPackages.hpp"
#include "src/GameServer/Utils/EngineLoad/EngineLoad.hpp"
#include "src/GameServer/Replication/ReplicationDefaults/ReplicationDefaults.hpp"
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
	//
	// Every entry below is a non-native UC class (no `native` keyword in
	// unrealscript/TgGame/Classes/*.uc) AND has a row in
	// asm_data_set_resources, meaning DB-driven name→UClass lookups
	// (ObjectCache::Find) will return nullptr if we don't load it here.
	EngineLoad::PreloadClass("TgGame.TgEffectBuff");

	EngineLoad::PreloadClass("TgGame.TgPawn_TurretFlak");
	EngineLoad::PreloadClass("TgGame.TgPawn_TurretFlame");
	EngineLoad::PreloadClass("TgGame.TgPawn_TurretPlasma");

	EngineLoad::PreloadClass("TgGame.TgDamageTypeAOE");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeBullet");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeExplosion");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeFalling");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeMeleeHeavy");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeMeleeLight");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeMinigun");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeOnFire");
	EngineLoad::PreloadClass("TgGame.TgDamageTypePoison");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeRanged");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeShotgun");
	EngineLoad::PreloadClass("TgGame.TgDamageTypeSnipe");

	EngineLoad::PreloadClass("TgGame.TgProj_Grenade");
	EngineLoad::PreloadClass("TgGame.TgProj_StraightTeleporter");
	EngineLoad::PreloadClass("TgGame.TgProj_Teleporter");

	EngineLoad::PreloadClass("TgGame.TgPawn_AVCompositeWalker");
	EngineLoad::PreloadClass("TgGame.TgPawn_AndroidMinion");
	EngineLoad::PreloadClass("TgGame.TgPawn_Boss_Destroyer");
	EngineLoad::PreloadClass("TgGame.TgPawn_Brawler");
	EngineLoad::PreloadClass("TgGame.TgPawn_Destructible");
	EngineLoad::PreloadClass("TgGame.TgPawn_Detonator");
	EngineLoad::PreloadClass("TgGame.TgPawn_DuneCommander");
	EngineLoad::PreloadClass("TgGame.TgPawn_Elite_Alchemist");
	EngineLoad::PreloadClass("TgGame.TgPawn_Elite_Assassin");
	EngineLoad::PreloadClass("TgGame.TgPawn_EscortRobot");
	EngineLoad::PreloadClass("TgGame.TgPawn_GroundPetA");
	EngineLoad::PreloadClass("TgGame.TgPawn_Guardian");
	EngineLoad::PreloadClass("TgGame.TgPawn_HoverShieldSphere");
	EngineLoad::PreloadClass("TgGame.TgPawn_Hunter");
	EngineLoad::PreloadClass("TgGame.TgPawn_Inquisitor");
	EngineLoad::PreloadClass("TgGame.TgPawn_Iris");
	EngineLoad::PreloadClass("TgGame.TgPawn_Raptor");
	EngineLoad::PreloadClass("TgGame.TgPawn_Reaper");
	EngineLoad::PreloadClass("TgGame.TgPawn_ScannerRecursive");
	EngineLoad::PreloadClass("TgGame.TgPawn_SiegeBarrage");
	EngineLoad::PreloadClass("TgGame.TgPawn_SiegeHover");
	EngineLoad::PreloadClass("TgGame.TgPawn_SiegeRapidFire");
	EngineLoad::PreloadClass("TgGame.TgPawn_SupportForeman");
	EngineLoad::PreloadClass("TgGame.TgPawn_Switchblade");
	EngineLoad::PreloadClass("TgGame.TgPawn_ThinkTank");
	EngineLoad::PreloadClass("TgGame.TgPawn_UberWalker");
	EngineLoad::PreloadClass("TgGame.TgPawn_Vanguard");
	EngineLoad::PreloadClass("TgGame.TgPawn_Vulcan");
	EngineLoad::PreloadClass("TgGame.TgPawn_Warlord");

	// Patch replication-related fields on CDOs of replicable actor classes.
	// CDOs now exist (LoadStartupPackages ran above) and no world has spawned
	// yet (CallOriginal below kicks off the engine init that ultimately
	// spawns GameInfo / GRI / Pawns), so values land in time for every spawn.
	ReplicationDefaults::Apply();

	GameEngine__Init::CallOriginal(GameEngine);

	LogCallEnd();
}

