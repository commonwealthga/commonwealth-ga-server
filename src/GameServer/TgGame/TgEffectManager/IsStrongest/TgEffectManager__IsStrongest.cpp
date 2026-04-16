#include "src/GameServer/TgGame/TgEffectManager/IsStrongest/TgEffectManager__IsStrongest.hpp"
#include "src/Utils/Logger/Logger.hpp"

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
// When no existing same-category effect is present, eg is automatically
// strongest.
bool __fastcall TgEffectManager__IsStrongest::Call(ATgEffectManager* Manager, void* /*edx*/, UTgEffectGroup* eg, unsigned long bConsiderLifetime) {
	if (!Manager || !eg) return false;

	const int egCategory   = eg->m_nCategoryCode;
	const float egValue    = eg->m_fApplicationValue;
	const float egLifeTime = eg->m_fLifeTime;

	bool foundSameCategory = false;

	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* existing = Manager->s_AppliedEffectGroups.Data[i];
		if (!existing) continue;
		if (existing->m_nCategoryCode != egCategory) continue;

		foundSameCategory = true;

		// If existing has strictly greater application value, eg is NOT strongest.
		if (existing->m_fApplicationValue > egValue) return false;

		// Tie-breaker on lifetime, when requested.
		if (bConsiderLifetime
			&& existing->m_fApplicationValue == egValue
			&& existing->m_fLifeTime > egLifeTime) {
			return false;
		}
	}

	// Either no existing effect of this category, or eg matches/exceeds every
	// one. In both cases eg is at least as strong as anything present, so let
	// the caller proceed with cloning and replacing.
	(void)foundSameCategory;
	return true;
}
