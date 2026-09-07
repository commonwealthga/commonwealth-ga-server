#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"

#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool UObject__CollectGarbage::bDisableGarbageCollection = false;

void __cdecl UObject__CollectGarbage::Call(void* param_1, uint32_t param_2, void* param_3) {
	if (bDisableGarbageCollection) return;

	// GC is a stop-the-world mark+sweep over every live UObject, on the one
	// thread that also runs the game. Its pause lands inside a single
	// GameEngine::Tick, so it reads to clients as a periodic hitch rather
	// than steady slowdown. Timed here (not just as part of engine_tick) so
	// a spike line can say whether the frame was GC or something else.
	// No-op when the "perf" channel is off.
	if (!PerfProbe::Enabled()) {
		CallOriginal(param_1, param_2, param_3);
		return;
	}

	const int objectsBefore = UObject::GObjObjects() ? UObject::GObjObjects()->Count : -1;
	const int64_t start = PerfProbe::Now();

	CallOriginal(param_1, param_2, param_3);

	const int64_t elapsed = PerfProbe::Now() - start;
	PerfProbe::AddTicks(PerfProbe::SLOT_GC, elapsed);

	const int objectsAfter = UObject::GObjObjects() ? UObject::GObjObjects()->Count : -1;
	Logger::Log("perf", "[GC] pause=%.1fms objects=%d->%d flags=0x%08X\n",
		PerfProbe::TicksToMs(elapsed), objectsBefore, objectsAfter,
		(unsigned)param_2);
}
