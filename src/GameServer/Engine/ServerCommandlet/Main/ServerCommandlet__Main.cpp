#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"

int __stdcall ServerCommandlet__Main::Call() {
	// todo maybe log
	int result = ServerCommandlet__Main::CallOriginal();

	// test
	return result;
}

