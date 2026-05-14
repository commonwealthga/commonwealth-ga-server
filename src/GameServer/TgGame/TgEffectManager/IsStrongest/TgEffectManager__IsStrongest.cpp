#include "src/GameServer/TgGame/TgEffectManager/IsStrongest/TgEffectManager__IsStrongest.hpp"

// UTgEffectManager::IsStrongest — original is an empty stub returning 0.
//
// UC declaration:
//   native function bool IsStrongest(TgEffectGroup eg, bool bConsiderLifetime);
//
// Caller (TgEffectManager.uc::GetNewEffectGroupByApp case 157):
//   if(IsStrongest(eg, true)) { RemoveAllEffectGroups(eg); newGrp = GetClonedEffectGroup(eg); }
//
// Semantic: "is `eg` stronger than every existing effect group with the same
// category currently applied?". If true, the caller wipes existing same-
// category effects and replaces them with a clone of eg. If false, eg is
// dropped entirely (newGrp stays none) and ProcessEffect bails — which is the
// bug the user is hitting: the empty stub returns 0, so case-157 effects
// (which include healing/poison grenades and the rest device) never apply
// their persistent component.
//
// Implementation walks s_AppliedEffectGroups (TArray @ TgEffectManager+0x468)
// and compares m_fApplicationValue (and m_fLifeTime when bConsiderLifetime).
// When no existing same-bucket effect is present, eg is automatically
// strongest.
//
// Category-302 ("Local") special case: cat-302 is a sentinel used by every
// other displacement path in TgEffectManager.uc — see GetStackingEffectGroup
// (uc:464) and GetRefreshedEffectGroup (uc:504): for cat-302 the bucket is
// scoped by m_nEffectGroupId, NOT category. Without this scope, ANY cat-302
// + app-157 effect (Strongest Wins) would wipe every other cat-302 effect
// in s_AppliedEffectGroups — including the jetpack's per-pulse +AirSpeed /
// +FlightAccel effects (egs 10450/52/54/56). Dropping FlightAccel triggers
// the client's r_FlightAcceleration ReplicatedEvent → SetPhysics(2) →
// player falls out of the air mid-flight while jetpack VFX/SFX/power-drain
// continue (the jetpack device's UC state has no idea its effects were
// displaced). Concrete trigger: Targeting System (eg 24240) and Inferno-X
// Cannon Mk III (eg 9360) — the only two devices with cat-302 + app-157 +
// type-263 in the DB.
bool __fastcall TgEffectManager__IsStrongest::Call(ATgEffectManager* Manager, void* /*edx*/, UTgEffectGroup* eg, unsigned long bConsiderLifetime) {
	if (!Manager || !eg) return false;

	const int egCategory   = eg->m_nCategoryCode;
	const int egGroupId    = eg->m_nEffectGroupId;
	const float egValue    = eg->m_fApplicationValue;
	const float egLifeTime = eg->m_fLifeTime;
	const bool isLocal     = (egCategory == 302);

	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* existing = Manager->s_AppliedEffectGroups.Data[i];
		if (!existing) continue;

		if (isLocal) {
			if (existing->m_nEffectGroupId != egGroupId) continue;
		} else {
			if (existing->m_nCategoryCode != egCategory) continue;
		}

		// If existing has strictly greater application value, eg is NOT strongest.
		if (existing->m_fApplicationValue > egValue) return false;

		// Tie-breaker on lifetime, when requested.
		if (bConsiderLifetime
			&& existing->m_fApplicationValue == egValue
			&& existing->m_fLifeTime > egLifeTime) {
			return false;
		}
	}

	// Either no existing effect in this bucket, or eg matches/exceeds every
	// one. In both cases eg is at least as strong as anything present, so let
	// the caller proceed with cloning and replacing.
	return true;
}
