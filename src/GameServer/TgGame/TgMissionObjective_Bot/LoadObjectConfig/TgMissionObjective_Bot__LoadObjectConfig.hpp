#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hooks the native override of LoadObjectConfig on the TgMissionObjective_Bot
// subclass. The address 0x10a7c440 was confirmed NOT to be a stub by the
// user — the binary's body does meaningful work (likely populating fields
// from asm tables), so CallOriginal runs first and we layer MapObjectConfig
// overrides on top.
//
// Parent (TgMissionObjective.LoadObjectConfig at 0x10a7bed0) is invoked
// transitively from inside the binary's body. We don't double-call it.
//
// Task force for the spawned bot is NOT a synthetic column — it comes from
// the parent class's nDefaultOwnerTaskForce field (default 2 = Defenders,
// see TgMissionObjective.uc:70 + 440 + 868). To override per objective, the
// user authors a map_object_config row for `n_default_owner_task_force` on
// the parent class's hook (out of scope here — current parent hook only
// writes r_e_status). If that's needed, extend the parent hook in a
// follow-up; for now editor defaults + this hook's other knobs cover the
// common cases.
class TgMissionObjective_Bot__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgMissionObjective_Bot*, void*),
	0x10a7c440,
	TgMissionObjective_Bot__LoadObjectConfig> {
public:
	static void __fastcall Call(ATgMissionObjective_Bot* Obj, void* edx);
	static inline void __fastcall CallOriginal(ATgMissionObjective_Bot* Obj, void* edx) {
		m_original(Obj, edx);
	}
};
