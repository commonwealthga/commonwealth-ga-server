#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native UTgPlayerCountVolume::Update is a `_notimplemented` stub in this
// binary (0x109aeeb0 is the stub trampoline). UC `Touch` (TgPlayerCountVolume.uc:14)
// filters incoming pawns by `TaskForceNumber` and then calls Update — without
// our reimplementation the Players array never fills and TgSeqEvent_PlayerCountHit
// never fires.
//
// Reimplementation responsibilities:
//   1. Bail if the volume is disabled.
//   2. Dedupe against Players (re-Touch on the same boundary mustn't re-fire).
//   3. Append the pawn to Players.
//   4. If the new count is EXACTLY PlayerCountTarget, fire
//      TgSeqEvent_PlayerCountHit via Actor::TriggerEventClass (the kismet
//      event class is looked up via ClassPreloader) AND broadcast a
//      SendAlert::Broadcast to every connected player using the volume's
//      configured messageID. Crossings past the target don't re-fire.
class TgPlayerCountVolume__Update : public HookBase<
	void(__fastcall*)(ATgPlayerCountVolume*, void*, ATgPawn*),
	0x109aeeb0,
	TgPlayerCountVolume__Update> {
public:
	static void __fastcall Call(ATgPlayerCountVolume* Volume, void* edx, ATgPawn* Other);
	static inline void __fastcall CallOriginal(ATgPlayerCountVolume* Volume, void* edx, ATgPawn* Other) {
		m_original(Volume, edx, Other);
	};
};
