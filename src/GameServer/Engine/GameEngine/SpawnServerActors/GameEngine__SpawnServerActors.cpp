#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/Utils/Logger/Logger.hpp"

void GameEngine__SpawnServerActors::Call(void* GameEngine) {
	Logger::Log("debug", "MINE GameEngine__SpawnServerActors START\n");
	GameEngine__SpawnServerActors::CallOriginal(GameEngine);
	Logger::Log("debug", "MINE GameEngine__SpawnServerActors END\n");
}

