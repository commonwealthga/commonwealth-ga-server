#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Direct native call to TgPawn::GetBuffIndex (vtable[0x560], FUN_109cd890).
// We CANNOT use the SDK wrapper `Pawn->GetBuffIndex(...)` — its stub at
// TgGame_functions.cpp:44095 forgets to copy the input `*nIndex` into the
// parms struct, so the native sees uninitialized stack garbage as the loop
// start index. With find-or-add, garbage past array-end skips existing
// entries → appends fresh on every call → m_EffectBuffInfo grows unbounded
// and the BuffFormula sums every duplicate, applying the same modifier N
// times. Direct native call below keeps `nIndex=0` end-to-end.
//
// __thiscall: ECX = this; rest on stack. __fastcall with a junk edx maps
// cleanly to that.
typedef void (__fastcall* GetBuffIndexNativeFn)(
	ATgPawn* /*this*/, void* /*edx*/,
	int /*propId*/, int /*catCode*/, int /*skillId*/, int /*devInst*/,
	int /*bAddIfNotExists*/, int* /*startIdx_inout*/);
static const GetBuffIndexNativeFn GetBuffIndexNative = (GetBuffIndexNativeFn)0x109cd890;

// TgPawn::GetBuffedProperty — intact native @ 0x109d7ff0 (flat-arg convention,
// per TgEffect__CheckEffectBuffModifier.cpp). Runs ConvertPropToPropList(ctx)
// expansion + the 3-layer CheckBuffInfoList formula over m_EffectBuffInfo.
typedef void(__fastcall* GetBuffedPropertyFn)(
	ATgPawn*, void* /*edx*/,
	unsigned char /*eRequestContext*/, int /*nPropId*/,
	int /*nReqCategoryCode*/, int /*nReqSkillId*/, int /*nReqDeviceInstId*/,
	int /*bUsePotencyModifier*/, float /*fBaseValue*/, float* /*fBuffedValue*/,
	void* /*Effect*/);
static const GetBuffedPropertyFn GetBuffedPropertyNative = (GetBuffedPropertyFn)0x109d7ff0;

// TgPawn::ApplyProperty — intact native @ 0x109cc7d0 (this, UTgProperty*). Reads
// the property's m_fRaw and fans it out to the cached/replicated engine field
// (e.g. r_nHealthMaximum @ +0x43c for HEALTH_MAX).
typedef void(__fastcall* ApplyPropertyFn)(ATgPawn*, void* /*edx*/, UTgProperty*);
static const ApplyPropertyFn ApplyPropertyNative = (ApplyPropertyFn)0x109cc7d0;

// Eager-cached base-stat recompute. Most buffed stats are LIVE-read — the engine
// calls GetBuffedProperty at use time (e.g. UTgDeviceFire::GetPropertyValueById
// for Range/Accuracy), so a registry change is picked up automatically. But a
// few base stats are EAGER-cached: ApplyProperty writes a concrete engine field
// from the property's m_fRaw, and the gameplay code reads that field, never the
// registry. HEALTH_MAX is the canonical case — ApplyProperty(304) (verified at
// 0x109cc7d0) copies m_fRaw into r_nHealthMaximum with no buff query, and a
// "+%max health" skill registers under prop 412 (HEALTH_MAX_MODIFIER), which
// ConvertPropToPropList(ITEM,304) expands to {412,390} (verified at 0x109e5220).
// So after a 412 buff changes, fold it back into 304: recompute m_fRaw from the
// registry, then ApplyProperty to update the cached field. Character-wide stat →
// ungated query (cat/skill/devInst = 0 matches the ungated skill entries).
static void RecomputeEagerBaseProp(ATgPawn* Pawn, int basePropId) {
	if (!Pawn || Pawn->s_Properties.Data == nullptr) return;
	UTgProperty* prop = nullptr;
	for (int i = 0; i < Pawn->s_Properties.Count; ++i) {
		UTgProperty* p = Pawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == basePropId) { prop = p; break; }
	}
	if (!prop) return;

	const float before = prop->m_fRaw;
	float buffed = prop->m_fBase;
	GetBuffedPropertyNative(Pawn, /*edx=*/nullptr,
		/*eRequestContext=ITEM*/1, basePropId,
		/*nReqCategoryCode=*/0, /*nReqSkillId=*/0, /*nReqDeviceInstId=*/0,
		/*bUsePotencyModifier=*/0, /*fBaseValue=*/prop->m_fBase,
		/*&fBuffedValue=*/&buffed, /*Effect=*/nullptr);

	prop->m_fRaw = buffed;
	ApplyPropertyNative(Pawn, /*edx=*/nullptr, prop);

	if (Logger::IsChannelEnabled("effects")) {
		// r_nHealthMaximum @ ATgPawn+0x43c — the eager-cached field ApplyProperty(304)
		// writes from m_fRaw (verified by decompiling 0x109cc7d0). This is the value
		// the game actually uses for the player's max HP; logging it next to the
		// registry-derived m_fRaw shows whether the cache tracks the buff state.
		const int healthMax = (basePropId == 304) ? *(int*)((char*)Pawn + 0x43c) : 0;
		Logger::Log("effects",
			"[MAXHP/recompute] pawn=%p baseProp=%d m_fBase=%.2f raw %.2f -> %.2f  r_nHealthMaximum=%d\n",
			(void*)Pawn, basePropId, prop->m_fBase, before, buffed, healthMax);
	}
}

// Reimplementation of TgPawn::ApplyBuff (UC native; binary stub @ 0x109bf7b0
// is empty so the buff registry never gets populated by the original code).
//
// Adds (or updates / removes) a single FBuffInfo entry in `pawn->m_EffectBuffInfo`
// (TArray @ +0x100C). The original GetPropertyValueById path on TgDeviceFire
// reads this registry — via vtable[0x568]=FUN_109cd6f0 (aggregator) and
// vtable[0x56C]=FUN_109cd4a0 (formula) — when computing fire-mode params, so
// any rolled mod we register here flows through to gameplay automatically.
//
// Slot mapping verified by decompiling FUN_109cd4a0 (the formula function):
//   v1 = base * (1 + ΣfItemPercent /100) + ΣfItemMod                                 // ITEM
//   v2 = v1   * (1 + ΣfSkillPercent/100) + ΣfSkillMod                                 // SKILL
//   v3 = v2   * (1 + (ΣfSelfPercent + ΣfPercent)/100) + ΣfSelfMod + ΣfMod             // Self+Generic
//
// Source-type → slot routing follows the field naming directly:
//   BUFF_SOURCE_TYPE_SKILL      (0) → fSkill*
//   BUFF_SOURCE_TYPE_ITEM       (1) → fItem*    ← rolled mods land here
//   BUFF_SOURCE_TYPE_EFFICIENCY (2) → f*         (generic catch-all)
//   BUFF_SOURCE_TYPE_SELF       (3) → fSelf*
//   BUFF_SOURCE_TYPE_OTHER      (4) → f*         (generic catch-all)
//
// Calc method 68 (PERC_INCREASE) / 69 (PERC_DECREASE) → percent slot.
// Calc method 67 (ADD)           / 70 (SUBTRACT)      → absolute slot.
// 69 and 70 are sign-flipped variants of 68 and 67.
//
// bRemove flips the sign once more (subtract the previously-added delta).

void __fastcall TgPawn__ApplyBuff::Call(ATgPawn* Pawn, void* /*edx*/, FBuffHeader BuffFilter,
                                        int nCalcMethodCode, float fAmount, unsigned long bRemove,
                                        unsigned char buffSourceType) {
	if (!Pawn) return;

	// Find existing matching entry, or append a fresh one when not removing.
	//
	// Apply path (bRemove=0): the binary's GetBuffIndex with bAddIfNotExists=1
	// uses EXACT match (every BuffHeader field must equal) and appends a fresh
	// entry on miss. That's exactly what we want, so call the native directly.
	// (Note: we cannot use the SDK wrapper `Pawn->GetBuffIndex` — its stub at
	// TgGame_functions.cpp:44095 forgets to copy *nIndex into the parms struct,
	// so the native sees uninitialized stack garbage as the loop start index.)
	//
	// Reverse path (bRemove=1): the binary's GetBuffIndex with bAddIfNotExists=0
	// uses WILDCARD match — stored=0 in any BuffHeader field is treated as a
	// wildcard that matches any non-zero query for that field. This semantic is
	// correct for the FILTER pass in GetBuffedProperty (where wildcards mean
	// "applies to all queries"), but WRONG for the REVERSE pass where we need
	// to find the specific entry we appended on apply. Concrete failure: if
	// apply registers two entries against the same propId — one with skillId=0
	// (wildcard) and one with skillId=308 (device-gated) — the reverse for the
	// skillId=308 entry wildcard-matches the skillId=0 entry FIRST in the
	// linear scan, debits the wrong slot, and the slot that should have been
	// debited drifts upward over time. Observed as the medstation heal losing
	// exactly 10% per Reapply cycle, with the missing amount accumulating in
	// the device-gated slot that never matches the heal query at fire time.
	//
	// Fix: do an exact-match scan ourselves for the reverse path. m_EffectBuffInfo
	// is typically 30-50 entries and only the Reapply path (respawn / respec /
	// connect) reverses, so the cost is negligible.
	int index = -1;
	if (bRemove) {
		if (Pawn->m_EffectBuffInfo.Data == nullptr) return;
		const int n = Pawn->m_EffectBuffInfo.Num();
		for (int i = 0; i < n; ++i) {
			const FBuffHeader& h = Pawn->m_EffectBuffInfo.Data[i].BuffHeader;
			if (h.nPropId          == BuffFilter.nPropId          &&
			    h.nReqCategoryCode == BuffFilter.nReqCategoryCode &&
			    h.nReqSkillId      == BuffFilter.nReqSkillId      &&
			    h.nReqDeviceInstId == BuffFilter.nReqDeviceInstId) {
				index = i;
				break;
			}
		}
		if (index < 0) return;  // entry already gone — nothing to reverse
	} else {
		index = 0;
		GetBuffIndexNative(Pawn, /*edx=*/nullptr,
		                   BuffFilter.nPropId, BuffFilter.nReqCategoryCode,
		                   BuffFilter.nReqSkillId, BuffFilter.nReqDeviceInstId,
		                   /*bAddIfNotExists=*/1,
		                   &index);
		if (index < 0) return;
		if (Pawn->m_EffectBuffInfo.Data == nullptr) return;
		if (index >= Pawn->m_EffectBuffInfo.Num()) return;
	}

	FBuffInfo& entry = Pawn->m_EffectBuffInfo.Data[index];

	const bool isPercent = (nCalcMethodCode == 68 || nCalcMethodCode == 69);
	float delta = fAmount;
	if (nCalcMethodCode == 69 || nCalcMethodCode == 70) delta = -delta;
	if (bRemove) delta = -delta;

	float* slot = nullptr;
	switch (buffSourceType) {
		case 0: slot = isPercent ? &entry.fSkillPercentModifier : &entry.fSkillModifier; break;
		case 1: slot = isPercent ? &entry.fItemPercentModifier  : &entry.fItemModifier;  break;
		case 3: slot = isPercent ? &entry.fSelfPercentModifier  : &entry.fSelfModifier;  break;
		case 2:
		case 4:
		default: slot = isPercent ? &entry.fPercentModifier : &entry.fModifier;          break;
	}

	if (slot) {
		const float prev = *slot;
		*slot += delta;
		Logger::Log("inventory",
			"ApplyBuff: pawn=0x%p prop=%d cm=%d amount=%.2f src=%u remove=%lu  slot %.2f -> %.2f  (entry idx=%d)\n",
			Pawn, BuffFilter.nPropId, nCalcMethodCode, fAmount, (unsigned)buffSourceType, bRemove,
			prev, *slot, index);

		// MAX-HP diagnostic: trace every registry mutation that feeds HEALTH_MAX
		// (304 base, 412 modifier, 390 mod) on the single "effects" channel so an
		// add-then-unspec test shows whether each apply has a matching reverse and
		// whether the 412/390 slots return to zero. Dump all four percent layers
		// of the shared entry — armor lands in itemP, skills in skillP.
		if ((BuffFilter.nPropId == 412 || BuffFilter.nPropId == 390 || BuffFilter.nPropId == 304
		     || BuffFilter.nPropId == 256 || BuffFilter.nPropId == 214 || BuffFilter.nPropId == 232)
		    && Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects",
				"[MAXHP/ApplyBuff] pawn=%p prop=%d cm=%d amt=%.2f src=%u remove=%lu  "
				"hdr{cat=%d skill=%d dev=%d} slot %.2f->%.2f  entry[idx=%d] itemP=%.2f skillP=%.2f selfP=%.2f genP=%.2f\n",
				(void*)Pawn, BuffFilter.nPropId, nCalcMethodCode, fAmount, (unsigned)buffSourceType, bRemove,
				BuffFilter.nReqCategoryCode, BuffFilter.nReqSkillId, BuffFilter.nReqDeviceInstId,
				prev, *slot, index,
				entry.fItemPercentModifier, entry.fSkillPercentModifier,
				entry.fSelfPercentModifier, entry.fPercentModifier);
		}
	}

	// Mirror the entry to the owning client. UC's TgPawn::ApplyBuff (stripped
	// here, that's why we reimplement it) ends with a `ClientSendBuff(entry)`
	// RPC — the client uses it to populate its own m_EffectBuffInfo and then
	// runs OnDeviceBuffChange, which is what recomputes things like the
	// crosshair tightness from buffed accuracy. Without this, server-side
	// math is correct but the client never sees the buff (visible symptom:
	// no crosshair change inside a buff station radius).
	//
	// SDK wrapper for ClientSendBuff is safe to use — single 0x30 FBuffInfo
	// parm with no bitfields, so the wrapper's memcpy delivers it intact.
	Pawn->eventClientSendBuff(entry);

	// Server-side mirror of the buff-change notification. The original UC
	// only fires `OnDeviceBuffChange` from inside the `ClientSendBuff` RPC
	// body, which by definition only runs on the owning client. But several
	// pieces of state that `OnDeviceBuffChange → SetTargetAutoLock` recompute
	// are read SERVER-SIDE — most importantly `TgDevice::m_bAutoLock` (the
	// flag that `TgDeviceFire::CalcInstantFire` checks to substitute the
	// locked target for the trace endpoint on heal beams like BioFeedback,
	// Nanite Enhancement/Restoration, and any other "soft-lock" weapon).
	//
	// `m_bAutoLock` has no `CPF_Net` flag (SDK confirms it's at TgDevice
	// +0x22C bit 0x40, plain bitfield) — it's a local mirror of a buff-
	// registry query (prop 200 AutoLockAngle, filtered by device's
	// m_nSkillId), recomputed independently on each side. Without firing
	// `OnDeviceBuffChange` on the server, the server's `m_bAutoLock` stays
	// false, `CalcInstantFire` takes the raw-trace branch, and the heal
	// only registers when the player is aimed directly at the teammate —
	// even though the client visualizes a locked target and shows the heal
	// beam playing. Visuals work, server-side hit doesn't.
	//
	// The original native `TgPawn::ApplyBuff` (stripped @ 0x109bf7b0) almost
	// certainly fired `OnDeviceBuffChange` locally on top of the
	// `ClientSendBuff` RPC: `OnDeviceBuffChange` is declared `simulated
	// event` and `SetTargetAutoLock` is `simulated function` — the
	// "intended to run on both sides" annotations only make sense if the
	// dispatch point was bidirectional, and inside the native is the only
	// bidirectional entry that naturally exists. Mirror that here.
	//
	// OnDeviceBuffChange's body is small (one SetTargetAutoLock call plus a
	// client-only HUD refresh that the if-NetMode-Client gate skips on
	// server). SetTargetAutoLock is one buff-registry GetBuffInfo lookup,
	// so the per-ApplyBuff cost is bounded. No-op for buffs that don't
	// touch prop 200, but cleaner to always call (mirrors the native) than
	// to special-case the propId here.
	Pawn->eventOnDeviceBuffChange();

	// Canonical entry-removal-at-zero (.planning/effect-buff-property-canonical.md
	// §4 Q5): when a reverse brings EVERY modifier on this entry to ~0, drop the
	// entry from m_EffectBuffInfo so a clear-then-reapply leaves the registry
	// byte-identical to before the apply. This is the contract that lets the
	// skill / loadout reapply path run with NO external delta-tracking — without
	// it, reversed entries linger and the producers needed g_appliedSkillDeltas /
	// AppliedBuffRecord to compensate. The client was just sent the zeroed entry
	// above, so its contribution is already cleared. Only check on the reverse
	// path; an entry is shared across source-types (skill+item on one filter), so
	// require ALL eight modifiers to be ~0 before removing. Float residual after
	// +X then -X is < 1e-5; 1e-3 is a safe "is zero" threshold.
	if (bRemove) {
		const FBuffInfo& e2 = Pawn->m_EffectBuffInfo.Data[index];
		auto nz = [](float v) { return v < -1e-3f || v > 1e-3f; };
		const bool anyNonZero =
			nz(e2.fSkillPercentModifier) || nz(e2.fSkillModifier) ||
			nz(e2.fItemPercentModifier)  || nz(e2.fItemModifier)  ||
			nz(e2.fSelfPercentModifier)  || nz(e2.fSelfModifier)  ||
			nz(e2.fPercentModifier)      || nz(e2.fModifier);
		if (!anyNonZero) {
			const int last = Pawn->m_EffectBuffInfo.Num() - 1;
			if (index != last) {
				Pawn->m_EffectBuffInfo.Data[index] = Pawn->m_EffectBuffInfo.Data[last];
			}
			Pawn->m_EffectBuffInfo.Count--;
		}
	}

	// Fold a HEALTH_MAX_MODIFIER (412) buff back into the eager-cached
	// r_nHealthMaximum. Fires on apply AND remove (after the registry reflects
	// the final state above) so max health tracks the buff symmetrically. This
	// replaces ReapplyCharacterSkillTree's old 412→304 direct-m_fRaw-write
	// workaround with the canonical registry path.
	//
	// BOTH modifier props that feed HEALTH_MAX must trigger the recompute.
	// ConvertPropToPropList(ITEM,304) = {412, 390} (verified at 0x109e5220):
	// 412 (HEALTH_MAX_MODIFIER) is what skills use; 390 (HEALTH_MOD) is what
	// blueprint mods use (verified in asm_data_set_effects — prop-390 class-157
	// +10% rolls are 100% blueprint mods). Previously only 412 fired, so a rolled
	// +%maxHP mod changed the registry but never refreshed r_nHealthMaximum until
	// some unrelated 412 change happened to recompute — silently under-applying
	// mod maxHP.
	if (BuffFilter.nPropId == 412 /* HEALTH_MAX_MODIFIER */ ||
	    BuffFilter.nPropId == 390 /* HEALTH_MOD (blueprint maxHP rolls) */) {
		RecomputeEagerBaseProp(Pawn, 304 /* HEALTH_MAX */);
	}

	// Note: we intentionally do NOT refresh r_fRequiredMoralePoints here.
	// `GetRequiredMoralePoints @ 0x10a19910` routes through
	// `GetBuffedProperty(BUFF_DEVICE, 318)`, whose expansion `{357, 385}`
	// folds the player's Output Mod (+70%) into the threshold formula and
	// blows it up to 31000+ once mod kits register their prop-385 entries.
	// The retail design keeps r_fRequiredMoralePoints at 100 — Output Mod
	// participation in morale lives in `MoraleCredit::Award`'s gain-rate
	// multiplier, not in the threshold. Tested 2026-05-19.
}
