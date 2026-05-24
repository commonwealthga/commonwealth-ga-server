#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// `Function TgGame.TgGame.ActivateAlarm` — intact native at 0x10a75740. UC
// signature: native function ActivateAlarm(Actor Originator, int nGlobalAlarmId,
//                                          optional string sTypeCode = "a");
// Per a Ghidra peek, the body iterates a TgBotFactory array (likely
// s_ActorFactories). This is a diagnostic-only hook: CallOriginal first so
// the native does its work, then we log what was passed so we can correlate
// "alarm fired" with "factory spawned" in the per-factory SpawnNextBot logs.
//
// FString is declared as 3 ints so the by-value pass-through on the stack
// matches MSVC __thiscall layout without needing FString copy semantics.
class TgGame__ActivateAlarm : public HookBase<
	void(__fastcall*)(ATgGame*, void*, AActor*, int, int, int, int),
	0x10a75740,
	TgGame__ActivateAlarm> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx,
		AActor* Originator, int nGlobalAlarmId,
		int sTypeCode_Data, int sTypeCode_Count, int sTypeCode_Max);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx,
		AActor* Originator, int nGlobalAlarmId,
		int sTypeCode_Data, int sTypeCode_Count, int sTypeCode_Max) {
		m_original(Game, edx, Originator, nGlobalAlarmId,
			sTypeCode_Data, sTypeCode_Count, sTypeCode_Max);
	}
};
