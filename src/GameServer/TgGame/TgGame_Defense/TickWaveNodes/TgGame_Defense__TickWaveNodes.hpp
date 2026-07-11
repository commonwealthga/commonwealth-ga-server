#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

#include <map>

class TgGame_Defense__TickWaveNodes : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*),
	0x10ad9ec0,
	TgGame_Defense__TickWaveNodes> {
public:
	// Per-node runtime state the SDK node has no fields for. fMin/fMaxFreq
	// mirror the kismet-linked SeqVar_RandomFloat bounds (equal for a plain
	// SeqVar_Float) so every pulse re-rolls the interval like retail's
	// per-read variable evaluation; nCursor is the round-robin position in
	// Targets. Keyed by node pointer — kismet objects are stable for the map
	// lifetime and CacheKismetConfiguration clears the map each match.
	struct NodeState {
		float fMinFreq = 0.0f;
		float fMaxFreq = 0.0f;
		int   nCursor  = 0;
	};
	static std::map<void*, NodeState> s_nodeState;

	static void ResetNodeState();
	static void SetNodeFrequency(void* Spawner, float fMin, float fMax);

	static void __fastcall Call(ATgGame_Defense* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx) {
		m_original(Game, edx);
	};
};
