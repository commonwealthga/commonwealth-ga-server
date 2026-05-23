#include "src/GameServer/Engine/GameEngine/Tick/GameEngine__Tick.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"
#include "src/GameServer/Combat/CombatMessageFlusher/CombatMessageFlusher.hpp"

void __fastcall GameEngine__Tick::Call(void* Engine, void* edx, float DeltaSeconds) {
	IpcClient::DrainInbound();
	// MissionAlerts self-throttles to ~1Hz off WorldInfo.TimeSeconds.
	MissionAlerts::Tick();
	// CombatMessageFlusher self-throttles to ~10Hz; drains per-pawn accumulators
	// that the engine's 21-record / 101-index threshold (FUN_109f9bc0) would
	// otherwise hold indefinitely for sparse damage events.
	CombatMessageFlusher::Tick();
	CallOriginal(Engine, edx, DeltaSeconds);
}
