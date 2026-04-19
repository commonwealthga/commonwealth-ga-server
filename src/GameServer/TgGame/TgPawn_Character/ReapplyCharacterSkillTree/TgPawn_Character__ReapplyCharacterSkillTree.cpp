#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <set>
#include <map>

// Per-pawn record of what each Reapply pass wrote so a subsequent respec
// can REVERSE it before applying the new allocation. Keyed by pawn pointer
// → (propId → cumulative delta m_fRaw received from the direct-apply path).
// Ticker-based skill effects track themselves via s_AppliedEffectGroups and
// get removed separately.
static std::map<ATgPawn_Character*, std::map<int, float>> g_appliedSkillDeltas;

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

	// --- RESPEC REVERSAL PASS ---
	// 1. Direct-apply deltas from last Reapply: subtract from m_fRaw and
	//    re-fan-out so replicated/cached stats follow.
	// 2. Ticker-based skill clones in s_AppliedEffectGroups: call
	//    ProcessEffect(bRemove=true) so UC's LifeOver/timer cleanup runs and
	//    the clone's property modifications get reversed.
	{
		auto& prev = g_appliedSkillDeltas[Pawn];
		std::set<int> reversedProps;
		for (auto& kv : prev) {
			int pid = kv.first;
			float delta = kv.second;
			UTgProperty* prop = nullptr;
			for (int i = 0; i < Pawn->s_Properties.Count; i++) {
				UTgProperty* p = Pawn->s_Properties.Data[i];
				if (p && p->m_nPropertyId == pid) { prop = p; break; }
			}
			if (!prop) continue;
			prop->m_fRaw -= delta;
			reversedProps.insert(pid);
			Logger::Log("skills",
				"[Reapply/respec] reverse propId=%d delta=%.3f -> raw=%.3f\n",
				pid, delta, prop->m_fRaw);
		}
		prev.clear();

		// Fan out the reversals so replicated stats follow back down.
		for (int pid : reversedProps) {
			UTgProperty* prop = nullptr;
			for (int i = 0; i < Pawn->s_Properties.Count; i++) {
				UTgProperty* p = Pawn->s_Properties.Data[i];
				if (p && p->m_nPropertyId == pid) { prop = p; break; }
			}
			if (prop) SetPawnProperty((ATgPawn*)Pawn, pid, prop->m_fRaw);
		}

		// Remove any s_AppliedEffectGroups clones tagged m_bSkillEffect.
		// Iterate in reverse because ProcessEffect(bRemove) mutates the list.
		for (int i = mgr->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
			UTgEffectGroup* ag = mgr->s_AppliedEffectGroups.Data[i];
			if (!ag) continue;
			unsigned int gflags = *(unsigned int*)((char*)ag + 0x74);
			if (gflags & 0x02) {  // m_bSkillEffect
				Logger::Log("skills",
					"[Reapply/respec] remove ticker skill clone egId=%d\n",
					ag->m_nEffectGroupId);
				FImpactInfo impact{};
				mgr->eventProcessEffect(ag, /*bRemove=*/1, /*Buffers=*/nullptr,
					/*aInstigator=*/(AActor*)Pawn, impact);
			}
		}
	}

	// Always clear first — ReapplyCharacterSkillTree is called on spawn AND
	// after respec. Stale entries from a previous build have to go even if we
	// end up applying none (e.g. a respec to zero points).
	mgr->ClearSkillBasedEffectGroups();

	auto it = GPawnSessions.find((ATgPawn*)Pawn);
	if (it == GPawnSessions.end()) {
		Logger::Log("skills", "[Reapply] pawn=%p has no session mapping\n", Pawn);
		LogCallEnd();
		return;
	}

	PlayerInfo* info = PlayerRegistry::GetByGuidPtr(it->second);
	if (!info) {
		Logger::Log("skills", "[Reapply] no PlayerInfo for guid=%s\n", it->second.c_str());
		LogCallEnd();
		return;
	}

	if (info->skills.empty()) {
		Logger::Log("skills", "[Reapply] charId=%lld has no skill allocation\n",
			(long long)info->selected_character_id);
		LogCallEnd();
		return;
	}

	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("skills", "[Reapply] db unavailable\n");
		LogCallEnd();
		return;
	}

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT effect_group_id, effect_group_type_value_id "
		"FROM asm_data_set_skill_effect_groups "
		"WHERE skill_group_id = ? AND skill_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("skills", "[Reapply] prepare failed: %s\n", sqlite3_errmsg(db));
		LogCallEnd();
		return;
	}

	int appliedGroups = 0;
	std::set<int> touchedPropIds;

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
			int egId   = sqlite3_column_int(stmt, 0);
			int egType = sqlite3_column_int(stmt, 1);

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

			// Branch on effect shape (within type 261):
			//   * Ticking / timed effects (apply_interval_sec > 0 OR
			//     lifetime_sec > 0) MUST go through ProcessEffect — it's
			//     what schedules the LifeOver timer and the per-tick apply
			//     (e.g. "regen +X HP every 2s" skill). Without ProcessEffect
			//     no tick ever fires.
			//   * Pure stat modifiers (lifetime=0 AND interval=0) write
			//     m_fRaw once and are done. Routing them through
			//     ProcessEffect is harmful — the resulting clone in
			//     s_AppliedEffectGroups grabs a HUD slot via SetEffectRep
			//     and with 10+ skills pushes real device/buff icons off
			//     the bar. Apply these directly instead.
			bool needsTicker = (group->m_fLifeTime > 0.0f)
			                || (group->m_fApplyInterval > 0.0f);
			if (needsTicker) {
				FImpactInfo impact{};
				mgr->eventProcessEffect(group, /*bRemove=*/0, /*Buffers=*/nullptr,
					/*aInstigator=*/(AActor*)Pawn, impact);
				// ProcessEffect did the property writes via
				// ApplyEventBasedEffects. Record touched props for fan-out.
				for (int i = 0; i < group->m_Effects.Count; i++) {
					if (group->m_Effects.Data[i])
						touchedPropIds.insert(group->m_Effects.Data[i]->m_nPropertyId);
				}
			} else {
				// Direct apply. Calc methods (asm_data_set_valid_values):
				//   67 Add (+), 70 Subtract (-),
				//   68 Increase (+%), 69 Decrease (-%).
				// %-variants take a fraction of the property's m_fBase —
				// matches UC's TgEffect.ApplyToProperty math.
				for (int i = 0; i < group->m_Effects.Count; i++) {
					UTgEffect* effect = group->m_Effects.Data[i];
					if (!effect) continue;
					int pid  = effect->m_nPropertyId;
					int calc = effect->m_nCalcMethodCode;
					float amt = effect->m_fBase;

					// Many skill effects target _MODIFIER propIds instead
					// of the base stat (e.g. +10% HEALTH_MAX is stored as
					// propId=412 HEALTH_MAX_MODIFIER). These modifier props
					// aren't in InitializeDefaultProps, and even if they
					// were their m_fBase is 0 so calc=68 yields delta=0.
					// Redirect to the base prop so the calc math uses the
					// real m_fBase (1300 for HEALTH_MAX) and m_fRaw lands
					// on the stat that ApplyProperty fans out.
					int applyPid = pid;
					switch (pid) {
						case 412: applyPid = 304; break; // HEALTH_MAX_MODIFIER -> HEALTH_MAX
						case  66: applyPid =  49; break; // GROUNDSPEED_MODIFIER -> GROUND_SPEED
						default: break;
					}

					UTgProperty* prop = nullptr;
					for (int j = 0; j < Pawn->s_Properties.Count; j++) {
						UTgProperty* p = Pawn->s_Properties.Data[j];
						if (p && p->m_nPropertyId == applyPid) { prop = p; break; }
					}
					if (!prop) continue;

					// BuildEffectGroup normalizes class-157 percent values to
					// fractions at load time, so by here m_fBase is always a
					// fraction for calc=68/69 regardless of source class.
					float delta = 0.0f;
					switch (calc) {
						case 67: delta =  amt;                   break;
						case 70: delta = -amt;                   break;
						case 68: delta =  prop->m_fBase * amt;   break;
						case 69: delta = -prop->m_fBase * amt;   break;
						default:
							Logger::Log("skills",
								"[Reapply/effect] egId=%d propId=%d unknown calc=%d — skipped\n",
								group->m_nEffectGroupId, pid, calc);
							continue;
					}

					prop->m_fRaw += delta;
					effect->m_fCurrent = delta;
					touchedPropIds.insert(applyPid);
					g_appliedSkillDeltas[Pawn][applyPid] += delta;

					if (applyPid != pid) {
						Logger::Log("skills",
							"[Reapply/effect] egId=%d propId=%d->applyPid=%d calc=%d amt=%.3f delta=%.3f -> raw=%.3f (direct,redirect)\n",
							group->m_nEffectGroupId, pid, applyPid, calc, amt, delta, prop->m_fRaw);
					} else {
						Logger::Log("skills",
							"[Reapply/effect] egId=%d propId=%d calc=%d amt=%.3f delta=%.3f -> raw=%.3f (direct)\n",
							group->m_nEffectGroupId, pid, calc, amt, delta, prop->m_fRaw);
					}
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

	// Fan-out: for every property a skill effect touched, call SetProperty
	// with the current m_fRaw so the native ApplyProperty fans the value
	// into the pawn's replicated/cached fields (r_fMaxHealth, power pool
	// max, etc.). Without this, effects live only in UTgProperty.m_fRaw —
	// stat screens and replicated client state never see the change.
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

	LogCallEnd();
}
