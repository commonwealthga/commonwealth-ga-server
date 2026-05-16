#include "src/GameServer/Engine/GameEngine/Tick/GameEngine__Tick.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"

void __fastcall GameEngine__Tick::Call(void* Engine, void* edx, float DeltaSeconds) {
	IpcClient::DrainInbound();
	// MissionAlerts self-throttles to ~1Hz off WorldInfo.TimeSeconds.
	MissionAlerts::Tick();
	CallOriginal(Engine, edx, DeltaSeconds);
}
