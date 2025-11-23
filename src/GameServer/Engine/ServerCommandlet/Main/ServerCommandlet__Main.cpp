#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __stdcall ServerCommandlet__Main::Call() {
	LogCallBegin();
	int result = ServerCommandlet__Main::CallOriginal();
	LogCallEnd();

	return result;
}

