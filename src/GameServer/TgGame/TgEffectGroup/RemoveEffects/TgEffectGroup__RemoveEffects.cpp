#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

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

	Logger::Log("effects",
		"[REMOVE-EFFECTS] eg=%p (egId=%d) target=%p  effects=%d bReset=%d\n",
		(void*)eg, eg->m_nEffectGroupId, (void*)Target, eg->m_Effects.Count, (int)bResetToFollow);

	for (int i = 0; i < eg->m_Effects.Count; ++i) {
		UTgEffect* effect = eg->m_Effects.Data[i];
		if (!effect) continue;

		unsigned int eflags = *(unsigned int*)((char*)effect + 0x48);

		// Skip Remove if Apply never ran. m_fCurrent is set by ApplyEffect as
		// its first statement (TgEffect.uc:116), so 0 here means the Apply was
		// gated out — typically by ProtectionModifier returning 0 in
		// ApplyEventBasedEffects, failing the inner lifetime test. The clone
		// still entered s_AppliedEffectGroups via the m_bIsManaged add-clause,
		// and several UC `event Remove` branches mutate pawn state
		// UNCONDITIONALLY (166/167/187 Stun(false), 100 Unhacked, 254
		// r_bResistTagging, 308 ForceResetTaskForce, 323 yaw-lock). Letting
		// those fire clobbers state that a *different* group's Apply set —
		// e.g. EMP eg 6995 (cat=653 stun) phantom-cloned past prop-235
		// protection, then on the next EMP its RemoveAllEffectGroups path
		// undid the cat=378 stun the new fire had just applied.
		if (effect->m_fCurrent == 0.0f) {
			Logger::Log("effects",
				"[REMOVE-EFFECTS]   [%d] effect=%p propId=%d calc=%d m_fCurrent=0 m_bRemovable=%d -> SKIP (Apply never ran)\n",
				i, (void*)effect,
				effect->m_nPropertyId, effect->m_nCalcMethodCode,
				(eflags & 0x02) ? 1 : 0);
			continue;
		}

		Logger::Log("effects",
			"[REMOVE-EFFECTS]   [%d] effect=%p propId=%d calc=%d m_fCurrent=%.3f m_bRemovable=%d -> eventRemove\n",
			i, (void*)effect,
			effect->m_nPropertyId, effect->m_nCalcMethodCode,
			effect->m_fCurrent, (eflags & 0x02) ? 1 : 0);

		// AirSpeed/FlightAccel diagnostic: log the m_fRaw/Pawn.AirSpeed/
		// Pawn.r_FlightAcceleration triple immediately before AND after the
		// eventRemove fires, when the effect targets prop 70 or 59. SetProperty
		// hook will also emit a [SET-PROP] line for the inner write — these
		// REMOVE-CTX bookends let us see the eventRemove's net effect with
		// the egId/effect-ptr context that [SET-PROP] alone lacks.
		const bool track = (effect->m_nPropertyId == 70 || effect->m_nPropertyId == 59);
		ATgPawn* pawnTarget = nullptr;
		float    rawBefore = -99999.0f, airBefore = -99999.0f, faBefore = -99999.0f;
		if (track && Target) {
			const char* tname = Target->Class ? Target->Class->GetFullName() : nullptr;
			if (tname && strstr(tname, "TgPawn") != nullptr) {
				pawnTarget = (ATgPawn*)Target;
				if (pawnTarget->s_Properties.Data) {
					for (int j = 0; j < pawnTarget->s_Properties.Num(); ++j) {
						UTgProperty* p = pawnTarget->s_Properties.Data[j];
						if (p && p->m_nPropertyId == effect->m_nPropertyId) { rawBefore = p->m_fRaw; break; }
					}
				}
				airBefore = pawnTarget->AirSpeed;
				faBefore  = pawnTarget->r_FlightAcceleration;
				Logger::Log("effects",
					"[REMOVE-CTX] PRE  egId=%d eff=%p prop=%d  m_fRaw=%.4f Pawn.AirSpeed=%.3f Pawn.r_FlightAccel=%.4f\n",
					eg->m_nEffectGroupId, (void*)effect, effect->m_nPropertyId,
					rawBefore, airBefore, faBefore);
			}
		}

		effect->eventRemove(Target, bResetToFollow);

		if (track && pawnTarget) {
			float rawAfter = -99999.0f;
			if (pawnTarget->s_Properties.Data) {
				for (int j = 0; j < pawnTarget->s_Properties.Num(); ++j) {
					UTgProperty* p = pawnTarget->s_Properties.Data[j];
					if (p && p->m_nPropertyId == effect->m_nPropertyId) { rawAfter = p->m_fRaw; break; }
				}
			}
			Logger::Log("effects",
				"[REMOVE-CTX] POST egId=%d eff=%p prop=%d  m_fRaw=%.4f (Δ%+.4f)  Pawn.AirSpeed=%.3f (Δ%+.3f)  Pawn.r_FlightAccel=%.4f (Δ%+.4f)\n",
				eg->m_nEffectGroupId, (void*)effect, effect->m_nPropertyId,
				rawAfter, rawAfter - rawBefore,
				pawnTarget->AirSpeed, pawnTarget->AirSpeed - airBefore,
				pawnTarget->r_FlightAcceleration, pawnTarget->r_FlightAcceleration - faBefore);
		}

		// Stale-pointer cleanup. CLONE effects (not template effects) get
		// marked into BuffEffectRegistry by CloneEffectGroup; without a
		// Forget on remove they accumulate across the session. UE3 GC
		// eventually reclaims the clone's memory, the allocator hands the
		// same address back to a fresh allocation, and `IsBuff()` returns
		// a false positive — see BuffEffectRegistry.hpp for the full story.
		//
		// The asymmetric-remove fix in UObject__ProcessEvent makes false
		// positives benign already, so this is defense-in-depth. Order
		// matters: Forget AFTER eventRemove so the ProcessEvent hook that
		// fires during eventRemove still sees IsBuff()=true and runs the
		// buff-route Remove. Forget'ing before would silently drop
		// legitimate buff cleanup.
		//
		// Templates aren't passed in here (templates aren't reaped via this
		// path), so we don't risk Forget'ing the template by mistake.
		BuffEffectRegistry::Forget(effect);
	}
}
