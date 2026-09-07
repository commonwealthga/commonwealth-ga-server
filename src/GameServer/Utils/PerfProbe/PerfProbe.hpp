#pragma once

#include <stdint.h>

// Frame-time probe for the server game thread.
//
// The game is single-threaded: everything the engine ticks, everything our
// hooks do, and every blocking call any of them makes lands inside one
// GameEngine::Tick. A "stutter" is one of those frames running long. This
// splits a frame into named slots so a spike line names the slot that ate it
// instead of leaving the whole frame as one number.
//
// Cost when the "perf" channel is off: BeginFrame/EndFrame return after one
// bool test; Scope ctor/dtor and Count() return after the same test. Nothing
// is timed and nothing is logged. Enable via control-server.json
// enabled_channels: ["perf"].
namespace PerfProbe {

// One slot per timed region. Keep in sync with kSlotNames in the .cpp.
enum Slot {
	SLOT_IPC_DRAIN = 0,
	SLOT_MISSION_ALERTS,
	SLOT_COMBAT_FLUSH,
	SLOT_REBALANCE_TRIGGER,
	SLOT_MATCH_STATS,
	SLOT_KISMET_DUMP,
	SLOT_MISSION_VO,
	SLOT_SUIT_REBUILD,
	SLOT_VR_HEAL_PAD,
	SLOT_MARKERS,
	SLOT_FX_BROWSE,
	SLOT_BEACON_REAPER,
	SLOT_ENGINE_TICK,        // CallOriginal: actor ticks + replication + net
	SLOT_GC,                 // UObject::CollectGarbage — nested INSIDE engine_tick
	SLOT_COUNT
};

// One counter per per-frame event tally. Keep in sync with kCounterNames.
enum Counter {
	CTR_PROCESS_EVENT = 0,   // UObject::ProcessEvent calls
	CTR_ACTOR_TICK,          // Actor::Tick calls
	CTR_IPC_SEND,            // IpcClient::Send calls (queued, not written)
	CTR_COUNT
};

bool Enabled();

// Second tier, channel "perfactors". Times every individual Actor::Tick to
// name the slowest actor in a bad frame. That is two QueryPerformanceCounter
// calls per actor per frame, so it is deliberately NOT part of "perf" — turn
// it on only after a "perf" run has pinned the stall inside engine_tick with
// gc ruled out. Cost when off: one bool test per actor.
bool DeepEnabled();

// Record one actor's tick duration. No-op unless DeepEnabled(). Takes void*
// so this header stays free of the SDK — it is included by the hottest TUs.
void NoteActorTick(void* actor, int64_t ticks);

void BeginFrame();
void EndFrame();

void AddTicks(Slot slot, int64_t ticks);
void Count(Counter counter, int n = 1);

int64_t Now();
double  TicksToMs(int64_t ticks);

// RAII timer. `PerfProbe::Scope s(PerfProbe::SLOT_MATCH_STATS);`
class Scope {
public:
	explicit Scope(Slot slot) : m_slot(slot), m_start(Enabled() ? Now() : 0) {}
	~Scope() {
		if (m_start != 0) AddTicks(m_slot, Now() - m_start);
	}
	Scope(const Scope&) = delete;
	Scope& operator=(const Scope&) = delete;
private:
	Slot    m_slot;
	int64_t m_start;
};

}  // namespace PerfProbe
