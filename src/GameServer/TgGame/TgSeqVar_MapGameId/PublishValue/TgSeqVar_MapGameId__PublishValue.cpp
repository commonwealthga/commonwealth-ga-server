#include "src/GameServer/TgGame/TgSeqVar_MapGameId/PublishValue/TgSeqVar_MapGameId__PublishValue.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
void** const kVtableSlot    = reinterpret_cast<void**>(0x116e2828 + 0x19c);
void*  const kStrippedStub  = reinterpret_cast<void*>(0x109fd030);
// Intact base USeqVar_Int::PublishValue (thiscall, 3 stack args, ret 0xC).
using BaseFn = void(__fastcall*)(USeqVar_Int*, void*, USequenceOp*, UProperty*, void*);
BaseFn const kBasePublish = reinterpret_cast<BaseFn>(0x10d0c630);
}

void TgSeqVar_MapGameId__PublishValue::Install() {
	if (*kVtableSlot != kStrippedStub) {
		Logger::Log("mapgameid", "PublishValue install SKIPPED: slot holds %p, expected stub %p\n",
			*kVtableSlot, kStrippedStub);
		return;
	}
	DWORD oldProtect = 0;
	VirtualProtect(kVtableSlot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*kVtableSlot = reinterpret_cast<void*>(&Call);
	VirtualProtect(kVtableSlot, sizeof(void*), oldProtect, &oldProtect);
	Logger::Log("mapgameid", "PublishValue vtable slot patched\n");
}

int TgSeqVar_MapGameId__PublishValue::ResolveCurrentMapGameId() {
	static int  cached   = 0;
	static bool resolved = false;
	if (resolved) return cached;

	std::string gameClass;
	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo && WorldInfo->Game && WorldInfo->Game->Class) {
		const char* raw = ((UObject*)WorldInfo->Game->Class)->GetFullName();
		const std::string full(raw ? raw : "");
		gameClass = (full.rfind("Class ", 0) == 0) ? full.substr(6) : full;
	}

	const std::string mapName = Config::GetMapNameChar();
	if (auto row = MapGameInfo::LookupByNameAndGameMode(mapName, gameClass)) {
		cached = row->map_game_id;
	}
	resolved = true;
	Logger::Log("mapgameid", "TgSeqVar_MapGameId: resolved %d (map=%s class=%s)\n",
		cached, mapName.c_str(), gameClass.c_str());
	return cached;
}

void __fastcall TgSeqVar_MapGameId__PublishValue::Call(USeqVar_Int* Var, void* edx, USequenceOp* Op, UProperty* Prop, void* VarLink) {
	Var->IntValue = ResolveCurrentMapGameId();

	if (Logger::IsChannelEnabled("mapgameid") && Op) {
		const char* raw = ((UObject*)Op)->GetFullName();
		const std::string opName(raw ? raw : "<null>");
		Logger::Log("mapgameid", "PublishValue: %d -> %s\n", Var->IntValue, opName.c_str());
	}

	kBasePublish(Var, nullptr, Op, Prop, VarLink);
}
