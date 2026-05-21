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

	// Sample the requested-class name ONCE before the call when we might need
	// it for fire_gate diagnostics (Spawn failure leaves us with ret==null and
	// no way to recover the class). Cheap when fire_gate channel is off.
	const bool fireGateLog = Logger::IsChannelEnabled("fire_gate");
	const char* requestedClassName = nullptr;
	bool isProjectileSpawn = false;
	if (fireGateLog && Class) {
		requestedClassName = Class->GetFullName();
		if (requestedClassName) {
			isProjectileSpawn =
				strstr(requestedClassName, "TgProj_")     != nullptr ||
				strstr(requestedClassName, "TgProjectile") != nullptr ||
				strstr(requestedClassName, "Engine.Projectile") != nullptr;
		}
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
		} else {
			
		}
	} else if (debugLog) {
		Logger::Log("debug", "MINE SpawnActor END Failed to spawn\n");
	}

	// Pair the per-projectile Spawn outcome to fire_gate so the
	// firing-while-jetpacking diagnostic can correlate ServerStartFire
	// (gate-passed verdict) with whether the projectile actor actually
	// materialised. Logs success+pointer or failure+input args.
	if (fireGateLog && isProjectileSpawn) {
		float lx = Location ? Location->X : 0.f;
		float ly = Location ? Location->Y : 0.f;
		float lz = Location ? Location->Z : 0.f;
		int rp = Rotation ? Rotation->Pitch : 0;
		int ry = Rotation ? Rotation->Yaw   : 0;
		int rr = Rotation ? Rotation->Roll  : 0;
		if (ret) {
			Logger::Log("fire_gate",
				"[Spawn-projectile] OK  class=%s -> %p  loc=(%.1f, %.1f, %.1f) rot=(p=%d, y=%d, r=%d) "
				"bNoCollisionFail=%d bNoFail=%d owner=%p template=%p\n",
				requestedClassName, ret, lx, ly, lz, rp, ry, rr,
				(int)bNoCollisionFail, (int)bNoFail, Owner, Template);
		} else {
			Logger::Log("fire_gate",
				"[Spawn-projectile] FAIL class=%s -> NULL  loc=(%.1f, %.1f, %.1f) rot=(p=%d, y=%d, r=%d) "
				"bNoCollisionFail=%d bNoFail=%d owner=%p template=%p\n",
				requestedClassName, lx, ly, lz, rp, ry, rr,
				(int)bNoCollisionFail, (int)bNoFail, Owner, Template);
		}
	}

	return ret;
}

