#include "src/GameServer/Engine/GameEngine/Tick/GameEngine__Tick.hpp"
#include "src/IpcClient/IpcClient.hpp"

void __fastcall GameEngine__Tick::Call(void* Engine, void* edx, float DeltaSeconds) {
	IpcClient::DrainInbound();
	CallOriginal(Engine, edx, DeltaSeconds);
}
