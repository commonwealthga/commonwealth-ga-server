#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"

// Walks m_Effects and ProcessEvents the UC `Remove` event on each one.
// SDK's UTgEffect::eventRemove builds the parm struct (Target +
// bResetToFollow:1) and ProcessEvents into the UC function — `TgEffect.Remove`
// is a UC event (FUNC_Event), not native, so this reaches the bytecode that
// reverses the property modifier.
//
// Target may be null if the caller doesn't have a handy reference (e.g.
// effect-group whose m_Target hasn't been wired). UC `Remove` early-returns on
// !m_bRemovable / bResetToFollow before touching Target, so passing null is
// safe — but `ApplyToProperty` later in that UC path needs a valid Target to
// reach `GetProperty`, so a null target effectively skips the reversal.
void __fastcall TgEffectGroup__RemoveEffects::Call(UTgEffectGroup* eg, void* /*edx*/, AActor* Target, unsigned long bResetToFollow) {
	if (!eg) return;

	for (int i = 0; i < eg->m_Effects.Count; ++i) {
		UTgEffect* effect = eg->m_Effects.Data[i];
		if (effect) {
			effect->eventRemove(Target, bResetToFollow);
		}
	}
}
