#pragma once

#include "src/pch.hpp"

// Constructs a UTgEffectGroup (with UTgEffect children) populated from
// asm_data_set_effect_groups + asm_data_set_effects. Shared by
// TgDeviceFire::GetEffectGroup and TgPawn_Character::ReapplyCharacterSkillTree.
// Implementation lives in TgDeviceFire__GetEffectGroup.cpp; declared here so
// both hooks can call it without forward-declaring privately.
UTgEffectGroup* BuildEffectGroup(int egId, int egType);
