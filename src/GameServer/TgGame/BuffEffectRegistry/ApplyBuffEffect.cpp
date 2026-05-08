#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

static unsigned char BuffSourceTypeFor(UTgEffect* effect) {
	UTgEffectGroup* g = effect ? effect->m_EffectGroup : nullptr;
	if (!g) return 4;
	if (g->m_bEfficiencyEffect) return 2;
	if (g->m_bSkillEffect)      return 0;
	if (g->m_bItemEffect)       return 1;
	return 4; // OTHER — covers station→friendly auras
}

void ApplyBuffEffectFromHook(UTgEffect* effect, AActor* target, bool bRemove) {
	if (!effect || !target) return;
	// Buff registry only applies to pawns. Per TgEffectBuff.uc:143, non-pawn
	// targets fall through silently. Class check via GetFullName prefix
	// because SDK StaticClass() is unreliable on this server build (see
	// reference_sdk_staticclass_misalignment.md).
	const char* targetClassName = target->Class ? target->Class->GetFullName() : nullptr;
	if (!targetClassName || strstr(targetClassName, "TgPawn") == nullptr) return;
	ATgPawn* Pawn = (ATgPawn*)target;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

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

	const unsigned char srcType = BuffSourceTypeFor(effect);

	TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, header, calc, fProratedAmount,
	                        /*bRemove=*/(unsigned long)(bRemove ? 1 : 0), srcType);

	if (!bRemove) {
		effect->m_fCurrent = fProratedAmount;
	} else {
		effect->m_fCurrent = 0.0f;
	}

	Logger::Log("effects",
		"[BUFF-ROUTE] effect=%p egId=%d propId=%d catCode=%d skillId=%d devInst=%d "
		"calc=%d amt=%.3f base=%.3f src=%u %s  pawn=%p\n",
		effect, g->m_nEffectGroupId,
		header.nPropId, header.nReqCategoryCode, header.nReqSkillId, header.nReqDeviceInstId,
		calc, fProratedAmount, effect->m_fBase,
		(unsigned)srcType, bRemove ? "REMOVE" : "APPLY", Pawn);
}
