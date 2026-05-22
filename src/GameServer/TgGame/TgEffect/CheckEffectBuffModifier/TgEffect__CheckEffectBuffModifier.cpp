#include "src/GameServer/TgGame/TgEffect/CheckEffectBuffModifier/TgEffect__CheckEffectBuffModifier.hpp"
#include "src/GameServer/TgGame/_effect_core/OriginResolver.hpp"
#include "src/GameServer/TgGame/_effect_core/DeviceLookup.hpp"
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
