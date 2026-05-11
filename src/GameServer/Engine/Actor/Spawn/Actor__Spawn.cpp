#include "src/GameServer/Engine/Actor/Spawn/Actor__Spawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

AActor* __fastcall Actor__Spawn::Call(UWorld* pThis, void* edx, UClass* Class, uint64_t InNameStub, FVector* Location, FRotator* Rotation, AActor* Template, bool bNoCollisionFail, bool bRemoteOwned, AActor* Owner, bool bNoFail, ULevel* OverrideLevel) {
	// Spawn fires extremely frequently (every projectile, every transient FX),
	// so the START/END debug logs are gated behind the "debug" channel — when
	// it's off (production), we skip both the GetFullName calls and the Log
	// invocations entirely.
	const bool debugLog = Logger::IsChannelEnabled("debug");
	if (debugLog) {
		Logger::Log("debug", "MINE SpawnActor START Requested: %s\n", Class->GetFullName());
	}

	AActor* ret = CallOriginal(pThis, edx, Class, InNameStub, Location, Rotation, Template, bNoCollisionFail, bRemoteOwned, Owner, bNoFail, OverrideLevel);

	if (ret) {
		if (debugLog) {
			Logger::Log("debug", "MINE SpawnActor END Spawned: %s\n", ret->GetFullName());
		}

		// Projectiles are fast-moving and short-lived. Without bAlwaysRelevant they can miss
		// the NetDriver's relevancy window and never replicate to clients.
		const char* cn = ret->Class->GetFullName();
		if (strstr(cn, "TgProj_") || strstr(cn, "TgProjectile") || strstr(cn, "Engine.Projectile")) {
			ret->bForceNetUpdate = 1;
			ret->bNetInitial = 1;
			ret->bNetDirty = 1;
			ret->NetUpdateFrequency = 5;
			ret->NetPriority = 10;
		}
	} else if (debugLog) {
		Logger::Log("debug", "MINE SpawnActor END Failed to spawn\n");
	}

	return ret;
}

