#pragma once

// DeviceCategorySkill — maps a device id to its "classifying skill id".
//
// Multi-row skills (e.g. Jetpack Power skill 526) gate each effect_group on
// `asm_data_set_effect_groups.required_skill_id`, where the value is the
// category skill id of the device the effect should apply to. The four
// jetpack rows have required_skill_id = 358 (Assault), 359 (Recon),
// 360 (Medic), 361 (Robotic). When the player fires a Medic Jetpack
// (device id 7032), the buff query needs to pass nReqSkillId=360 so the
// registry's match rule (FUN_109cd890: stored>0 must equal query) picks
// up the row for that variant only.
//
// The mapping lives in `asm_data_set_items` — every item where
// `item_id == device_id` carries the device's classifying skill_id.
// Examples (verified against the shipped DB):
//   item_id=7032 "Medic Crescent Jetpack"      → skill_id=360
//   item_id=1987 "iMINIGUN" (assault gun)      → skill_id=276
//   item_id=2914 "Inferno-X Cannon"            → skill_id=276
//   item_id=4166 "Heatwrack M.A.S.E.R."        → skill_id=276
//
// Returns 0 (wildcard) when the device id is unknown or has no
// category skill (e.g., devices like HUMAN BASE ATTRIBUTES that
// aren't player-controlled). Wildcard means "no constraint" on the
// query side — consistent with how skill-source entries with
// required_skill_id=0 stay universal.
class ATgPawn;

namespace DeviceCategorySkill {
	// Direct lookup by device id.
	int Lookup(int deviceId);

	// Convenience: given a pawn and a device-instance id (`m_nSourceDeviceInstId`
	// on a cloned effect group), walks the pawn's tracked equipped devices to
	// find the matching deviceId, then returns its category skillId. Returns
	// 0 (wildcard) when the instId isn't tracked or has no skill mapping.
	// Used by CloneEffectGroup [DAMAGE-BUFF] / [EFFECT-BUFF] where the clone
	// only carries the instId, not the deviceId.
	int LookupByInstanceId(ATgPawn* pawn, int deviceInstanceId);
}
