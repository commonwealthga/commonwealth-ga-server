#include "src/GameServer/TgGame/TgSeqVar_MapGameId/GetRef/TgSeqVar_MapGameId__GetRef.hpp"
#include "src/GameServer/TgGame/TgSeqVar_MapGameId/PublishValue/TgSeqVar_MapGameId__PublishValue.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
void** const kVtableSlot   = reinterpret_cast<void**>(0x116e2828 + 0x16c);
void*  const kStrippedStub = reinterpret_cast<void*>(0x109fd020);
}

void TgSeqVar_MapGameId__GetRef::Install() {
	if (*kVtableSlot != kStrippedStub) {
		Logger::Log("mapgameid", "GetRef install SKIPPED: slot holds %p, expected stub %p\n",
			*kVtableSlot, kStrippedStub);
		return;
	}
	DWORD oldProtect = 0;
	VirtualProtect(kVtableSlot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*kVtableSlot = reinterpret_cast<void*>(&Call);
	VirtualProtect(kVtableSlot, sizeof(void*), oldProtect, &oldProtect);
	Logger::Log("mapgameid", "GetRef vtable slot patched\n");
}

int* __fastcall TgSeqVar_MapGameId__GetRef::Call(USeqVar_Int* Var, void* edx) {
	Var->IntValue = TgSeqVar_MapGameId__PublishValue::ResolveCurrentMapGameId();
	return &Var->IntValue;
}
