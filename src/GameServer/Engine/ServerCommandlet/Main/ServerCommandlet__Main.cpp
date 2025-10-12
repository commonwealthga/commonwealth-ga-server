#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __stdcall ServerCommandlet__Main::Call() {
	Logger::Log("debug", "MINE ServerCommandlet__Main START\n");
	int result = ServerCommandlet__Main::CallOriginal();

	Logger::Log("debug", "MINE ServerCommandlet__Main END\n");
	return result;
}

