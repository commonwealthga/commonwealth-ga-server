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
// sTypeCode is passed by reference (original does ret 0xc).
class TgGame__ActivateAlarm : public HookBase<
	void(__fastcall*)(ATgGame*, void*, AActor*, int, FString*),
	0x10a75740,
	TgGame__ActivateAlarm> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx,
		AActor* Originator, int nGlobalAlarmId, FString* sTypeCode);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx,
		AActor* Originator, int nGlobalAlarmId, FString* sTypeCode) {
		m_original(Game, edx, Originator, nGlobalAlarmId, sTypeCode);
	}
};
