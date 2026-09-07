#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"

// pch.hpp, not <windows.h> directly — it pulls winsock2 in first, which is the
// ordering every other TU here relies on.
#include "src/pch.hpp"

#include <string.h>
#include <string>

#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace PerfProbe {

namespace {

// Keep in sync with enum Slot.
const char* const kSlotNames[SLOT_COUNT] = {
	"ipc_drain",
	"mission_alerts",
	"combat_flush",
	"rebalance_trigger",
	"match_stats",
	"kismet_dump",
	"mission_vo",
	"suit_rebuild",
	"vr_heal_pad",
	"markers",
	"fx_browse",
	"beacon_reaper",
	"engine_tick",
	"gc",
};

// Keep in sync with enum Counter.
const char* const kCounterNames[CTR_COUNT] = {
	"process_event",
	"actor_tick",
	"ipc_send",
};

// A frame over this many ms gets its own breakdown line. 50ms ≈ 20fps; the
// server targets 30Hz, so anything past this is a visible hitch to every
// client on the instance.
constexpr double kSpikeMs = 50.0;

// Cadence of the rolling summary line.
constexpr double kSummarySeconds = 10.0;

// Re-read once per frame rather than per Scope — SLOT_ACTOR_TICK_FEEDS is
// entered once per actor per frame, and Logger::IsChannelEnabled is a
// string-keyed lookup.
bool    g_enabled = false;
bool    g_deepEnabled = false;

// Slowest single Actor::Tick this frame, for the "perfactors" tier.
int64_t     g_worstActorTicks = 0;
std::string g_worstActorClass;

int64_t g_freq       = 0;
int64_t g_frameStart = 0;

int64_t g_slotTicks[SLOT_COUNT];
int64_t g_counters[CTR_COUNT];

// Rolling window state.
int64_t g_windowStart   = 0;
int64_t g_windowSlot[SLOT_COUNT];
int64_t g_windowCounter[CTR_COUNT];
int64_t g_windowTotal   = 0;   // summed frame ticks
int64_t g_windowWorst   = 0;   // worst single frame, ticks
int     g_windowFrames  = 0;
int     g_windowSpikes  = 0;

inline int64_t Freq() {
	if (g_freq == 0) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		g_freq = (int64_t)li.QuadPart;
	}
	return g_freq;
}

inline double ToMs(int64_t ticks) {
	const int64_t f = Freq();
	return f ? (double)ticks * 1000.0 / (double)f : 0.0;
}

}  // namespace

bool Enabled() { return g_enabled; }

bool DeepEnabled() { return g_deepEnabled; }

void NoteActorTick(void* actor, int64_t ticks) {
	if (!g_deepEnabled || ticks <= g_worstActorTicks) return;
	g_worstActorTicks = ticks;
	// Only on a new maximum, so this is a handful of lookups per frame, not
	// one per actor. ObjectClassCache memoizes per UClass*.
	AActor* a = static_cast<AActor*>(actor);
	g_worstActorClass = (a && a->Class)
		? ObjectClassCache::GetClassName(a->Class) : "<null>";
}

double TicksToMs(int64_t ticks) { return ToMs(ticks); }

int64_t Now() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return (int64_t)li.QuadPart;
}

void AddTicks(Slot slot, int64_t ticks) {
	if (!g_enabled) return;
	if ((unsigned)slot >= (unsigned)SLOT_COUNT) return;
	g_slotTicks[slot] += ticks;
}

void Count(Counter counter, int n) {
	if (!g_enabled) return;
	if ((unsigned)counter >= (unsigned)CTR_COUNT) return;
	g_counters[counter] += n;
}

void BeginFrame() {
	// One string-keyed lookup per frame; everything else this frame reads the
	// cached bool. Picking the flag up here (rather than at DLL init) means
	// the channel can be turned on for a live instance without a restart if
	// the config is ever re-read.
	g_enabled = Logger::IsChannelEnabled("perf");
	if (!g_enabled) { g_deepEnabled = false; return; }
	g_deepEnabled = Logger::IsChannelEnabled("perfactors");

	Freq();

	g_worstActorTicks = 0;
	g_worstActorClass.clear();

	memset(g_slotTicks, 0, sizeof(g_slotTicks));
	memset(g_counters,  0, sizeof(g_counters));

	g_frameStart = Now();
	if (g_windowStart == 0) g_windowStart = g_frameStart;
}

void EndFrame() {
	if (!g_enabled || g_frameStart == 0) return;

	const int64_t frameTicks = Now() - g_frameStart;
	g_frameStart = 0;

	g_windowFrames++;
	g_windowTotal += frameTicks;
	if (frameTicks > g_windowWorst) g_windowWorst = frameTicks;
	for (int i = 0; i < SLOT_COUNT; i++)  g_windowSlot[i]    += g_slotTicks[i];
	for (int i = 0; i < CTR_COUNT;  i++)  g_windowCounter[i] += g_counters[i];

	// ── Spike line: one frame, full breakdown ──────────────────────────────
	const double frameMs = ToMs(frameTicks);
	if (frameMs >= kSpikeMs) {
		g_windowSpikes++;

		// Slots are disjoint except SLOT_ENGINE_TICK, which wraps the
		// engine's own tick (actor ticks + replication + net flush).
		// SLOT_ACTOR_TICK_FEEDS is our per-actor work and is therefore
		// *inside* engine_tick, not additive with it — hence "of which".
		char buf[768];
		int off = snprintf(buf, sizeof(buf), "[SPIKE] frame=%.1fms", frameMs);
		for (int i = 0; i < SLOT_COUNT && off > 0 && off < (int)sizeof(buf); i++) {
			const double ms = ToMs(g_slotTicks[i]);
			if (ms < 0.5) continue;   // don't bury the signal in noise
			off += snprintf(buf + off, sizeof(buf) - off, " %s=%.1f",
			                kSlotNames[i], ms);
		}
		for (int i = 0; i < CTR_COUNT && off > 0 && off < (int)sizeof(buf); i++) {
			off += snprintf(buf + off, sizeof(buf) - off, " %s=%lld",
			                kCounterNames[i], (long long)g_counters[i]);
		}
		if (g_deepEnabled && off > 0 && off < (int)sizeof(buf)) {
			off += snprintf(buf + off, sizeof(buf) - off, " worst_actor=%s/%.1fms",
			                g_worstActorClass.empty() ? "-" : g_worstActorClass.c_str(),
			                ToMs(g_worstActorTicks));
		}
		Logger::Log("perf", "%s\n", buf);
	}

	// ── Summary line: the window's shape, so a quiet log still says what
	//    normal looks like ────────────────────────────────────────────────
	const double windowMs = ToMs(Now() - g_windowStart);
	if (windowMs >= kSummarySeconds * 1000.0 && g_windowFrames > 0) {
		char buf[768];
		int off = snprintf(buf, sizeof(buf),
			"[SUMMARY] %.0fs frames=%d avg=%.1fms worst=%.1fms spikes=%d",
			windowMs / 1000.0, g_windowFrames,
			ToMs(g_windowTotal / g_windowFrames), ToMs(g_windowWorst),
			g_windowSpikes);
		for (int i = 0; i < SLOT_COUNT && off > 0 && off < (int)sizeof(buf); i++) {
			const double ms = ToMs(g_windowSlot[i] / g_windowFrames);
			if (ms < 0.05) continue;
			off += snprintf(buf + off, sizeof(buf) - off, " %s=%.2f",
			                kSlotNames[i], ms);
		}
		for (int i = 0; i < CTR_COUNT && off > 0 && off < (int)sizeof(buf); i++) {
			off += snprintf(buf + off, sizeof(buf) - off, " %s/f=%lld",
			                kCounterNames[i],
			                (long long)(g_windowCounter[i] / g_windowFrames));
		}
		Logger::Log("perf", "%s\n", buf);

		g_windowStart  = Now();
		g_windowFrames = 0;
		g_windowTotal  = 0;
		g_windowWorst  = 0;
		g_windowSpikes = 0;
		memset(g_windowSlot,    0, sizeof(g_windowSlot));
		memset(g_windowCounter, 0, sizeof(g_windowCounter));
	}
}

}  // namespace PerfProbe
