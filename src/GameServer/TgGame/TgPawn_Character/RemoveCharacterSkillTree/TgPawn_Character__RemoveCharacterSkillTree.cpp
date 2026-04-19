#include "src/GameServer/TgGame/TgPawn_Character/RemoveCharacterSkillTree/TgPawn_Character__RemoveCharacterSkillTree.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgPawn_Character::RemoveCharacterSkillTree — original is _notimplemented
// at 0x109c0e00. Called on respawn / respec / class-change to strip any
// active skill-based effect groups before Reapply builds a fresh set.
//
// The skill-based list is separate from the general s_AppliedEffectGroups
// list (`s_SkillBasedEffectGroups` at TgEffectManager+0x474). Clearing it
// prevents duplicate entries and stale state; the individual effects that
// have already been *applied* to the pawn (health/armor mods via
// ApplyEventBasedEffects) will get re-applied on the next Reapply pass.
void __fastcall TgPawn_Character__RemoveCharacterSkillTree::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();

	if (Pawn && Pawn->r_EffectManager) {
		Pawn->r_EffectManager->ClearSkillBasedEffectGroups();
		Logger::Log("skills", "[Remove] cleared skill-based effect groups on pawn=%p\n", Pawn);
	}

	LogCallEnd();
}
