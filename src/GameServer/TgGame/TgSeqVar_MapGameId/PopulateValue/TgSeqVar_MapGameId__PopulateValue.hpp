#pragma once

#include "src/pch.hpp"

// UTgSeqVar_MapGameId::PopulateValue — sibling of the PublishValue stub
// (vtable 0x116e2828 + 0x1a0 -> stripped stub 0x109fd040). Mirrors the
// intact TgSeqVar_RouteNumber shape (0x109fcd90): refresh IntValue, then
// tail-call USeqVar_Int::PopulateValue (0x10d0c890). Installed as a direct
// vtable-slot patch — see TgSeqVar_MapGameId__PublishValue.
class TgSeqVar_MapGameId__PopulateValue {
public:
	static void Install();
	static void __fastcall Call(USeqVar_Int* Var, void* edx, USequenceOp* Op, UProperty* Prop, void* VarLink);
};
