#pragma once

#include "src/pch.hpp"

// UTgSeqVar_MapGameId::PublishValue — the override slot (vtable 0x116e2828
// + 0x19c) points at a stripped stub (0x109fd030), so the var never
// resolves and every kismet "MapGameId == N" SeqCond_CompareInt stays
// false (e.g. 1P_SDColony06_P's Quest-Version side rooms). The intact
// sibling TgSeqVar_RouteNumber (0x109fcd50) shows the retail shape:
// refresh IntValue from the game, then tail-call USeqVar_Int::PublishValue
// (0x10d0c630). We refresh from map_game_info by map name + game class.
//
// Installed as a direct vtable-slot patch, NOT a Detours hook: the stub is
// a bare `ret 0xC` (3 bytes) — below the 5 bytes Detours needs — and
// HookBase::Install ignores DetourAttach failures silently
// (UdpNetDriver__TickDispatch precedent for slot patching).
class TgSeqVar_MapGameId__PublishValue {
public:
	static void Install();
	// Cached once per process (one map per instance).
	static int ResolveCurrentMapGameId();
	static void __fastcall Call(USeqVar_Int* Var, void* edx, USequenceOp* Op, UProperty* Prop, void* VarLink);
};
