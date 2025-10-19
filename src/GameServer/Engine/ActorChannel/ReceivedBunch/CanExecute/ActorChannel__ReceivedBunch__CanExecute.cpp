#include "src/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __cdecl ActorChannel__ReceivedBunch__CanExecute::Call(void* param_1, void* param_2, int param_3) {
	bool result = (*(int *)(param_3 + 0x90) & 0x00200040) == 0x00200040; // allow server functions

	UObject* func = (UObject*)param_3;
	if (result) {
		if (
			strcmp(func->GetFullName(), "Function Engine.PlayerController.ServerMove") != 0
			&& strcmp(func->GetFullName(), "Function Engine.PlayerController.ServerUpdatePing") != 0
			&& strcmp(func->GetFullName(), "Function Engine.PlayerController.OldServerMove") != 0
		) {
			Logger::Log("rpc", "[%s] [ACCEPT] %s\n", Logger::GetTime(), func->GetFullName());
		}
	} else {
		Logger::Log("rpc", "[%s] [REJECT] %s\n", Logger::GetTime(), func->GetFullName());
	}

	return result;
}

