#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Stripped native `Function TgGame.TgMissionObjective.UpdateMatineeNodeStatus`
// at 0x10a796e0 (`RET 0x4` stub). Called from `TgMissionObjective.uc:508`
// inside UpdateObjectiveStatus whenever r_eStatus transitions. The original
// implementation drove the linked TgSeqEvent_ObjectiveMatineeCommander Kismet
// event by firing one of its output pins (Play / Reverse / Stop / Pause /
// ChangeDir) depending on the new status — that's what tells the Matinee
// timeline to actually run for escort-style objectives, dragging the payload
// (TgObjectiveAttachActor, PHYS_Interpolating) along its track.
//
// Without this hook, the capture math still progresses (m_fCurrCaptureTime
// ticks up, status reaches ATTACKER_INPROGRESS) but the Matinee never gets a
// "Play" command, bIsPlaying stays false, and the payload sits at Position=0
// forever even though SetMatineePlayRate is being called every tick with the
// right rate.
class TgMissionObjective__UpdateMatineeNodeStatus : public HookBase<
	void(__fastcall*)(ATgMissionObjective*, void*, unsigned char),
	0x10a796e0,
	TgMissionObjective__UpdateMatineeNodeStatus> {
public:
	static void __fastcall Call(ATgMissionObjective* Obj, void* edx, unsigned char eStatus);
	static inline void __fastcall CallOriginal(ATgMissionObjective* Obj, void* edx, unsigned char eStatus) {
		m_original(Obj, edx, eStatus);
	}
};
