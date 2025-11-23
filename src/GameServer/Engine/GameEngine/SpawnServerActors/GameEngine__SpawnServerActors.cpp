#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/Utils/Logger/Logger.hpp"

void GameEngine__SpawnServerActors::Call(void* GameEngine) {
	LogCallBegin();
	GameEngine__SpawnServerActors::CallOriginal(GameEngine);
	LogCallEnd();
}

