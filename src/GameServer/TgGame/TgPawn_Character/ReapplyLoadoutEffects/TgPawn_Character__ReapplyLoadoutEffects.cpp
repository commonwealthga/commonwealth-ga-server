#include "src/GameServer/TgGame/TgPawn_Character/ReapplyLoadoutEffects/TgPawn_Character__ReapplyLoadoutEffects.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UC `native function ReapplyLoadoutEffects()` (stub @ 0x109c0e10). Called from:
//   1. TgPawn.uc:10269 Dying.EndState (medic / self-revive; same pawn pointer).
//   2. TgGame_Arena.uc:398 round-restart loop.
// Both paths run AFTER the death-time `RemoveAllEffects` sweep (TgPawn.uc:4456
// timer or :4460 direct), which strips every entry in s_AppliedEffectGroups —
// INCLUDING the lifetime=0 m_bIsManaged skill clones RCST stores there. So the
// revived pawn loses its skill buffs (prop 256 accuracy, prop 214 HP, …) until
// the next respec save. Fix: clear residual transients via RemoveAllEffects,
// then call ReapplyCharacterSkillTree (idempotent — reverts then reapplies).
// Original is the stripped `_notimplemented` stub; CallOriginal is a no-op.
void __fastcall TgPawn_Character__ReapplyLoadoutEffects::Call(ATgPawn_Character* Pawn, void* /*edx*/) {
	LogCallBegin();
	if (!Pawn) { LogCallEnd(); return; }

	ATgEffectManager* Mgr = Pawn->r_EffectManager;
	if (Mgr) {
		Logger::Log("effects",
			"[REAPPLY-LOADOUT] pawn=%p clearing transient effects (applied=%d)\n",
			(void*)Pawn, Mgr->s_AppliedEffectGroups.Count);
		TgEffectManager__RemoveAllEffects::Call(Mgr, nullptr, nullptr);
	}

	// Reapply skill-tree effects: death's RemoveAllEffects stripped them.
	TgPawn_Character__ReapplyCharacterSkillTree::Call(Pawn, nullptr);

	LogCallEnd();
}
