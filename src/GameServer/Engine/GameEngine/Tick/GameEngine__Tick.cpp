#include "src/GameServer/Engine/GameEngine/Tick/GameEngine__Tick.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"
#include "src/GameServer/Combat/CombatMessageFlusher/CombatMessageFlusher.hpp"
#include "src/GameServer/Matchmaking/SetupRebalanceTrigger/SetupRebalanceTrigger.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Engine/KismetWebDump/KismetWebDump.hpp"
#include "src/GameServer/TgGame/MissionVODirector/MissionVODirector.hpp"

void __fastcall GameEngine__Tick::Call(void* Engine, void* edx, float DeltaSeconds) {
	IpcClient::DrainInbound();
	// MissionAlerts self-throttles to ~1Hz off WorldInfo.TimeSeconds.
	MissionAlerts::Tick();
	// CombatMessageFlusher self-throttles to ~10Hz; drains per-pawn accumulators
	// that the engine's 21-record / 101-index threshold (FUN_109f9bc0) would
	// otherwise hold indefinitely for sparse damage events.
	CombatMessageFlusher::Tick();
	// Self-gates on mission timer SETUP state; fires one rebalance request 5s
	// before setup ends.
	SetupRebalanceTrigger::Tick();
	// Match stats: pending-death flush + objective capture/contest time.
	// Self-throttles to 4Hz; no-op when stats disabled (home map).
	MatchStats::Tick();
	// One-shot delayed kismet dump (no-op unless -dumpkismet armed it at
	// BeginPlay). Fires after the sublevel-streaming settle window so the dump
	// includes streamed sublevels (e.g. the *_Sound announcer-VO kismet).
	KismetWebDump::TickDelayedDump(DeltaSeconds);
	// Raid_DomeCityDefense_P: fire Ava's per-round satellite-countdown VO
	// (Bancroft_HalfwayPoint/_30sRemaining/_10sRemaining) off the round timer.
	// Self-gates on map name + Defense round state; no-op elsewhere.
	MissionVODirector::Tick(DeltaSeconds);
	CallOriginal(Engine, edx, DeltaSeconds);
}
