#pragma once

// Modifier-prop tables and helpers — co-located source of truth for both:
//   * IsModifierProp(pid)        — used by ReapplyCharacterSkillTree to decide
//                                  whether a skill effect should land in the
//                                  buff registry (TgPawn::ApplyBuff) instead
//                                  of being written directly to s_Properties.
//   * kBuffDeviceModMap          — used by ApplyPlayerModsToDeployable at
//                                  deploy time to bake the player's buff
//                                  registry entries into the deployable's
//                                  s_Properties.
//
// Both are derived from the same engine function: the binary's
// `ConvertPropToPropList` (FUN_109e5220 @ 0x109e5220). That function takes
// (context, propId) and emits a list of "modifier" propIds the buff
// aggregator iterates through. Modifier props are propIds that can appear in
// m_EffectBuffInfo entries; base stats (like prop 49 GroundSpeed, prop 51
// Health, prop 217-219 Protection) are NOT modifier props — they're written
// directly to s_Properties.
//
// Re-extract from the binary if it changes:
//   mcp__ghidra__decompile_function_by_address(0x109e5220)
namespace ModifierProps {

// True if `pid` can serve as an FBuffInfo entry's nPropId — i.e. is in the
// OUTPUT set of FUN_109e5220 across any of its four contexts (BUFF_PAWN=1,
// ctx 2, BUFF_DEVICE=3, BUFF_OTHER=4). Skill effects targeting these route
// through the buff registry.
bool IsModifierProp(int pid);

// Inverse of ConvertPropToPropList(BUFF_DEVICE, *) — for each modifier propId
// (key into the player's m_EffectBuffInfo), the list of deployable-side base
// props it scales when the player deploys a station / turret / bomb.
//
// Used at deploy time to bake the player's registered buffs into the
// deployable's s_Properties. Only includes BUFF_DEVICE inversions; BUFF_PAWN
// expansions live on the pawn's effect-application path (CheckEffectBuffModifier
// equivalent in CloneEffectGroup) and don't apply to deployable s_Properties.
struct BuffDeviceMapping {
	int modifierPropId;
	int targets[5];   // -1 sentinel terminator
};

extern const BuffDeviceMapping kBuffDeviceModMap[];
extern const int               kBuffDeviceModMapCount;

}  // namespace ModifierProps
