#include "src/GameServer/TgGame/TgEffectManager/SubmitMitigationDamage/TgEffectManager__SubmitMitigationDamage.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string.h>

// Reimplements the stripped UC native
//   `native function SubmitMitigationDamage(int nProtectionType, int nDamage)`
//   on TgEffectManager. Original entry point is an empty stub at 0x10a70f50.
//
// Call site: `TgEffectGroup.CalcProtection` (TgEffectGroup.uc:773-781) on the
// hit target's r_EffectManager, fired whenever damage is reduced by a
// protection prop and bSubmitMitigation is true. The only path that sets
// bSubmitMitigation=true is `TgEffectDamage.ProtectionModifier:165` (the
// regular damage pipeline). The arguments encode "this much damage was just
// absorbed by `nProtectionType`".
//
// Without this, finite-HP shields (e.g. AOE Shield: effect group 5835 with
// m_nHealth=2000, prop 219) only ever expire on their lifetime timer (10s),
// silently dropping the "or 2000 damage absorbed, whichever comes first"
// half of the contract.
//
// UC's `TgEffectManager.ProcessEffect:241-245` already publishes
// r_nShieldHealthMax / r_nShieldHealthRemaining at apply time so enemies see
// the bar; this hook decrements r_nShieldHealthRemaining per absorbed hit
// and drops the effect group via the normal remove pipeline when the bar
// reaches zero (shield-break).
void __fastcall TgEffectManager__SubmitMitigationDamage::Call(
	ATgEffectManager* Manager, void* /*edx*/,
	int nProtectionType, int nDamage)
{
	if (!Manager || nDamage <= 0) return;

	// Find the shield: a group in s_AppliedEffectGroups with m_nHealth > 0 (the
	// buffed pool stamped at apply time) AND containing an effect targeting the
	// protection prop that absorbed the damage. UC's apply path scopes shield
	// HP to a single active group at a time — first match is the active shield.
	UTgEffectGroup* shield = nullptr;
	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
		if (!g || g->m_nHealth <= 0) continue;
		bool matches = false;
		for (int j = 0; j < g->m_Effects.Count; j++) {
			UTgEffect* e = g->m_Effects.Data[j];
			if (e && e->m_nPropertyId == nProtectionType) { matches = true; break; }
		}
		if (matches) { shield = g; break; }
	}
	if (!shield) {
		Logger::Log("effects",
			"[SHIELD-MIT] mgr=%p prop=%d dmg=%d  no matching shield in applied list (%d entries)\n",
			(void*)Manager, nProtectionType, nDamage, Manager->s_AppliedEffectGroups.Count);
		return;
	}

	int before = shield->m_nHealthMitigated;
	shield->m_nHealthMitigated -= nDamage;
	if (shield->m_nHealthMitigated < 0) shield->m_nHealthMitigated = 0;

	// Sync to the owner's replicated bar — only TgPawn carries the field pair
	// r_nShieldHealthMax (+0x155C) / r_nShieldHealthRemaining (+0x1560).
	// TgDeployable can also receive shielding (TgEffectGroup.uc:777-781) but
	// has no equivalent replicated bar; the per-deployable HUD comes from
	// r_nHealth on the deployable itself.
	AActor* owner = Manager->r_Owner;
	bool ownerIsPawn = false;
	if (owner && owner->Class) {
		const char* cn = owner->Class->GetFullName();
		ownerIsPawn = (cn && strncmp(cn, "Class TgGame.TgPawn", 19) == 0);
	}
	if (ownerIsPawn) {
		ATgPawn* pawn = (ATgPawn*)owner;
		pawn->r_nShieldHealthRemaining = shield->m_nHealthMitigated;
		pawn->bNetDirty = 1;
		pawn->bForceNetUpdate = 1;
	}

	Logger::Log("effects",
		"[SHIELD-MIT] mgr=%p egId=%d prop=%d dmg=%d  m_nHealthMitigated %d -> %d\n",
		(void*)Manager, shield->m_nEffectGroupId, nProtectionType, nDamage,
		before, shield->m_nHealthMitigated);

	// Shield-break: pool drained, drop the effect group. Our reimplemented
	// RemoveEffectGroup reverses any modifiers the group's effects installed
	// (so prop 219 m_fRaw drops back to 0 — subsequent AoE hits go through
	// unmitigated), cancels the LifeDone timer, frees the HUD slot, and
	// swap-removes from s_AppliedEffectGroups.
	if (shield->m_nHealthMitigated <= 0) {
		Logger::Log("effects",
			"[SHIELD-MIT] mgr=%p egId=%d shield broken — removing group\n",
			(void*)Manager, shield->m_nEffectGroupId);
		TgEffectManager__RemoveEffectGroup::Call(Manager, nullptr, shield);

		// Clear the published bar so the client HUD's shield indicator
		// disappears. UC apply rewrites these on the next shield, so we don't
		// need to reset on every mitigation — just on shield-break.
		if (ownerIsPawn) {
			ATgPawn* pawn = (ATgPawn*)owner;
			pawn->r_nShieldHealthMax = 0;
			pawn->r_nShieldHealthRemaining = 0;
			pawn->bNetDirty = 1;
			pawn->bForceNetUpdate = 1;
		}
	}
}
