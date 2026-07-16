#pragma once

#include "src/pch.hpp"

// UTgSeqVar_MapGameId::GetRef — third member of the stripped stub cluster
// (vtable 0x116e2828 + 0x16c -> stub 0x109fd020, returns null). The publish
// collector (USequenceOp::GetIntVars, 0x10d091f0) reads each linked var via
// vtable+0x16c and SKIPS null refs, so the CompareInt saw ValueA=0 even with
// Publish/PopulateValue patched (playtest capture 2026-07-16). Intact sibling
// TgSeqVar_RouteNumber (0x109fcd20): refresh IntValue from source, return
// &IntValue. Installed as a direct vtable-slot patch.
class TgSeqVar_MapGameId__GetRef {
public:
	static void Install();
	static int* __fastcall Call(USeqVar_Int* Var, void* edx);
};
