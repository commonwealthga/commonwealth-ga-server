#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
// TgPawn__ApplyBuff.hpp includes pch.hpp which provides every SDK type we use
// below (ATgPawn, ATgDeployable, ATgRepInfo_Player, APlayerReplicationInfo,
// UTgEffect, UTgEffectGroup, FBuffHeader).
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Class-name check via GetFullName prefix. SDK StaticClass() is unreliable on
// this server build (see reference_sdk_staticclass_misalignment.md), so we
// substring-match the class's full name like the rest of the codebase does.
static bool IsClass(AActor* a, const char* needle) {
	if (!a || !a->Class) return false;
	const char* n = a->Class->GetFullName();
	return n && strstr(n, needle) != nullptr;
}

// Port of TgPawn::GetCurrentOwnerPawn (native @ 0x109c7eb0) — minus the
// "controlling-pawn override at +0x101c" branch which is for player-controlled
// vehicles/robots and doesn't affect buff classification for the common case.
// Returns the deploying-player pawn for henchman pets, or self.
static ATgPawn* GetCurrentOwnerPawn(ATgPawn* pawn) {
	if (!pawn) return nullptr;
	ATgPawn* owner = pawn->r_Owner;
	return owner ? owner : pawn;
}

// Mirror of `TgEffectBuff.GetBuffType` (unrealscript/TgGame/Classes/
// TgEffectBuff.uc:16-119). Categorises the buff into a TG_BUFF_SOURCE_TYPE
// which `TgPawn::ApplyBuff` uses to bucket the entry in m_EffectBuffInfo.
// The bucket choice changes how `GetBuffedProperty`'s layered formula
// aggregates the value (SELF/OTHER both contribute to the same outer layer
// per TgPawn__ApplyBuff.cpp; SKILL/ITEM/EFFICIENCY get their own multipliers).
//
// Without this, every station-applied buff defaulted to OTHER(4) even when
// the source was the target's own equipped device (m_nType ∈ {261..1104})
// or the target IS the instigator — which produced subtle stacking
// differences vs the original game.
//
// Enum values per `TG_BUFF_SOURCE_TYPE` (TgGame_classes.h:1517-1525):
//   0 SKILL, 1 ITEM, 2 EFFICIENCY, 3 SELF, 4 OTHER
static unsigned char GetBuffType(UTgEffect* effect, AActor* target) {
	UTgEffectGroup* g = effect ? effect->m_EffectGroup : nullptr;
	if (!g) return 4;

	// Group-flag priority — matches UC ordering.
	if (g->m_bEfficiencyEffect) return 2;
	if (g->m_bSkillEffect)      return 0;
	if (g->m_bItemEffect)       return 1;

	// m_nType-based SELF classification. These are all "device fire mode"
	// types where the buff originates from the target's own equipped device
	// (lifetime=0 hold-to-sustain scope/zoom/jetpack-style modifiers).
	switch (g->m_nType) {
		case 261: case 262: case 263: case 265:
		case 266: case 283: case 397: case 759: case 1104:
			return 3;
	}

	AActor* inst = g->m_Instigator;
	if (!inst) return 4;
	if (target == inst) return 3;

	const bool tgtPawn  = IsClass(target, "TgPawn");
	const bool tgtDpl   = IsClass(target, "TgDeployable");
	const bool instPawn = IsClass(inst,   "TgPawn");
	const bool instDpl  = IsClass(inst,   "TgDeployable");

	// ATgRepInfo_Player extends APlayerReplicationInfo (TgGame_classes.h:18033),
	// so the equality comparisons below are implicit upcasts — no cast needed.
	if (tgtPawn && instPawn) {
		ATgPawn* ownT = GetCurrentOwnerPawn((ATgPawn*)target);
		ATgPawn* ownI = GetCurrentOwnerPawn((ATgPawn*)inst);
		if (ownT && ownT == ownI) return 3;
	} else if (tgtPawn && instDpl) {
		auto* dI = (ATgDeployable*)inst;
		auto* pT = (ATgPawn*)target;
		if (dI->r_DRI && dI->r_DRI->r_InstigatorInfo &&
		    dI->r_DRI->r_InstigatorInfo == pT->PlayerReplicationInfo) {
			return 3;
		}
	} else if (tgtDpl && instPawn) {
		auto* dT = (ATgDeployable*)target;
		auto* pI = (ATgPawn*)inst;
		if (dT->r_DRI && dT->r_DRI->r_InstigatorInfo &&
		    dT->r_DRI->r_InstigatorInfo == pI->PlayerReplicationInfo) {
			return 3;
		}
	} else if (tgtDpl && instDpl) {
		auto* dT = (ATgDeployable*)target;
		auto* dI = (ATgDeployable*)inst;
		if (dT->r_DRI && dI->r_DRI &&
		    dT->r_DRI->r_InstigatorInfo &&
		    dT->r_DRI->r_InstigatorInfo == dI->r_DRI->r_InstigatorInfo) {
			return 3;
		}
	}

	return 4;
}

// Mirror of `TgEffectBuff.CanBeApplied` (TgEffectBuff.uc:5-14). The base
// `TgEffect.CanBeApplied` unconditionally returns true for deployable
// targets; TgEffectBuff overrides to check `Target.CanApplyEffects()` —
// which on TgDeployable returns `!m_bInDestroyedState`. Without this check
// we'd apply station auras to deployables in their destroyed state.
//
// Pawn target case: pawn.CanApplyEffects() returns `s_bCanApplyEffects` —
// an in-flight flag toggled during cinematic / death animations. We honor
// it to match the UC contract.
static bool CanBeApplied(AActor* target) {
	if (!target) return false;
	if (IsClass(target, "TgDeployable")) {
		return !((ATgDeployable*)target)->m_bInDestroyedState;
	}
	if (IsClass(target, "TgPawn")) {
		return ((ATgPawn*)target)->s_bCanApplyEffects != 0;
	}
	// Non-Pawn / non-Deployable target — UC base TgEffect.CanBeApplied
	// returns false here. Mirror that.
	return false;
}

void ApplyBuffEffectFromHook(UTgEffect* effect, AActor* target, bool bRemove) {
	if (!effect || !target) return;
	// Buff registry is pawn-only. Per TgEffectBuff.uc:143, non-pawn targets
	// fall through silently — the ApplyBuff body checks `TgPawn(Target) != none`
	// before calling Pawn.ApplyBuff. Class check via GetFullName prefix
	// because SDK StaticClass() is unreliable on this server build (see
	// reference_sdk_staticclass_misalignment.md).
	if (!IsClass(target, "TgPawn")) return;
	ATgPawn* Pawn = (ATgPawn*)target;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	// On apply (only), gate via CanBeApplied — TgEffectBuff.ApplyEffect
	// short-circuits here per TgEffectBuff.uc:186-189. Remove is unconditional
	// (a pawn that became uncanapply between apply and remove still needs
	// the buff entry cleaned up).
	if (!bRemove && !CanBeApplied(target)) {
		return;
	}

	const int   calc           = effect->m_nCalcMethodCode;
	const bool  isPercent      = (calc == 68 || calc == 69);
	const bool  bUseOnInterval = (*(unsigned int*)((char*)effect + 0x48) & 0x01) != 0;

	float fProratedAmount;
	if (!bRemove) {
		// **Use m_fCurrent, not m_fBase.** UC's `TgEffect.ApplyEffect` runs
		// BEFORE SetEffectRep (our entry into ApplyBuffEffectFromHook on the
		// apply path), and along the way it calls `CheckEffectBuffModifier`
		// which scales the value by any buff-on-buff modifiers (e.g. Repair
		// Overcharge skill 613, prop 361 +10% on prop-65 buffs from the
		// FRA's right-click). UC then writes the post-scaling result to
		// `m_fCurrent` (TgEffect.uc:116). Reading `m_fBase` here loses that
		// scaling — the FRA's +40% buff registered as +40% on the turret
		// instead of the expected +44% (1.4× damage instead of 1.44×).
		//
		// `m_fCurrent` is also already prorated (UC applies the
		// `m_fBase / (m_fLifeTime / m_fApplyInterval)` math for aoi effects
		// itself), so we don't need to re-do that here. Fall back to
		// `m_fBase` only when UC apparently didn't run apply (m_fCurrent
		// still 0) — e.g. ProtectionModifier blocked the apply chain, or
		// we're on a different non-SetEffectRep entry path.
		float amt = effect->m_fCurrent;
		if (amt == 0.0f) {
			if (g->m_fLifeTime > 0.0f && g->m_fApplyInterval > 0.0f && bUseOnInterval) {
				amt = effect->m_fBase / (g->m_fLifeTime / g->m_fApplyInterval);
			} else {
				amt = effect->m_fBase;
			}
		}
		fProratedAmount = amt;
		// BuildEffectGroup stores class-157 percent base as fraction (0.30 for
		// +30%); buff formula wants percent (30.0). Same reversal as
		// ReapplyCharacterSkillTree.cpp:339.
		if (isPercent) fProratedAmount *= 100.0f;
	} else {
		// On remove: re-derive from m_fCurrent the same way apply did, instead
		// of relying on a stored value. Apply path no longer overwrites
		// m_fCurrent (see comment block below the ApplyBuff call), so by the
		// time UC's eventRemove fires (and we read m_fCurrent here) it's still
		// UC's fraction form — same as it was at apply time.
		float amt = effect->m_fCurrent;
		if (amt == 0.0f) {
			// Apply path was skipped (e.g. ProtectionModifier zeroed lifetime,
			// Strongest-Wins displaced before our slot got populated). Nothing
			// to reverse — mirrors the m_fCurrent==0 guard in
			// reference_phantom_clone_skipped_apply.md.
			return;
		}
		fProratedAmount = amt;
		// Same percent-conversion the apply path did so the buff registry's
		// remove subtracts the same magnitude the apply added.
		if (isPercent) fProratedAmount *= 100.0f;
	}

	FBuffHeader header{};
	header.nPropId          = effect->m_nPropertyId;
	header.nReqCategoryCode = effect->m_nPropertyValueId;
	header.nReqSkillId      = g->m_nRequiredSkillId;
	header.nReqDeviceInstId = (g->m_nReqDeviceInstanceId > 0) ? g->m_nReqDeviceInstanceId : 0;

	const unsigned char srcType = GetBuffType(effect, target);

	TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, header, calc, fProratedAmount,
	                        /*bRemove=*/(unsigned long)(bRemove ? 1 : 0), srcType);

	// **Do NOT touch effect->m_fCurrent at all.** UC's TgEffect.ApplyEffect
	// writes m_fCurrent = fProratedAmount in fraction form (e.g. 0.5 for +50%
	// PERC_INCREASE) at TgEffect.uc:116. UC's event Remove later reads that
	// same m_fCurrent at TgEffect.uc:615 and feeds it into ApplyToProperty,
	// which for PERC_INCREASE computes `m_fRaw - (m_fBase * m_fCurrent)` —
	// expecting the fraction. We must not poison that.
	//
	// Earlier shape OVERWROTE m_fCurrent here with the percent-form value
	// (0.5 → 50) so the matching Remove could read the same number back. That
	// works for genuine class-157 (TgEffectBuff) effects whose Remove goes
	// through the buff registry — but class-157 is STRIPPED on this binary,
	// so every effect with DB class_res_id=157 instantiates as a plain
	// TgEffect at runtime and inherits the base UC eventRemove that DOES
	// read m_fCurrent into ApplyToProperty. Any false-positive
	// BuffEffectRegistry::IsBuff hit on a class-80 PERC effect then made UC
	// remove `m_fBase * 50 = 24000` instead of `m_fBase * 0.5 = 240`,
	// catastrophically driving the prop's m_fRaw negative.
	//
	// Concrete failure (2026-05-19): stealth-while-jetpacking. Stealth's
	// +50% AirSpeed (effect 12893, class_res_id=80, egId 9315) was routed
	// here via false-positive IsBuff. Override poisoned m_fCurrent to 50.
	// On the next remove UC computed `720 - (480 * 50) = -23280`, clamped
	// Pawn.AirSpeed to zero, and the jetpack lost all thrust for the rest
	// of the session.
	//
	// Fix: the remove path scales-on-read identically to apply (the
	// `if (isPercent) fProratedAmount *= 100` above), so the buff registry
	// sees matching magnitudes on both sides without us mutating UC's
	// m_fCurrent. UC keeps its untouched fraction; we derive our percent
	// locally each time.
	//
	// Also: do NOT pre-zero m_fCurrent. UC sets it to 0 ITSELF on
	// TgEffect.uc:616 after the ApplyToProperty call. Pre-zeroing would
	// race UC's Remove and make `m_fRaw - m_fBase * 0 = m_fRaw` (no-op),
	// leaving m_fRaw permanently elevated by the apply's delta. The
	// 2026-05-14 Crescent jetpack fix learned this.

	Logger::Log("effects",
		"[BUFF-ROUTE] effect=%p egId=%d propId=%d catCode=%d skillId=%d devInst=%d "
		"calc=%d amt=%.3f base=%.3f src=%u %s  pawn=%p\n",
		effect, g->m_nEffectGroupId,
		header.nPropId, header.nReqCategoryCode, header.nReqSkillId, header.nReqDeviceInstId,
		calc, fProratedAmount, effect->m_fBase,
		(unsigned)srcType, bRemove ? "REMOVE" : "APPLY", Pawn);
}
