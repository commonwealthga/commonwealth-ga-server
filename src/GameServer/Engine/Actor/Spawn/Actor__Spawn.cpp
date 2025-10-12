#include "src/GameServer/Engine/Actor/Spawn/Actor__Spawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

AActor* __fastcall Actor__Spawn::Call(UWorld* pThis, void* edx, UClass* Class, uint64_t InNameStub, FVector* Location, FRotator* Rotation, AActor* Template, bool bNoCollisionFail, bool bRemoteOwned, AActor* Owner, bool bNoFail, ULevel* OverrideLevel) {
	Logger::Log("debug", "MINE SpawnActor START Requested: %s\n", Class->GetFullName());

	AActor* ret = CallOriginal(pThis, edx, Class, InNameStub, Location, Rotation, Template, bNoCollisionFail, bRemoteOwned, Owner, bNoFail, OverrideLevel);

	if (ret) {
		Logger::Log("debug", "MINE SpawnActor END Spawned: %s\n", ret->GetFullName());
	} else {
		Logger::Log("debug", "MINE SpawnActor END Failed to spawn\n");
	}
	
	return ret;
}

