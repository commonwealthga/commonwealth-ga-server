#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall ServerCommandlet__Main::Call(void* This, void* edx, void* Params) {
	LogCallBegin();
	int result = ServerCommandlet__Main::CallOriginal(This, Params);
	LogCallEnd();

	return result;
}

