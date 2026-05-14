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
		if (g->m_fLifeTime > 0.0f && g->m_fApplyInterval > 0.0f && bUseOnInterval) {
			fProratedAmount = effect->m_fBase / (g->m_fLifeTime / g->m_fApplyInterval);
		} else {
			fProratedAmount = effect->m_fBase;
		}
		// BuildEffectGroup stores class-157 percent base as fraction (0.30 for
		// +30%); buff formula wants percent (30.0). Same reversal as
		// ReapplyCharacterSkillTree.cpp:339.
		if (isPercent) fProratedAmount *= 100.0f;
	} else {
		// On remove use the amount we recorded on apply (already in percent
		// form for percent calcs, since we wrote it that way). Note: the
		// native apply path also writes m_fCurrent (in FRACTION for class-157
		// percent), but we OVERWROTE that with PERCENT in our apply hook —
		// so by the time UC's eventRemove fires (and we read m_fCurrent
		// here), it's the PERCENT value our apply hook stored.
		fProratedAmount = effect->m_fCurrent;
		if (fProratedAmount == 0.0f) {
			// Apply path was skipped (e.g. ProtectionModifier zeroed lifetime,
			// Strongest-Wins displaced before our slot got populated). Nothing
			// to reverse — mirrors the m_fCurrent==0 guard in
			// reference_phantom_clone_skipped_apply.md.
			return;
		}
	}

	FBuffHeader header{};
	header.nPropId          = effect->m_nPropertyId;
	header.nReqCategoryCode = effect->m_nPropertyValueId;
	header.nReqSkillId      = g->m_nRequiredSkillId;
	header.nReqDeviceInstId = (g->m_nReqDeviceInstanceId > 0) ? g->m_nReqDeviceInstanceId : 0;

	const unsigned char srcType = GetBuffType(effect, target);

	TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, header, calc, fProratedAmount,
	                        /*bRemove=*/(unsigned long)(bRemove ? 1 : 0), srcType);

	// **Apply path only**: stash the (potentially percent-converted) value so
	// the matching Remove can pass the same number to ApplyBuff. For class-157
	// (TgEffectBuff) we OVERRIDE what UC would otherwise write, switching the
	// stored value from fraction (0.30) to percent (30) — UC's TgEffect base
	// ApplyEffect later runs `m_fCurrent = fProratedAmount` (TgEffect.uc:116)
	// in fraction form, so without our override the Remove would subtract the
	// wrong magnitude.
	//
	// **Do NOT zero m_fCurrent in the Remove path.** Earlier shape did
	// `effect->m_fCurrent = 0.0f` here; that races with UC's `event Remove`,
	// which is dispatched AFTER our intercept and reads m_fCurrent inside
	// `ApplyToProperty(Target, m_nPropertyId, m_fCurrent, true)`
	// (TgEffect.uc:615). Pre-zeroing made UC compute
	// `NewValue = m_fRaw - 0 = m_fRaw` — a silent no-op that left the
	// property's m_fRaw permanently elevated by the apply's delta. UC sets
	// `m_fCurrent = 0` ITSELF on TgEffect.uc:616 after the ApplyToProperty
	// call, so leaving it alone here is correct.
	//
	// Concrete failure (2026-05-14): Crescent jetpack fire effect (egId 26175,
	// class TgEffect, prop 70 +115 AirSpeed) was getting buff-routed via a
	// false-positive in BuffEffectRegistry::IsBuff (stale clone pointer
	// reused by allocator). Each fire pulse: APPLY +115 → m_fRaw 600→715,
	// REMOVE no-op → m_fRaw stays 715 → next APPLY +115 → m_fRaw 715→830 →
	// permanent compounding +115 per pulse over a session. Removing the
	// pre-zero here lets UC's Remove subtract the correct delta even when
	// the false-positive routes us through this hook.
	if (!bRemove) {
		effect->m_fCurrent = fProratedAmount;
	}

	Logger::Log("effects",
		"[BUFF-ROUTE] effect=%p egId=%d propId=%d catCode=%d skillId=%d devInst=%d "
		"calc=%d amt=%.3f base=%.3f src=%u %s  pawn=%p\n",
		effect, g->m_nEffectGroupId,
		header.nPropId, header.nReqCategoryCode, header.nReqSkillId, header.nReqDeviceInstId,
		calc, fProratedAmount, effect->m_fBase,
		(unsigned)srcType, bRemove ? "REMOVE" : "APPLY", Pawn);
}
