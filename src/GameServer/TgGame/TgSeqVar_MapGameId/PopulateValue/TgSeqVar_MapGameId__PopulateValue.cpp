#include "src/GameServer/TgGame/TgSeqVar_MapGameId/PopulateValue/TgSeqVar_MapGameId__PopulateValue.hpp"
#include "src/GameServer/TgGame/TgSeqVar_MapGameId/PublishValue/TgSeqVar_MapGameId__PublishValue.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
void** const kVtableSlot   = reinterpret_cast<void**>(0x116e2828 + 0x1a0);
void*  const kStrippedStub = reinterpret_cast<void*>(0x109fd040);
// Intact base USeqVar_Int::PopulateValue (thiscall, 3 stack args, ret 0xC).
using BaseFn = void(__fastcall*)(USeqVar_Int*, void*, USequenceOp*, UProperty*, void*);
BaseFn const kBasePopulate = reinterpret_cast<BaseFn>(0x10d0c890);
}

void TgSeqVar_MapGameId__PopulateValue::Install() {
	if (*kVtableSlot != kStrippedStub) {
		Logger::Log("mapgameid", "PopulateValue install SKIPPED: slot holds %p, expected stub %p\n",
			*kVtableSlot, kStrippedStub);
		return;
	}
	DWORD oldProtect = 0;
	VirtualProtect(kVtableSlot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*kVtableSlot = reinterpret_cast<void*>(&Call);
	VirtualProtect(kVtableSlot, sizeof(void*), oldProtect, &oldProtect);
	Logger::Log("mapgameid", "PopulateValue vtable slot patched\n");
}

void __fastcall TgSeqVar_MapGameId__PopulateValue::Call(USeqVar_Int* Var, void* edx, USequenceOp* Op, UProperty* Prop, void* VarLink) {
	Var->IntValue = TgSeqVar_MapGameId__PublishValue::ResolveCurrentMapGameId();

	if (Logger::IsChannelEnabled("mapgameid") && Op) {
		const char* raw = ((UObject*)Op)->GetFullName();
		const std::string opName(raw ? raw : "<null>");
		Logger::Log("mapgameid", "PopulateValue: %d -> %s\n", Var->IntValue, opName.c_str());
	}

	kBasePopulate(Var, nullptr, Op, Prop, VarLink);
}
