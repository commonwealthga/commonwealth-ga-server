#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"

// pch.hpp, not <windows.h> directly — it pulls winsock2 in first, which is the
// ordering every other TU here relies on.
#include "src/pch.hpp"

#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace PerfProbe {

namespace {

// p1100c: game-thread CPU time (user+kernel, 100 ns units) at the window start.
uint64_t g_windowThreadCpu = 0;
uint64_t ThreadCpu100ns() {
	FILETIME c, e, k, u;
	if (!GetThreadTimes(GetCurrentThread(), &c, &e, &k, &u)) return 0;
	return ((uint64_t)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((uint64_t)u.dwHighDateTime << 32 | u.dwLowDateTime);
}

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
	"los_trace",
	"replicate",
	"relevant",
	"ai_ctrl",
	"ai_pawn",
	"pc_tick",
};

// Keep in sync with enum Counter.
const char* const kCounterNames[CTR_COUNT] = {
	"process_event",
	"actor_tick",
	"ipc_send",
	"los_trace",
	"los_dup",
	"los_ctrl",
	"replicate",
	"relevant",
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

// p1100d: per-class tick totals for the 10 s window ("perfactors").
struct ClassTicks { int64_t n = 0; int64_t ticks = 0; };
std::unordered_map<void*, ClassTicks> g_classTicks;

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
	if (g_deepEnabled && actor) {  // p1100d
		ClassTicks& ct = g_classTicks[static_cast<AActor*>(actor)->Class];
		ct.n++; ct.ticks += ticks;
	}
	if (!g_deepEnabled || ticks <= g_worstActorTicks) return;
	g_worstActorTicks = ticks;
	// Only on a new maximum, so this is a handful of lookups per frame, not
	// one per actor. ObjectClassCache memoizes per UClass*.
	AActor* a = static_cast<AActor*>(actor);
	g_worstActorClass = (a && a->Class)
		? ObjectClassCache::GetClassName(a->Class) : "<null>";
}

double TicksToMs(int64_t ticks) { return ToMs(ticks); }

// p1100g: ReplicateActor per actor class for the 10 s window.
static std::unordered_map<void*, ClassTicks> g_repTicks;
void NoteRepActor(void* actor, int64_t ticks) {
	if (!g_deepEnabled || !actor) return;
	ClassTicks& e = g_repTicks[static_cast<AActor*>(actor)->Class];
	e.n++; e.ticks += ticks;
}

// p1100f: ProcessEvent per UFunction for the 10 s window ("perfactors").
static std::unordered_map<void*, ClassTicks> g_eventTicks;  // n = all calls, ticks = top-level inclusive
void NoteEvent(void* function, int64_t ticks) {
	if (!g_deepEnabled || !function) return;
	ClassTicks& e = g_eventTicks[function];
	e.n++;
	if (ticks > 0) e.ticks += ticks;
}

static int64_t g_frameNumber = 0;
int64_t FrameNumber() { return g_frameNumber; }

int64_t Now() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return (int64_t)li.QuadPart;
}

void AddTicks(Slot slot, int64_t ticks) {
	if (!g_enabled) return;
	if ((unsigned)slot >= (unsigned)SLOT_COUNT) return;
	g_slotTicks[slot] += ticks;
	g_windowSlot[slot] += ticks;  // p1100b
}

void Count(Counter counter, int n) {
	if (!g_enabled) return;
	if ((unsigned)counter >= (unsigned)CTR_COUNT) return;
	g_counters[counter] += n;
	g_windowCounter[counter] += n;  // p1100b: also outside the GameEngine::Tick window
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
	g_frameNumber++;
	if (!g_enabled || g_frameStart == 0) return;

	const int64_t frameTicks = Now() - g_frameStart;
	g_frameStart = 0;

	g_windowFrames++;
	g_windowTotal += frameTicks;
	if (frameTicks > g_windowWorst) g_windowWorst = frameTicks;
	// p1100b: g_windowSlot / g_windowCounter are fed directly by AddTicks / Count

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
		{
			const uint64_t cpu = ThreadCpu100ns();
			if (g_windowThreadCpu != 0 && cpu >= g_windowThreadCpu && off > 0 && off < (int)sizeof(buf))
				off += snprintf(buf + off, sizeof(buf) - off, " thread_cpu/f=%.2fms",
				                (double)(cpu - g_windowThreadCpu) / 10000.0 / g_windowFrames);
			g_windowThreadCpu = cpu;
		}
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

		if (!g_repTicks.empty()) {  // p1100g
			std::vector<std::pair<int64_t, void*>> v;
			for (auto& kv : g_repTicks) v.push_back({kv.second.ticks, kv.first});
			std::sort(v.begin(), v.end(), [](const std::pair<int64_t, void*>& a, const std::pair<int64_t, void*>& b) { return a.first > b.first; });
			std::string line = "[REPCLASS] ReplicateActor per frame (n, ms):";
			char cb[200];
			for (size_t i = 0; i < v.size() && i < 20; i++) {
				const ClassTicks& e = g_repTicks[v[i].second];
				snprintf(cb, sizeof cb, " %s=%.1f/%.3f", ObjectClassCache::GetClassName((UClass*)v[i].second).c_str(), (double)e.n / g_windowFrames, ToMs(e.ticks) / g_windowFrames);
				line += cb;
			}
			snprintf(cb, sizeof cb, " classes=%d", (int)v.size()); line += cb;
			Logger::Log("perf", "%s\n", line.c_str());
			g_repTicks.clear();
		}
		if (!g_eventTicks.empty()) {  // p1100f
			for (int pass = 0; pass < 2; pass++) {
				std::vector<std::pair<int64_t, void*>> v;
				for (auto& kv : g_eventTicks) v.push_back({pass ? kv.second.n : kv.second.ticks, kv.first});
				std::sort(v.begin(), v.end(), [](const std::pair<int64_t, void*>& a, const std::pair<int64_t, void*>& b) { return a.first > b.first; });
				std::string line = pass ? "[EVENTS] by calls per frame (n, top-level ms):" : "[EVENTS] by top-level time per frame (n, ms):";
				char cb[200];
				for (size_t i = 0; i < v.size() && i < 20; i++) {
					const ClassTicks& e = g_eventTicks[v[i].second];
					std::string nm = ((UObject*)v[i].second)->GetFullName();
					snprintf(cb, sizeof cb, " %s=%.1f/%.3f", nm.c_str(), (double)e.n / g_windowFrames, ToMs(e.ticks) / g_windowFrames);
					line += cb;
				}
				snprintf(cb, sizeof cb, " functions=%d", (int)v.size()); line += cb;
				Logger::Log("perf", "%s\n", line.c_str());
			}
			g_eventTicks.clear();
		}
		if (!g_classTicks.empty()) {  // p1100d
			std::vector<std::pair<int64_t, void*>> v;
			for (auto& kv : g_classTicks) v.push_back({kv.second.ticks, kv.first});
			std::sort(v.begin(), v.end(), [](const std::pair<int64_t, void*>& a, const std::pair<int64_t, void*>& b) { return a.first > b.first; });
			std::string line = "[ACTORCLASS] per frame (n, ms):";
			char cb[160];
			for (size_t i = 0; i < v.size() && i < 15; i++) {
				const ClassTicks& ct = g_classTicks[v[i].second];
				snprintf(cb, sizeof cb, " %s=%.1f/%.3f", ObjectClassCache::GetClassName((UClass*)v[i].second).c_str(),
				         (double)ct.n / g_windowFrames, ToMs(ct.ticks) / g_windowFrames);
				line += cb;
			}
			snprintf(cb, sizeof cb, " classes=%d", (int)v.size()); line += cb;
			Logger::Log("perf", "%s\n", line.c_str());
			g_classTicks.clear();
		}
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
