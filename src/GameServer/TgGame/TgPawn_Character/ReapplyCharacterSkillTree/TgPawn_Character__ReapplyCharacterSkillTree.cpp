#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <set>

// REBUILD (2026-05-22): the two per-pawn reversal manifests — g_appliedSkillDeltas
// (direct m_fRaw writes) and g_appliedSkillBuffs (buff-registry entries) — are
// GONE. Every type-261 skill effect now applies through the canonical
// ProcessEffect path and is retained in s_AppliedEffectGroups (lifetime>0 or
// m_bIsManaged), so reversal on respawn/respec is ProcessEffect(bRemove) over the
// retained m_bSkillEffect clones — no out-of-band tracking. Unblocked by: real
// TgEffectBuff now loads (so class-157 modifier buffs route canonically),
// ApplyBuff does the 412→304 eager HEALTH_MAX recompute, and invisible skill
// groups no longer force m_bHasVisual (so SetEffectRep claims no HUD slot for
// them — the old reason this path avoided ProcessEffect).

// TgPawn::SetProperty @ 0x109bf420 — (pawn, propId, fNewValue). Writes
// UTgProperty.m_fRaw then dispatches ApplyProperty (vtable[0x8f4]) which is
// what fans the value out to the 7-ish cached/replicated fields the game
// actually uses for HP, MaxHP, Power, etc. UC ApplyEventBasedEffects writes
// m_fRaw directly but does NOT trigger fan-out — so without calling this
// after effect application, skill modifiers live in UTgProperty but the
// pawn's replicated stats never change.
static void SetPawnProperty(ATgPawn* Pawn, int nPropertyId, float fNewValue) {
	((void(__fastcall*)(ATgPawn*, void*, int, float))0x109bf420)(Pawn, nullptr, nPropertyId, fNewValue);
}

// Effects-channel max-HP snapshot. Dumps prop-304's registry-derived m_fRaw and
// its eager-cached engine field r_nHealthMaximum (ATgPawn+0x43c, written by
// ApplyProperty(304)) plus the applied-group count. Called at the begin /
// post-revert / end boundaries so an "add +maxHP skill, then unspec" test shows
// in ONE channel whether HEALTH_MAX returns to base — and how many applied
// groups linger after the revert pass. If raw/r_nHealthMaximum stay elevated
// after unspec while appliedGroups doesn't drop, the skill clones aren't being
// reverted; if appliedGroups drops but the value stays high, the registry leak
// is downstream of the clone removal.
static void LogMaxHpSnapshot(ATgPawn* Pawn, const char* phase) {
	if (!Pawn || !Logger::IsChannelEnabled("effects")) return;
	float raw = 0.0f, base = 0.0f; bool found = false;
	for (int i = 0; i < Pawn->s_Properties.Count; i++) {
		UTgProperty* p = Pawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == 304) { raw = p->m_fRaw; base = p->m_fBase; found = true; break; }
	}
	const int healthMax = *(int*)((char*)Pawn + 0x43c);
	const int appliedGroups =
		Pawn->r_EffectManager ? Pawn->r_EffectManager->s_AppliedEffectGroups.Count : -1;
	Logger::Log("effects",
		"[MAXHP/RCST %s] pawn=%p prop304%s raw=%.2f base=%.2f  r_nHealthMaximum=%d  appliedGroups=%d\n",
		phase, (void*)Pawn, found ? "" : "(MISSING)", raw, base, healthMax, appliedGroups);
}

// UTgPawn_Character::ReapplyCharacterSkillTree — the binary has a stripped
// `_notimplemented` stub at 0x109c0df0, so we own the full implementation.
//
// Contract: fetch the pawn's skill allocation (cached on PlayerInfo from
// PLAYER_REGISTER), wipe any previously-applied skill effect groups, then for
// every row allocated with points > 0 materialize all of that skill's effect
// groups from the asm data and register them via AddSkillBasedEffectGroup.
//
// Sub-skill rank granularity is not wired up yet — AsmDat capture never
// populates asm_data_set_sub_skills. Every effect group in the table uses
// sub_skill_id = 0. Treat "allocated" as a binary: ≥1 point → apply.
void __fastcall TgPawn_Character__ReapplyCharacterSkillTree::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();

	if (!Pawn || !Pawn->r_EffectManager) {
		Logger::Log("skills", "[Reapply] null pawn or effect manager, bailing\n");
		LogCallEnd();
		return;
	}

	ATgEffectManager* mgr = Pawn->r_EffectManager;

	// r_Owner on the manager is what UC's ProcessEffect uses as the Target of
	// `ApplyEventBasedEffects(r_Owner, Impact)` when applying removable
	// (property-modifier) effects. If it's null at spawn, ApplyToProperty has
	// no Target → UTgProperty.m_fRaw is never written → skills do nothing.
	Logger::Log("skills",
		"[Reapply] pawn=%p mgr=%p mgr->r_Owner=%p (expect ==pawn for effects to apply)\n",
		(void*)Pawn, (void*)mgr, (void*)mgr->r_Owner);

	LogMaxHpSnapshot((ATgPawn*)Pawn, "begin");

	// Shared across revert + apply phases so the final fanout pass at the end
	// covers every prop either layer touched. Moved up from the apply block
	// when armor was split into Revert/Apply so the revert phase can also
	// contribute to it.
	std::set<int> touchedPropIds;

	// =============================================================
	// PHASE 1 — REVERT (LIFO of apply order: skills first, armor last)
	// =============================================================
	//
	// Why LIFO: apply order is `armor → skills` (gear first, then training on
	// top). Reverting in reverse undoes each layer at the raw value it was
	// applied to, keeping every snapshot check valid. Before this restructure
	// armor revert lived INSIDE ApplyDefaultArmor and ran AFTER skill apply
	// had already shifted raw — its snapshot mismatched, the revert was
	// skipped, the new armor delta stacked on top of the old one. Same
	// failure on skill revert side. Toggling the skill UI re-saved without
	// changes → both layers re-stacked → HP grew unbounded.
	{
		// Reversal of the previous pass's skill effects is the m_bSkillEffect
		// clone removal below: ProcessEffect(bRemove) reverses each retained
		// clone's effects (TgEffect.Remove / TgEffectBuff.Remove → ApplyBuff
		// reverse + the 412→304 recompute). The old g_appliedSkillDeltas /
		// g_appliedSkillBuffs manifests are gone — every skill effect now applies
		// via ProcessEffect and is retained in s_AppliedEffectGroups (lifetime>0
		// or m_bIsManaged), so there is nothing to track or reverse out-of-band.

		// Remove any s_AppliedEffectGroups clones tagged m_bSkillEffect.
		// Iterate in reverse because ProcessEffect(bRemove) mutates the list.
		int skillClonesReverted = 0;
		const int appliedBefore = mgr->s_AppliedEffectGroups.Count;
		for (int i = mgr->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
			UTgEffectGroup* ag = mgr->s_AppliedEffectGroups.Data[i];
			if (!ag) continue;
			unsigned int gflags = *(unsigned int*)((char*)ag + 0x74);
			if (gflags & 0x02) {  // m_bSkillEffect
				Logger::Log("skills",
					"[Reapply/respec] remove ticker skill clone egId=%d\n",
					ag->m_nEffectGroupId);
				if (Logger::IsChannelEnabled("effects")) {
					Logger::Log("effects",
						"[MAXHP/RCST revert-clone] egId=%d gflags=0x%X effects=%d\n",
						ag->m_nEffectGroupId, gflags, ag->m_Effects.Count);
				}
				skillClonesReverted++;
				FImpactInfo impact{};
				mgr->eventProcessEffect(ag, /*bRemove=*/1, /*Buffers=*/nullptr,
					/*aInstigator=*/(AActor*)Pawn, impact);
			}
		}
		if (Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects",
				"[MAXHP/RCST phase1] appliedGroups %d -> %d, skill clones reverted=%d\n",
				appliedBefore, mgr->s_AppliedEffectGroups.Count, skillClonesReverted);
		}
	}

	// Armor revert AFTER skill revert: LIFO matches the apply order below.
	// Snapshots are still valid because nothing else has touched raw since
	// the skill revert above and armor was the last layer to apply last time.
	Armor::RevertDefaultArmor((ATgPawn*)Pawn);
	touchedPropIds.insert(304);  // HEALTH_MAX — armor always touches it if propHP exists

	LogMaxHpSnapshot((ATgPawn*)Pawn, "post-revert");

	// Always clear first — ReapplyCharacterSkillTree is called on spawn AND
	// after respec. Stale entries from a previous build have to go even if we
	// end up applying none (e.g. a respec to zero points).
	mgr->ClearSkillBasedEffectGroups();

	// =============================================================
	// PHASE 2 — APPLY (armor first, skills on top)
	// =============================================================
	//
	// Order matters for snapshot bookkeeping consistency with Phase 1. Apply
	// armor before any session-lookup bail so even a respec-to-zero or a
	// "no PlayerInfo" path still ends up wearing armor — without this, those
	// branches used to bail early and leave the player with bare base HP
	// after a respec.
	Armor::ApplyDefaultArmor((ATgPawn*)Pawn);
	touchedPropIds.insert(304);  // idempotent with the revert-side insert

	// Skill apply runs only when we have a real allocation. Wrapped in
	// do-while(false) so any missing-precondition path breaks out cleanly
	// and still hits the combined fanout below.
	do {
		auto it = GPawnSessions.find((ATgPawn*)Pawn);
		if (it == GPawnSessions.end()) {
			Logger::Log("skills", "[Reapply] pawn=%p has no session mapping — armor-only\n", Pawn);
			break;
		}

		PlayerInfo* info = PlayerRegistry::GetByGuidPtr(it->second);
		if (!info) {
			Logger::Log("skills", "[Reapply] no PlayerInfo for guid=%s — armor-only\n", it->second.c_str());
			break;
		}

		if (info->skills.empty()) {
			Logger::Log("skills", "[Reapply] charId=%lld has no skill allocation — armor-only\n",
				(long long)info->selected_character_id);
			break;
		}

		sqlite3* db = Database::GetConnection();
		if (!db) {
			Logger::Log("skills", "[Reapply] db unavailable — armor-only\n");
			break;
		}

	// Multi-row skills are NOT a "ranks per point" structure — each row gates
	// on `asm_data_set_effect_groups.required_skill_id` to a specific device
	// class. Skill 526 Jetpack Power lists four rows because the skill's
	// tooltip enumerates four jetpack variants:
	//   "Medic Jetpack -30% Power Cost"  (effect_group required_skill_id=360)
	//   "Recon Jetpack -30%"             (=359)
	//   "Robotic Jetpack -30%"           (=361)
	//   "Assault Jetpack -30%"           (=358)
	// Only the row matching the equipped jetpack should apply. We honor that
	// by tagging the ApplyBuff entry's `header.nReqSkillId` with the row's
	// required_skill_id; the binary's GetBuffIndex match rule (FUN_109cd890)
	// then filters: stored>0 entries only match queries with the same skillId.
	// required_skill_id=0 stays wildcard (universal effects).
	//
	// Earlier (mistaken) fix: ORDER BY id LIMIT s.points. That collapsed the
	// stack from -120% to -30% for power-cost skills but treated unrelated
	// rows as ranks. Reverted in favor of the correct discriminator.
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT seg.effect_group_id, seg.effect_group_type_value_id, "
		"       COALESCE(eg.required_skill_id, 0) "
		"FROM asm_data_set_skill_effect_groups seg "
		"LEFT JOIN asm_data_set_effect_groups eg "
		"  ON eg.effect_group_id = seg.effect_group_id "
		"WHERE seg.skill_group_id = ? AND seg.skill_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("skills", "[Reapply] prepare failed: %s — armor-only\n", sqlite3_errmsg(db));
		break;
	}

	int appliedGroups = 0;
	// (touchedPropIds declared at function scope above so it spans both phases.)

	Logger::Log("skills", "[Reapply] info->skills contents (%d entries):\n", (int)info->skills.size());
	for (const SkillAllocation& s : info->skills) {
		Logger::Log("skills", "[Reapply]   skill %d/%d pts=%d\n",
			s.skill_group_id, s.skill_id, s.points);
	}

	for (const SkillAllocation& s : info->skills) {
		if (s.points <= 0) continue;

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
		sqlite3_bind_int(stmt, 1, s.skill_group_id);
		sqlite3_bind_int(stmt, 2, s.skill_id);

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int egId         = sqlite3_column_int(stmt, 0);
			int egType       = sqlite3_column_int(stmt, 1);
			// (required_skill_id is read by BuildEffectGroup into m_nRequiredSkillId;
			// the canonical ApplyBuff path reads it off the group, so no local copy.)

			UTgEffectGroup* group = BuildEffectGroup(egId, egType);
			if (!group) {
				Logger::Log("skills",
					"[Reapply] BuildEffectGroup failed egId=%d egType=%d (skillGroup=%d skill=%d)\n",
					egId, egType, s.skill_group_id, s.skill_id);
				continue;
			}

			// Mark the group as skill-sourced so UC filters (e.g.
			// TgEffectGroup.m_bSkillEffect at +0x2A8 bit) discriminate it from
			// device-sourced groups when needed.
			{
				unsigned int& gflags = *(unsigned int*)((char*)group + 0x74);
				gflags |= 0x02;  // m_bSkillEffect (see TgEffectGroup.uc:63)
			}

			mgr->AddSkillBasedEffectGroup(group);

			// effect_group_type_value_id governs WHEN this fires:
			//   261  Equip             — applied at equip time (our case)
			//   264  Hit               — fires when the player hits a target
			//   505  Hit Situational   — conditional hit trigger
			//   759  Successful Hit    — fires on successful hit
			//   1104 Reactive (skill)  — event-driven by combat
			// Only type 261 should apply at Reapply time; the others are
			// triggered by combat events and will be pulled from
			// s_SkillBasedEffectGroups by UC's ApplyEffectType(T) when the
			// corresponding event fires. Applying a "Successful Hit" heal
			// at spawn wrongly heals the player for the on-hit amount.
			if (egType != 261) {
				// Still added to s_SkillBasedEffectGroups above so UC's
				// event-driven iteration can find them; just don't apply now.
				Logger::Log("skills",
					"[Reapply] egId=%d egType=%d skill=%d/%d — non-261, not applying at equip\n",
					egId, egType, s.skill_group_id, s.skill_id);
				appliedGroups++;
				continue;
			}

			// Canonical apply: EVERY type-261 skill group goes through
			// ProcessEffect → clone → ApplyEffects → each effect's virtual
			// ApplyEffect. Modifier buffs route via TgEffectBuff.ApplyEffect →
			// ApplyBuff (incl. %max-health on prop 412, folded into HEALTH_MAX by
			// ApplyBuff's 412→304 GetBuffedProperty recompute); direct stat effects
			// via base TgEffect.ApplyEffect → ApplyToProperty. m_bSkillEffect was
			// set above so GetBuffType buckets buffs into the SKILL layer.
			//
			// No direct-apply branch, no delta maps: reversal is the m_bSkillEffect
			// clone removal in Phase 1. Pure stat-mod (lifetime=0/interval=0) groups
			// are retained too — BuildEffectGroup sets m_bIsManaged for lifetime<=0,
			// so ProcessEffect keeps the clone in s_AppliedEffectGroups. They no
			// longer flood the HUD: invisible skill groups (icon_id=0) don't set
			// m_bHasVisual, so SetEffectRep claims no managed slot for them.
			{
				FImpactInfo impact{};
				mgr->eventProcessEffect(group, /*bRemove=*/0, /*Buffers=*/nullptr,
					/*aInstigator=*/(AActor*)Pawn, impact);
				for (int i = 0; i < group->m_Effects.Count; i++) {
					if (group->m_Effects.Data[i])
						touchedPropIds.insert(group->m_Effects.Data[i]->m_nPropertyId);
				}
			}
			appliedGroups++;
		}
	}
	sqlite3_finalize(stmt);

	Logger::Log("skills",
		"[Reapply] charId=%lld skills=%d appliedGroups=%d touchedProps=%d\n",
		(long long)info->selected_character_id, (int)info->skills.size(),
		appliedGroups, (int)touchedPropIds.size());
	} while (false);
	// (end of do-while skill-apply scope. Breaks above land here so the
	// combined fanout below always runs, even on the armor-only paths.)

	// =============================================================
	// PHASE 3 — combined fanout
	// =============================================================
	//
	// Single SetProperty pass over every prop touched by revert OR apply
	// (skill direct deltas + armor's HEALTH_MAX entry). ApplyProperty fans
	// the value into r_fMaxHealth and the other cached/replicated HP storage
	// sites so the client sees the final m_fRaw, not the intermediates that
	// would have been visible if armor and skill each fanned out separately.
	for (int pid : touchedPropIds) {
		UTgProperty* prop = nullptr;
		for (int i = 0; i < Pawn->s_Properties.Count; i++) {
			UTgProperty* p = Pawn->s_Properties.Data[i];
			if (p && p->m_nPropertyId == pid) { prop = p; break; }
		}
		if (!prop) {
			Logger::Log("skills",
				"[Reapply/fanout] propId=%d not in s_Properties — can't fan out\n", pid);
			continue;
		}
		Logger::Log("skills",
			"[Reapply/fanout] propId=%d raw=%.3f base=%.3f -> SetProperty\n",
			pid, prop->m_fRaw, prop->m_fBase);
		SetPawnProperty((ATgPawn*)Pawn, pid, prop->m_fRaw);
	}

	// Post-apply property dump — log every UTgProperty in the pawn's
	// s_Properties that a skill effect should have touched. If m_fRaw ==
	// m_fBase for these after appliedGroups>0, UC's ApplyEventBasedEffects
	// path isn't writing through. If the property isn't in s_Properties at
	// all, there's no target to modify and the whole mechanism is moot.
	{
		static const int watchProps[] = {
			155 /*PROTECTION_PHYSICAL*/, 243 /*POWERPOOL*/, 244 /*POWERPOOL_RECHARGE_RATE*/,
			255 /*POWERPOOL_MAX*/,       304 /*HEALTH_MAX*/, 51  /*HEALTH*/
		};
		for (int pid : watchProps) {
			UTgProperty* prop = nullptr;
			for (int i = 0; i < Pawn->s_Properties.Count; i++) {
				UTgProperty* p = Pawn->s_Properties.Data[i];
				if (p && p->m_nPropertyId == pid) { prop = p; break; }
			}
			if (prop) {
				Logger::Log("skills",
					"[Reapply/prop] propId=%d base=%.3f raw=%.3f min=%.3f max=%.3f\n",
					pid, prop->m_fBase, prop->m_fRaw, prop->m_fMinimum, prop->m_fMaximum);
			} else {
				Logger::Log("skills",
					"[Reapply/prop] propId=%d NOT IN s_Properties (cannot be modified by effect)\n",
					pid);
			}
		}
	}

	LogMaxHpSnapshot((ATgPawn*)Pawn, "end");

	LogCallEnd();
}
