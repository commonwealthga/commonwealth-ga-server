#include "src/GameServer/TgGame/_effect_core/HitSituationalMitigation.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

namespace {

// Every property `TgEffectDamage.ProtectionModifier` can consume for an
// impact, read straight off the three switches in TgEffectGroup.uc:
//   CalcCategoryProtection   (608) — by m_nCategoryCode
//   CalcDamageTypeProtection (673) — by m_nDamageType
//   CalcAttackTypeProtection (715) — by m_eAttackType, category 302/963 only
// Anything outside this set cannot change a mitigation result, so a 505 debuff
// on it is left strictly alone.
bool IsMitigationProp(int propId) {
	switch (propId) {
		// Category axis
		case 159: case 158: case 160: case 163: case 168:
		case 233: case 235: case 266: case 371: case 328:
		// Damage-type axis
		case 155: case 157: case 156: case 324:
		// Attack-type axis
		case 217: case 218: case 219:
			return true;
		default:
			return false;
	}
}

// A protection write made by this impact's own 505 pass, captured before it
// landed. Keyed by (victim, instigator): both the debuff group and the damage
// group get `m_Instigator` from the same `SubmitEffect` call
// (TgDeviceFire.uc:400-408 — `m_Owner.Instigator`), so equality means "same
// shooter, same victim".
//
// NOTE: deliberately NOT keyed on m_nSourceDeviceInstId. A skill-sourced group
// resolves that differently from the device's damage group, and that asymmetry
// is what scopes Eagle Eye's potency to weapon debuffs only — confirmed
// intended behaviour we must not disturb.
struct PendingDebuff {
	ATgPawn* victim;
	AActor*  instigator;
	int      propId;
	float    preRaw;
};

// A property currently lent back to its pre-debuff value, with the delta owed.
struct ActiveSwap {
	UTgProperty* prop;
	float        delta;   // what the 505 pass had applied; re-added on close
};

std::vector<PendingDebuff> g_pending;
std::vector<ActiveSwap>    g_active;

ATgPawn* AsPawn(AActor* actor) {
	if (!actor) return nullptr;
	if (!ObjectClassCache::ClassNameContains(actor, "TgPawn")) return nullptr;
	return static_cast<ATgPawn*>(actor);
}

}  // namespace

namespace HitSituationalMitigation {

void NoteDebuffApplied(UTgEffect* effect) {
	if (!effect) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	// Only the HP-gated situational pass runs ahead of the damage submit.
	if (g->m_nType != 505) return;
	if (g->m_nSituationalType != 1270 && g->m_nSituationalType != 1271) return;
	if (!IsMitigationProp(effect->m_nPropertyId)) return;

	// Pawn victims only: the closing bracket (TgEffectDamage.uc:206) is inside
	// `if(bIsPawnTarget)`, so arming for a deployable would leave the swap open.
	ATgPawn* victim = AsPawn(g->m_Target);
	if (!victim) return;

	UTgProperty* prop = TgPawn__GetProperty::CallOriginal(victim, nullptr, effect->m_nPropertyId);
	if (!prop) return;

	// Two 505 groups hitting the same prop on one impact: keep the earliest
	// capture, which is the true pre-impact value.
	for (const PendingDebuff& rec : g_pending) {
		if (rec.victim == victim && rec.propId == effect->m_nPropertyId) return;
	}

	g_pending.push_back(PendingDebuff{ victim, g->m_Instigator,
	                                   effect->m_nPropertyId, prop->m_fRaw });

	if (Logger::IsChannelEnabled("ki")) {
		Logger::Log("ki",
			"[NOTE] egId=%d type=505 sit=%d prop=%d victim=%p inst=%p preRaw=%.3f\n",
			g->m_nEffectGroupId, g->m_nSituationalType, effect->m_nPropertyId,
			(void*)victim, (void*)g->m_Instigator, prop->m_fRaw);
	}
}

void BeginImpactMitigation(UTgEffect* damageEffect) {
	if (!damageEffect) return;
	UTgEffectGroup* g = damageEffect->m_EffectGroup;
	if (!g) return;

	// A window left open would be a bug in the bracketing, not something to
	// paper over — close it before opening a new one so a property can never
	// be double-lent.
	if (!g_active.empty()) {
		Logger::Log("ki",
			"[BEGIN] window still open with %d entries — closing before reopen\n",
			(int)g_active.size());
		for (ActiveSwap& s : g_active) {
			if (s.prop) s.prop->m_fRaw += s.delta;
		}
		g_active.clear();
	}

	ATgPawn* victim = AsPawn(g->m_Target);
	if (victim) {
		for (const PendingDebuff& rec : g_pending) {
			if (rec.victim != victim || rec.instigator != g->m_Instigator) continue;

			UTgProperty* prop = TgPawn__GetProperty::CallOriginal(victim, nullptr, rec.propId);
			if (!prop) continue;

			// Store the delta rather than the absolute value so anything else
			// that writes this property inside the window (a shield breaking
			// mid-mitigation, say) composes correctly on close.
			const float delta = prop->m_fRaw - rec.preRaw;
			prop->m_fRaw = rec.preRaw;
			g_active.push_back(ActiveSwap{ prop, delta });

			if (Logger::IsChannelEnabled("ki")) {
				Logger::Log("ki",
					"[BEGIN] egId=%d victim=%p prop=%d  m_fRaw %.3f -> %.3f (delta %.3f held)\n",
					g->m_nEffectGroupId, (void*)victim, rec.propId,
					rec.preRaw + delta, rec.preRaw, delta);
			}
		}
	}

	// Records are consumed by the first damage application that follows them.
	// Clearing unconditionally bounds how long a record can survive an impact
	// that never dealt damage — it cannot reach a later, unrelated shot.
	g_pending.clear();
}

void EndImpactMitigation(ATgEffectManager* Manager) {
	if (g_active.empty()) return;

	for (ActiveSwap& s : g_active) {
		if (!s.prop) continue;
		s.prop->m_fRaw += s.delta;
		if (Logger::IsChannelEnabled("ki")) {
			Logger::Log("ki",
				"[END] mgr=%p prop restored to %.3f (delta %.3f re-applied)\n",
				(void*)Manager, s.prop->m_fRaw, s.delta);
		}
	}
	g_active.clear();
}

}  // namespace HitSituationalMitigation
