#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

#include <map>
#include <vector>

class TgGame_Defense__TickWaveNodes : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*),
	0x10ad9ec0,
	TgGame_Defense__TickWaveNodes> {
public:
	// Per-node runtime state the SDK node has no fields for. fMin/fMaxFreq
	// mirror the kismet-linked SeqVar_RandomFloat bounds (equal for a plain
	// SeqVar_Float) so every pulse re-rolls the interval like retail's
	// per-read variable evaluation. nCursor walks `order`, a freshly shuffled
	// permutation of Targets indices rebuilt each cycle — every factory still
	// fires exactly once per cycle (duplicate Targets entries still weight it),
	// but WHICH spot a given pulse uses is random every game instead of always
	// the same Targets order. Keyed by node pointer — kismet objects are stable
	// for the map lifetime and CacheKismetConfiguration clears the map each
	// match.
	struct NodeState {
		float fMinFreq = 0.0f;
		float fMaxFreq = 0.0f;
		int   nCursor  = 0;       // position inside `order`
		int   nCycle   = 0;       // completed shuffles (rng ordinal)
		int   nPulse   = 0;       // pulses fired this round (rng ordinal)
		int   nRound   = -1;      // round the pulse counter belongs to
		std::vector<int> order;   // shuffled Targets indices for the cycle
	};
	static std::map<void*, NodeState> s_nodeState;

	static void ResetNodeState();
	static void SetNodeFrequency(void* Spawner, float fMin, float fMax);

	static void __fastcall Call(ATgGame_Defense* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx) {
		m_original(Game, edx);
	};
};
