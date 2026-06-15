#include "src/GameServer/TgGame/TgEffectManager/ProcessReactiveSkillBasedEffectGroup/TgEffectManager__ProcessReactiveSkillBasedEffectGroup.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <map>
#include <set>

// Reimplements stripped UC native
//   `native function ProcessReactiveSkillBasedEffectGroup(int nCategory, bool bRemove)`
//   on TgEffectManager. Original entry is an empty stub at 0x10a6ef60.
//
// Call sites (apply / remove):
//   * APPLY  — TgEffectManager.uc:272-275: when ProcessEffect adds a group of
//              a category that was not already present (`!bCategoryExisted`),
//              UC dispatches `ProcessReactiveSkillBasedEffectGroup(cat, false)`.
//   * REMOVE — original binary's RemoveEffectGroup. We re-fire it from our
//              reimplemented `TgEffectManager__RemoveEffectGroup` whenever the
//              swap-remove leaves the category empty in s_AppliedEffectGroups.
//
// This is the dispatch surface for "reactive" skills — DB skill_effect_groups
// rows whose effect_group_type_value_id = 1104 ("Reactive Skills only") and
// whose underlying effect_group has `required_category_value_id` set.
// Concrete example: Aegis Armament (skill 913) — effect group 26696, prop 155
// (Protection-Physical) +25 calc=ADD, m_nReqCategoryCode=770 (PERSONAL_SHIELD).
// The skill is added to s_SkillBasedEffectGroups by ReapplyCharacterSkillTree
// at equip time but NOT applied; this hook flips it on whenever the player
// gains a category-770 effect group (any shield), and off when the last one
// leaves.
//
// Aegis-style skills are pure stat mods (lifetime=0, calc 67/68/69/70). We
// apply them as direct s_Properties[propId]->m_fRaw writes — same shape as
// Inventory::ApplyDeviceEquipEffects. Reverse on bRemove using the calc's
// inverse delta. We do NOT add to s_AppliedEffectGroups: the skill is
// transient by design (lifecycle bound to the host shield's category presence)
// and ProcessEffect is the wrong path for it (would grab a HUD slot, would
// run ProtectionModifier on the lifetime, would compete for replication).
//
// If a future reactive skill carries a non-zero lifetime or apply_interval,
// log + skip — those would need a different dispatch (ProcessEffect or a
// sub-timer) and we'd want to design that explicitly rather than guess.
namespace {

void ApplyOrReverseStatMod(ATgPawn* pawn, int propId, int calcMethod, float base, bool bRemove) {
	if (!pawn) return;

	// O(1) TMap lookup at pawn+0x400 — the binary native uses this same path.
	UTgProperty* prop = TgPawn__GetProperty::CallOriginal(pawn, nullptr, propId);
	if (!prop) {
		Logger::Log("skills",
			"[REACTIVE]   propId=%d not in pawn s_Properties — skipped\n", propId);
		return;
	}

	const float curRaw   = prop->m_fRaw;
	const float propBase = prop->m_fBase;
	float delta = 0.0f;
	switch (calcMethod) {
		case 67: delta = base;              break; // ADD
		case 70: delta = -base;             break; // SUBTRACT
		case 68: delta = propBase * base;   break; // PERC_INCREASE
		case 69: delta = -propBase * base;  break; // PERC_DECREASE
		default:
			Logger::Log("skills",
				"[REACTIVE]   propId=%d cm=%d unsupported — skipped\n",
				propId, calcMethod);
			return;
	}
	if (bRemove) delta = -delta;

	prop->m_fRaw = curRaw + delta;
	Logger::Log("skills",
		"[REACTIVE]   propId=%d cm=%d base=%.2f bRemove=%d  m_fRaw %.2f -> %.2f\n",
		propId, calcMethod, base, bRemove ? 1 : 0, curRaw, prop->m_fRaw);
}

// Per-pawn record of which reactive categories are CURRENTLY committed to
// s_Properties. The native applies its stat-mod as a raw `m_fRaw += delta`
// (no in-place re-derivation), so a double-ON or a missed-OFF permanently
// desyncs the property — and because death-cleanup only fires OFF for
// categories still present at death, a leak can't self-heal by dying, only by
// reconnect. This registry makes apply/reverse idempotent: ON commits the
// delta only on the off->on edge, OFF reverses only on the on->off edge.
//
// Keyed by r_nPawnId (monotonic per-spawn, never reused — same rationale as
// Armor::Records), so the committed-state persists across same-actor respawn
// alongside the m_fRaw it tracks, while a fresh pawn / reconnect gets a clean
// slate. Entries self-prune when a pawn's last reactive category turns off.
std::map<int, std::set<int>>& ActiveReactiveCategories() {
	static std::map<int, std::set<int>> g;
	return g;
}

}  // namespace

void __fastcall TgEffectManager__ProcessReactiveSkillBasedEffectGroup::Call(
	ATgEffectManager* Manager, void* /*edx*/,
	int nCategory, unsigned long bRemove)
{
	if (!Manager) return;

	AActor* owner = Manager->r_Owner;
	// Two-tier validity: null OR small-int corruption. owner->Class reads at
	// offset 0x34; if owner=0x35d (the observed crash value, =861
	// TG_PHYSICALITY_MECHANICAL) we'd fault at 0x391. Some upstream path is
	// depositing physical-type constants into UObject* fields — guard until
	// it's tracked down. See TgEffectGroup__RemoveEffects.cpp for context.
	if (!owner || reinterpret_cast<uintptr_t>(owner) < 0x10000u) {
		if (owner) {
			Logger::Log("skills",
				"[REACTIVE] mgr=%p r_Owner=%p — small-int value, aborting\n",
				(void*)Manager, (void*)owner);
		}
		return;
	}
	// TgDeployable can also have an effect manager but doesn't carry skill
	// allocations; reactive dispatch is a no-op for it. Cached class name
	// keeps this off the per-call alloc path.
	const std::string& cn = ObjectClassCache::GetClassName(owner->Class);
	if (cn.compare(0, 19, "Class TgGame.TgPawn") != 0) return;
	ATgPawn* pawn = (ATgPawn*)owner;

	// Idempotency guard: collapse redundant edges. ON only commits when the
	// category isn't already committed; OFF only reverses when it is. This caps
	// the worst case of any unbalanced caller at a single delta that self-heals
	// on the next clean toggle — it can never compound across shield re-casts.
	auto& reg = ActiveReactiveCategories();
	auto regIt = reg.find(pawn->r_nPawnId);
	const bool currentlyOn = (regIt != reg.end() && regIt->second.count(nCategory) != 0);
	const bool wantOn = (bRemove == 0);
	if (currentlyOn == wantOn) {
		Logger::Log("skills",
			"[REACTIVE] mgr=%p cat=%d bRemove=%lu  already %s — idempotent no-op\n",
			(void*)Manager, nCategory, bRemove, currentlyOn ? "ON" : "OFF");
		return;
	}

	int matched = 0;
	for (int i = 0; i < Manager->s_SkillBasedEffectGroups.Count; i++) {
		UTgEffectGroup* g = Manager->s_SkillBasedEffectGroups.Data[i];
		if (!g || reinterpret_cast<uintptr_t>(g) < 0x10000u) {
			if (g) {
				Logger::Log("skills",
					"[REACTIVE] mgr=%p s_SkillBasedEffectGroups[%d]=%p — "
					"small-int value, skipping\n",
					(void*)Manager, i, (void*)g);
			}
			continue;
		}
		if (g->m_nReqCategoryCode != nCategory) continue;

		// Guard against future reactive skills with non-trivial lifecycle.
		// All shipped reactive skills today are lifetime=0 stat mods; if
		// someone adds a ticking one we want a loud skip rather than a
		// silently-broken refresh chain.
		if (g->m_fLifeTime > 0.0f || g->m_fApplyInterval > 0.0f) {
			Logger::Log("skills",
				"[REACTIVE] mgr=%p egId=%d skipped — non-zero lifetime/interval (%.2f / %.2f) "
				"not supported by direct apply path\n",
				(void*)Manager, g->m_nEffectGroupId, g->m_fLifeTime, g->m_fApplyInterval);
			continue;
		}

		Logger::Log("skills",
			"[REACTIVE] mgr=%p cat=%d bRemove=%lu  matched egId=%d effects=%d\n",
			(void*)Manager, nCategory, bRemove, g->m_nEffectGroupId, g->m_Effects.Count);

		for (int j = 0; j < g->m_Effects.Count; j++) {
			UTgEffect* e = g->m_Effects.Data[j];
			if (!e || reinterpret_cast<uintptr_t>(e) < 0x10000u) {
				if (e) {
					Logger::Log("skills",
						"[REACTIVE] mgr=%p egId=%d effect[%d]=%p — small-int "
						"value, skipping\n",
						(void*)Manager, g->m_nEffectGroupId, j, (void*)e);
				}
				continue;
			}
			ApplyOrReverseStatMod(pawn, e->m_nPropertyId, e->m_nCalcMethodCode,
			                      e->m_fBase, bRemove != 0);
		}
		matched++;
	}

	// Record the new committed-state only on a real transition. ON commits the
	// category only if a skill group actually applied (matched>0); OFF clears it
	// and prunes the pawn entry once it holds no committed categories.
	if (wantOn) {
		if (matched > 0) reg[pawn->r_nPawnId].insert(nCategory);
	} else if (regIt != reg.end()) {
		regIt->second.erase(nCategory);
		if (regIt->second.empty()) reg.erase(regIt);
	}

	if (matched == 0) {
		Logger::Log("skills",
			"[REACTIVE] mgr=%p cat=%d bRemove=%lu  no matching skill groups "
			"(s_SkillBasedEffectGroups has %d entries)\n",
			(void*)Manager, nCategory, bRemove, Manager->s_SkillBasedEffectGroups.Count);
	}
}
