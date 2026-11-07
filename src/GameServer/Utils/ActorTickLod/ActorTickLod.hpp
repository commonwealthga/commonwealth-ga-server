#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Distance level-of-detail for AI actors in the hub and the open-world zones.
//
// Missions and PvP keep the stock behaviour. In TgGame_City / TgGame_OpenWorldPVE
// instances (one map, many players spread over it, many idle enemies):
//   - an AI pawn and its controller with no player within kNearDistance tick
//     on every kMidStride-th frame, past kFarDistance on every kStride-th
//     frame, with the summed frame time, so movement, timers and state code
//     still advance at the same speed; past kNearDistance their
//     NetUpdateFrequency is capped at kMidNetFreq;
//   - with no player within kSleepDistance they do not tick at all (nobody can
//     see them: they are already not relevant past kFarDistance) and resume
//     where they stopped when a player comes closer;
//   - open-world zones run the server at kZoneTickRate frames per second
//     (-zonetick=N to change, 0 = keep the configured rate);
//   - an AI pawn (not bAlwaysRelevant) farther than kFarDistance from a viewer
//     is not relevant to that viewer (no line-of-sight trace, no replication);
//   - another player's pawn farther than kPlayerDistance from a viewer is not
//     relevant to that viewer.
// Falling pawns always tick every frame. Off with the -noactorlod switch.
namespace ActorTickLod {

constexpr float kNearDistance   = 6000.0f;
constexpr float kFarDistance    = 10000.0f;
constexpr float kSleepDistance  = 12000.0f;
constexpr float kPlayerDistance = 12000.0f;
constexpr int   kMidStride      = 2;
constexpr int   kStride         = 4;
constexpr float kMidNetFreq     = 10.0f;
constexpr int   kZoneTickRate   = 20;

void Install();

// The listening net driver (its NetServerMaxTickRate sets the frame rate).
void SetNetDriver(UNetDriver* driver);

// Top of GameEngine::Tick.
void BeginFrame(float DeltaSeconds);

// True in hub / open-world instances while the LOD is on.
bool Active();

// Squared distance between two points.
inline float DistSq(const FVector& a, const float* b) {
	const float dx = a.X - b[0], dy = a.Y - b[1], dz = a.Z - b[2];
	return dx * dx + dy * dy + dz * dz;
}

}  // namespace ActorTickLod

class Pawn__Tick : public HookBase<
	void*(__fastcall*)(void*, void*, float, int),
	0x10ca38f0,   // APawn::Tick, vtable slot 98 of every pawn class
	Pawn__Tick> {
public:
	static void* __fastcall Call(void* a1, void* edx, float a2, int a3);
	static inline void* __fastcall CallOriginal(void* a1, void* edx, float a2, int a3) {
		return m_original(a1, edx, a2, a3);
	};
};

class Controller__Tick : public HookBase<
	void*(__fastcall*)(void*, void*, float, int),
	0x10cc92c0,   // AController::Tick, vtable slot 98 of the controller classes
	Controller__Tick> {
public:
	static void* __fastcall Call(void* a1, void* edx, float a2, int a3);
	static inline void* __fastcall CallOriginal(void* a1, void* edx, float a2, int a3) {
		return m_original(a1, edx, a2, a3);
	};
};

// p1102: TgAIController overrides Tick (vtable slot 98 = 0x10a7f5e0, used by that
// class only) and never calls AController::Tick, so the hook above never saw an
// AI controller: every enemy in a zone thought every frame however far away.
class TgAIController__Tick : public HookBase<
	void*(__fastcall*)(void*, void*, float, int),
	0x10a7f5e0,   // TgAIController::Tick
	TgAIController__Tick> {
public:
	static void* __fastcall Call(void* a1, void* edx, float a2, int a3);
	static inline void* __fastcall CallOriginal(void* a1, void* edx, float a2, int a3) {
		return m_original(a1, edx, a2, a3);
	};
};
