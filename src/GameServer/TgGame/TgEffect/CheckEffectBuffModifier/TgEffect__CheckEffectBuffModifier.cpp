#include "src/GameServer/TgGame/TgEffect/CheckEffectBuffModifier/TgEffect__CheckEffectBuffModifier.hpp"
#include "src/GameServer/TgGame/_effect_core/OriginResolver.hpp"
#include "src/GameServer/TgGame/_effect_core/DeviceLookup.hpp"
#include "src/GameServer/TgGame/_effect_core/HitSituationalMitigation.hpp"
#include "src/GameServer/TgGame/_effect_core/CleanseTracking.hpp"
#include "src/GameServer/TgGame/_effect_core/EffectCredit.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of TgEffect::CheckEffectBuffModifier — stripped stub at
// 0x10a6f270. Scales an effect's outgoing value (*NewValue, the damage/heal
// magnitude) by the SOURCE pawn's buff registry, via the intact
// GetBuffedProperty 3-layer formula. See .planning/effect-buff-property-canonical.md
// §2 (formula + ConvertPropToPropList expansion) and §4 (Q2/Q3 origin/device).
//
// Clean-room rebuild notes:
//   * Device + skill origin resolved canonically via OriginResolver
//     (m_nSourceDeviceInstId for player/pet, or the deployable→spawner-device
//     walk) and DeviceLookup — NO DeployableOriginRegistry / DeviceCategorySkill
//     side-maps.
//   * Source pawn (whose buffs scale the output) resolved via class-name, not
//     IsA(), through ObjectClassCache.
//   * Single query for every effect class: BUFF_PAWN with the effect's own
//     property; ConvertPropToPropList (intact) expands per-effect — for damage
//     to {65, 385, per-attack-type 212/214/321, …}, for heal to {330, 385}.
//     Heal effects query on prop 51: that is the canonical heal-output key
//     (only 51+heal-class expands to {330,385}; the effect's literal target
//     prop is 211 Missing-Health / 260 Repair, which have no expansion). This
//     is the heal-scaling contract, not a 211-specific special case.

// TgPawn::GetBuffedProperty — intact native @ 0x109d7ff0.
typedef void(__fastcall* GetBuffedPropertyFn)(
	ATgPawn*, void*,
	unsigned char /*eRequestContext*/, int /*nPropId*/,
	int /*nReqCategoryCode*/, int /*nReqSkillId*/, int /*nReqDeviceInstId*/,
	int /*bUsePotencyModifier*/, float /*fBaseValue*/, float* /*fBuffedValue*/,
	void* /*Effect*/);
static const GetBuffedPropertyFn GetBuffedPropertyNative = (GetBuffedPropertyFn)0x109d7ff0;

// ConvertPropToPropList context 1 = BUFF_PAWN (full per-effect expansion).
// bUsePotencyModifier=1 adds prop 376 (Effect Potency) to the BUFF_PAWN
// expansion so Station-Buff-style skills layer in; no effect in other contexts.
static constexpr unsigned char BUFF_PAWN = 1;

// Resolve a candidate instigator to the pawn whose buff registry scales the
// output: a pawn directly, or the deploying pawn behind a deployable
// (m_Instigator on the deployable). Class-name check (no IsA) via the cache.
static ATgPawn* ResolveSourcePawn(AActor* inst) {
	if (!inst) return nullptr;
	if (ObjectClassCache::ClassNameContains(inst, "TgPawn")) {
		return static_cast<ATgPawn*>(inst);
	}
	if (ObjectClassCache::ClassNameContains(inst, "Deployable")) {
		APawn* deployer = inst->Instigator;
		if (deployer && ObjectClassCache::ClassNameContains(deployer, "TgPawn")) {
			return static_cast<ATgPawn*>(deployer);
		}
	}
	return nullptr;
}

void __fastcall TgEffect__CheckEffectBuffModifier::Call(UTgEffect* effect, void* /*edx*/, float* NewValue) {
	if (!effect || !NewValue) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	// Type-505 application capture (devusage) — settles what a skill-sourced
	// situational instance actually carries at apply time. The KI/Eagle-Eye
	// finding (theorycraft-console: killer-instinct-diag.md §3) shows skill
	// groups resolve origin differently from device groups (KI's potency
	// query "finds nothing"), so Group Heal Savior (eg 16587) attribution —
	// which delivering wave, if any — must be read from a live capture, not
	// assumed. Rare events (situational skill triggers only); one line each.
	if (g->m_nType == 505 && Logger::IsChannelEnabled("devusage")) {
		Logger::Log("devusage",
			"[505-APPLY] egId=%d sit=%d prop=%d val=%.1f inst=%p srcInstId=%d srcSkillId=%d target=%p\n",
			g->m_nEffectGroupId, g->m_nSituationalType, effect->m_nPropertyId,
			*NewValue, (void*)g->m_Instigator, g->m_nSourceDeviceInstId,
			g->m_nSourceDeviceSkillId, (void*)g->m_Target);
	}

	// Group Heal Savior (eg 16587) trigger → SAVIOR match event, with the
	// delivering device. Single-cast tests (instances 207/208, 2026-08-04)
	// settled the instance's m_nSourceDeviceInstId: it IS the delivering
	// group-heal's inventory id, reliably — the earlier "wrong device"
	// reading came from mis-grouping a multi-cast log. Those same tests
	// showed Triage Wave does NOT proc this skill (its own 505 fires
	// instead), so Triage rescues are covered by eg 22375, not here.
	// Keyed on the prop-155 effect so the group's second effect
	// (GroundSpeed) doesn't double-fire the event.
	if (g->m_nEffectGroupId == 16587 && effect->m_nPropertyId == 155) {
		ATgPawn* saveTarget =
			(g->m_Target && ObjectClassCache::ClassNameContains(g->m_Target, "TgPawn"))
				? static_cast<ATgPawn*>(g->m_Target) : nullptr;
		ATgPawn* saveCredit = EffectCredit::ResolveCreditPawn(g->m_Instigator);
		MatchStats::OnSaviorTrigger(saveCredit, saveTarget,
			EffectCredit::ResolveDeviceId(g, saveCredit));
	}

	// Self-shot mitigation bracket. This native is the last thing that runs on
	// an effect before its value reaches the target, which makes it the one
	// place that sees both halves of the type-505 ordering defect at the right
	// moment: the debuff at TgEffect.uc:115 (before ApplyToProperty writes it)
	// and the damage at TgEffectDamage.uc:131 (before ProtectionModifier reads
	// it). Both calls come first so the early-outs below can't skip them.
	// See HitSituationalMitigation.hpp.
	const bool isDamage = ObjectClassCache::ClassNameContains(effect, "TgEffectDamage");
	if (isDamage) {
		HitSituationalMitigation::BeginImpactMitigation(effect);
	} else {
		HitSituationalMitigation::NoteDebuffApplied(effect);
		// Property-140 "Remove Effect": capture cleanse provenance before
		// ApplyEffect routes to RemoveEffectGroupsByCategory (which never
		// sees the instigator). No-op for every other property.
		CleanseTracking::NotePendingCleanse(effect);

		// Property-243 Power Pool restore (Power Wave 26097/16921, Triage
		// Wave's conditional half 22375, …). Runs before ApplyToProperty, so
		// the target's pool is still pre-restore — split the pre-buff-scale
		// value into restored vs. wasted the same way the heal clamp does.
		// Triage Wave carries power ONLY in its HP-gated group, so a nonzero
		// power_restored on it doubles as "the conditional half fired".
		if (effect->m_nPropertyId == 243 && *NewValue > 0.0f &&
		    g->m_Target && ObjectClassCache::ClassNameContains(g->m_Target, "TgPawn")) {
			ATgPawn* tp = static_cast<ATgPawn*>(g->m_Target);
			float missing = tp->r_fMaxPowerPool - tp->r_fCurrentPowerPool;
			if (missing < 0.0f) missing = 0.0f;
			const float restored = *NewValue < missing ? *NewValue : missing;
			ATgPawn* credit = EffectCredit::ResolveCreditPawn(g->m_Instigator);
			MatchStats::OnDevicePowerRestore(credit,
				EffectCredit::ResolveDeviceId(g, credit),
				(int)restored, (int)(*NewValue - restored));
		}
	}

	const float origValue = *NewValue;
	if (origValue == 0.0f) return;  // nothing to scale

	ATgPawn* srcPawn = ResolveSourcePawn(g->m_Instigator);
	if (!srcPawn) return;

	// Heal effects scale on the canonical heal-output key (prop 51); every
	// other class queries on its own property and lets ConvertPropToPropList
	// expand it.
	int queryPropId = effect->m_nPropertyId;
	const bool isHeal = ObjectClassCache::ClassNameContains(effect, "TgEffectHeal");
	if (isHeal) queryPropId = 51;

	// Canonical device + class-skill origin (replaces the two side-maps).
	OriginResolver::DeviceOrigin origin = OriginResolver::Resolve(g);
	int queryDevInst  = origin.instId;
	int queryCatSkill = origin.skillId;
	if (queryCatSkill == 0 && queryDevInst != 0) {
		// Origin gave a device instance but no skill id (e.g. group carried
		// only the inst id) — recover the class-skill from the equipped device.
		queryCatSkill = DeviceLookup::SkillIdForDevice(srcPawn, queryDevInst);
	}

	float buffedValue = origValue;
	GetBuffedPropertyNative(
		srcPawn, /*edx=*/nullptr,
		BUFF_PAWN,
		queryPropId,
		/*nReqCategoryCode=*/g->m_nCategoryCode,
		/*nReqSkillId=*/queryCatSkill,
		/*nReqDeviceInstId=*/queryDevInst,
		/*bUsePotencyModifier=*/1,
		/*fBaseValue=*/origValue,
		/*fBuffedValue=*/&buffedValue,
		/*Effect=*/effect);

	if (buffedValue != origValue) {
		*NewValue = buffedValue;
		if (Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects",
				"[CHECK-BUFF-MOD] effect=%p class=%s effPropId=%d queryPropId=%d  %.3f -> %.3f  src=%p egId=%d cat=%d devInst=%d skill=%d\n",
				effect, ObjectClassCache::GetClassName(effect).c_str(),
				effect->m_nPropertyId, queryPropId,
				origValue, buffedValue,
				srcPawn, g->m_nEffectGroupId, g->m_nCategoryCode,
				queryDevInst, queryCatSkill);
		}
	}
}
