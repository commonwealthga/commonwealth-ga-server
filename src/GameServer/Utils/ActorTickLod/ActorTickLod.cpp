#include "src/GameServer/Utils/ActorTickLod/ActorTickLod.hpp"

#include <wchar.h>
#include <cwchar>
#include <unordered_map>
#include <vector>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"
#include "src/GameServer/Utils/PerfKnobs/PerfKnobs.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace ActorTickLod {

namespace {

constexpr float kNearDistSq = kNearDistance * kNearDistance;
constexpr float kFarDistSq = kFarDistance * kFarDistance;
constexpr float kSleepDistSq = kSleepDistance * kSleepDistance;
constexpr float kRefreshSeconds = 0.25f;  // player list refresh

bool g_enabled = true;
bool g_active = false;
void* g_game = nullptr;            // GameInfo the active flag was computed for
unsigned g_frame = 0;
float g_deltas[kStride] = {};       // last kStride frame times
float g_refreshAcc = kRefreshSeconds;
float g_logAcc = 0.0f;
unsigned g_ran = 0, g_skipped = 0, g_near = 0, g_calls = 0;
unsigned g_aiRan = 0, g_aiSkipped = 0, g_aiCalls = 0;  // p1102: TgAIController ticks
float g_lastSum = 0.0f;  // pawn + controller ticks since the last perf line
std::vector<AController*> g_players;
std::vector<FVector> g_playerLocs;
std::unordered_map<APawn*, float> g_netFreq;  // AI pawns with a capped NetUpdateFrequency -> original
UNetDriver* g_driver = nullptr;
int g_zoneTick = kZoneTickRate;
int g_baseTick = 0;  // configured NetServerMaxTickRate

float NearestPlayerSq(const FVector& loc);

void SetTickRate(bool zone) {
	if (g_driver == nullptr || g_zoneTick <= 0) return;
	if (g_baseTick == 0) g_baseTick = g_driver->NetServerMaxTickRate;
	const int want = zone ? g_zoneTick : g_baseTick;
	if (g_driver->NetServerMaxTickRate != want) {
		g_driver->NetServerMaxTickRate = want;
		Logger::Log("perf", "[ActorTickLod] server tick rate %d\n", want);
	}
}

// Cap the replication rate of AI pawns past kNearDistance; restore it when a
// player comes close or the pawn is gone.
void UpdateNetFreq(AWorldInfo* wi) {
	std::unordered_map<APawn*, float> next;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe || p->Controller != c || p->bAlwaysRelevant) continue;
		bool isPlayer = false;
		for (AController* pc : g_players) if (pc == c) { isPlayer = true; break; }
		if (isPlayer) continue;
		auto it = g_netFreq.find(p);
		const bool capped = it != g_netFreq.end();
		const float orig = capped ? it->second : p->NetUpdateFrequency;
		if (NearestPlayerSq(p->Location) >= kNearDistSq && orig > kMidNetFreq) {
			p->NetUpdateFrequency = kMidNetFreq;
			next[p] = orig;
		} else if (capped) {
			p->NetUpdateFrequency = orig;
		}
	}
	g_netFreq.swap(next);  // pawns no longer listed are gone (or lost their controller)
}

void Refresh() {
	g_players.clear();
	g_playerLocs.clear();
	if (Globals::Get().GWorld == nullptr) { g_active = false; return; }
	AWorldInfo* wi = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (wi == nullptr || wi->Game == nullptr) { g_active = false; g_game = nullptr; return; }
	if ((void*)wi->Game != g_game) {
		g_game = wi->Game;
		const std::string& cn = ObjectClassCache::GetClassName((UObject*)wi->Game);
		g_active = g_enabled && (cn == "Class TgGame.TgGame_City" || cn == "Class TgGame.TgGame_OpenWorldPVE");
		g_netFreq.clear();
		Logger::Log("perf", "[ActorTickLod] %s for %s\n", g_active ? "on" : "off", cn.c_str());
		SetTickRate(cn == "Class TgGame.TgGame_OpenWorldPVE");
	}
	if (!g_active) return;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) continue;
		g_players.push_back(c);
		if (c->Pawn != nullptr) g_playerLocs.push_back(c->Pawn->Location);
	}
	UpdateNetFreq(wi);
}

bool IsPlayer(AController* c) {
	for (AController* p : g_players) if (p == c) return true;
	return false;
}

// Squared distance to the nearest player pawn (huge when there is none).
float NearestPlayerSq(const FVector& loc) {
	const float l[3] = {loc.X, loc.Y, loc.Z};
	float best = 3.0e38f;
	for (const FVector& p : g_playerLocs) { const float d = DistSq(p, l); if (d < best) best = d; }
	return best;
}

// Frame time to pass when an actor ticks: 0 = skip this frame.
float TickDelta(APawn* pawn, float delta, bool& throttled) {
	throttled = false;
	if (!g_active || pawn == nullptr || pawn->bDeleteMe) return delta;
	AController* c = pawn->Controller;
	if (c == nullptr || IsPlayer(c)) return delta;
	if (pawn->bAlwaysRelevant || pawn->Physics == 2 /* PHYS_Falling */) return delta;
	const float nearest = NearestPlayerSq(pawn->Location);
	if (nearest < kNearDistSq) { ++g_near; return delta; }
	if (nearest >= kSleepDistSq) return 0.0f;
	const int stride = nearest < kFarDistSq ? kMidStride : kStride;
	// Spread the throttled actors over the stride frames by a hash of the address
	// (pawns are allocated on 8 KB boundaries, so low address bits are all equal).
	const unsigned phase = ((unsigned)(uintptr_t)pawn * 2654435761u) >> 24;
	if ((phase + g_frame) % stride != 0) return 0.0f;
	float sum = 0.0f;
	for (int i = 0; i < stride; ++i) sum += g_deltas[(g_frame + kStride - i) % kStride];
	g_lastSum = sum;
	throttled = true;
	return sum;
}

// The engine ticks an actor only while its bTicked differs from the world's
// per-frame toggle; a real tick flips it. A skipped frame must flip it too, or
// the engine stops offering the actor on every other frame (and an even stride
// would never land on its frame again).
void MarkTicked(AActor* a) { a->bTicked = !a->bTicked; }

}  // namespace

bool Active() { return g_active; }

void BeginFrame(float DeltaSeconds) {
	++g_frame;
	g_deltas[g_frame % kStride] = DeltaSeconds;
	g_refreshAcc += DeltaSeconds;
	if (g_refreshAcc >= kRefreshSeconds) { g_refreshAcc = 0.0f; Refresh(); }
	if (g_active && PerfProbe::Enabled()) {
		g_logAcc += DeltaSeconds;
		if (g_logAcc >= 10.0f) {
			Logger::Log("perf", "[ActorTickLod] 10s ai ticks ran=%u skipped=%u near=%u calls=%u sum=%.3f frame=%u players=%u aictrl calls=%u ran=%u skipped=%u\n",
			            g_ran, g_skipped, g_near, g_calls, g_lastSum, g_frame, (unsigned)g_players.size(), g_aiCalls, g_aiRan, g_aiSkipped);
			g_logAcc = 0.0f; g_ran = g_skipped = g_near = g_calls = 0; g_aiRan = g_aiSkipped = g_aiCalls = 0;
		}
	}
}

void SetNetDriver(UNetDriver* driver) { g_driver = driver; g_baseTick = 0; }

void Install() {
	if (wcsstr(GetCommandLineW(), L"-noactorlod")) {
		g_enabled = false;
		Logger::Log("perf", "[ActorTickLod] off (-noactorlod)\n");
	}
	if (const wchar_t* z = wcsstr(GetCommandLineW(), L"-zonetick=")) {
		g_zoneTick = (int)wcstol(z + 10, nullptr, 10);
		if (g_zoneTick != 0 && (g_zoneTick < 10 || g_zoneTick > 60)) g_zoneTick = kZoneTickRate;
	}
	Pawn__Tick::Install();
	Controller__Tick::Install();
	TgAIController__Tick::Install();  // p1102
}

}  // namespace ActorTickLod

using namespace ActorTickLod;

// p1100f: time the original tick by owner kind (perf channel only).
static PerfProbe::Slot TickSlot(AController* c, bool pawn) {
	if (c != nullptr && IsPlayer(c)) return PerfProbe::SLOT_PC_TICK;
	return pawn ? PerfProbe::SLOT_AI_PAWN : PerfProbe::SLOT_AI_CTRL;
}

void* __fastcall Pawn__Tick::Call(void* a1, void* edx, float a2, int a3) {
	if (!g_active) return CallOriginal(a1, edx, a2, a3);
	++g_calls;
	bool throttled;
	const float d = TickDelta((APawn*)a1, a2, throttled);
	if (d == 0.0f) { MarkTicked((AActor*)a1); ++g_skipped; return nullptr; }
	if (throttled) ++g_ran;
	PerfProbe::Scope _t(TickSlot(((APawn*)a1)->Controller, true));
	return CallOriginal(a1, edx, d, a3);
}

void* __fastcall Controller__Tick::Call(void* a1, void* edx, float a2, int a3) {
	if (!g_active) return CallOriginal(a1, edx, a2, a3);
	AController* c = (AController*)a1;
	if (IsPlayer(c) || c->Pawn == nullptr || c->Pawn->Controller != c) { PerfProbe::Scope _t(TickSlot(c, false)); return CallOriginal(a1, edx, a2, a3); }
	bool throttled;
	const float d = TickDelta(c->Pawn, a2, throttled);
	if (d == 0.0f) { MarkTicked((AActor*)a1); ++g_skipped; return nullptr; }
	if (throttled) ++g_ran;
	PerfProbe::Scope _t(PerfProbe::SLOT_AI_CTRL);
	return CallOriginal(a1, edx, d, a3);
}

// p1102: an AI controller follows its pawn's tier (same phase, same summed
// delta), so a sleeping pawn no longer has a controller thinking for it every
// frame. cs_settings perf_aictrl_lod = 0 restores the old every-frame tick.
void* __fastcall TgAIController__Tick::Call(void* a1, void* edx, float a2, int a3) {
	AController* c = (AController*)a1;
	if (!g_active || c->Pawn == nullptr || c->Pawn->Controller != c || PerfKnobs::AiCtrlLod() == 0) {
		PerfProbe::Scope _t(PerfProbe::SLOT_AI_CTRL);
		return CallOriginal(a1, edx, a2, a3);
	}
	++g_aiCalls;
	bool throttled;
	const float d = TickDelta(c->Pawn, a2, throttled);
	if (d == 0.0f) { MarkTicked((AActor*)a1); ++g_aiSkipped; return nullptr; }
	if (throttled) ++g_aiRan;
	PerfProbe::Scope _t(PerfProbe::SLOT_AI_CTRL);
	return CallOriginal(a1, edx, d, a3);
}
