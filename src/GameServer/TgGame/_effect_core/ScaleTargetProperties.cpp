#include "src/GameServer/TgGame/_effect_core/ScaleTargetProperties.hpp"
#include "src/GameServer/TgGame/_effect_core/DeviceLookup.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

// ATgPawn::GetBuffedProperty — intact native @ 0x109d7ff0 (flat-arg convention,
// matches the typedef used in TgPawn__ApplyBuff.cpp and TgEffect__CheckEffect-
// BuffModifier.cpp). Runs ConvertPropToPropList(ctx) expansion + the 3-layer
// CheckBuffInfoList formula over the deployer's m_EffectBuffInfo.
typedef void(__fastcall* GetBuffedPropertyFn)(
    ATgPawn*, void* /*edx*/,
    unsigned char /*eRequestContext*/, int /*nPropId*/,
    int /*nReqCategoryCode*/, int /*nReqSkillId*/, int /*nReqDeviceInstId*/,
    int /*bUsePotencyModifier*/, float /*fBaseValue*/, float* /*fBuffedValue*/,
    void* /*Effect*/);
static const GetBuffedPropertyFn GetBuffedPropertyNative =
    (GetBuffedPropertyFn)0x109d7ff0;

// ConvertPropToPropList contexts (the eRequestContext byte).
// Source of truth: `decompiled/.../ATgPawn__ConvertPropToPropList.cpp`
// (intact native @ 0x109e5220). Contexts 1=BUFF_PAWN and 2=BUFF_DEVICE_CTX
// exist in the native but no current QueryPlan needs them; add when used.
static constexpr unsigned char BUFF_DEVICE = 3;
static constexpr unsigned char BUFF_OTHER  = 4;

// Per-prop query plan — which (srcType, query_input) pair to pass to
// GetBuffedProperty when scaling a given target slot. Justified case-by-case
// by ConvertPropToPropList's switch (decompiled @ 0x109e5220); see design doc
// §Canonical adjustments for the citation tables.
struct QueryPlan {
    unsigned char srcType;
    int           queryInputProp;
};

// Deployable: srcType=3 (BUFF_DEVICE) for everything; HP_MAX requires
// query_input=339 because srcType=3 case 304 is pass-through (doesn't fold
// 339), whereas srcType=3 case 339 emits {339, 385} — folding Station Buff
// (skill 795, prop 339) AND Output Mod (385) automatically.
static QueryPlan QueryPlanForDeployableProp(int propId) {
    switch (propId) {
    case 304 /*TGPID_HEALTH_MAX*/:
        return {BUFF_DEVICE, 339 /*HEALTH_MAX_DEPLOYABLES_MODIFIER*/};
    default:
        return {BUFF_DEVICE, propId};
    }
}

// Pet: HP_MAX uses srcType=4 (BUFF_OTHER) + input=366 — that path is
// canonical per ConvertPropToPropList srcType=4 case 366 emitting {366, 385}
// (Pet Max Health Modifier + Output Mod auto-fold). The pet damage path
// (prop 350) is deliberately NOT scaled here — CheckOwnerPetBuff handles
// it at fire time. Range/Accuracy/AoE weapon stats aren't on pet's
// s_Properties (they live on the pet's device fire-mode m_Properties) and
// are handled by BridgeOwnerEntriesToPet below, not by this walk.
static QueryPlan QueryPlanForPetProp(int propId) {
    switch (propId) {
    case 304 /*TGPID_HEALTH_MAX*/:
        return {BUFF_OTHER, 366 /*PET_MAX_HEALTH_MODIFIER*/};
    default:
        return {BUFF_DEVICE, propId};
    }
}

// Bridge owner's pet-specific modifier entries onto pet's m_EffectBuffInfo
// with canonical re-target. Pet weapon-stat queries at fire time
// (UTgDeviceFire::GetPropertyValueById → pet->GetBuffedProperty(BUFF_DEVICE,
// weapon_prop, base) → ConvertPropToPropList(3, weapon_prop)) fold the
// re-targeted prop IDs, scaling the pet's effective range/accuracy/AoE by
// the owner's skills.
//
// Re-target table — each row is justified by ConvertPropToPropList(3, X):
//   381 PET_RANGE_MODIFIER     → 114 RANGE_MODIFIER     (ConvertPropToPropList(3, 5)   emits 114)
//   383 PET_ACCURACY_MODIFIER  → 113 ACCURACY_MODIFIER  (ConvertPropToPropList(3, 10)  emits 113)
//   382 PET_DMG_RADIUS_MOD     → 352 AOE_RADIUS_MOD     (ConvertPropToPropList(3, 6)   emits 352)
//
// Pet damage (350) is NOT bridged — CheckOwnerPetBuff handles it at fire
// time (TgEffectDamage.uc:130). Double-bridging would compound the scale.
// Pet HP_MAX (366) is NOT bridged either — that's covered by the pet
// s_Properties pre-bake via QueryPlanForPetProp(304).
//
// One ApplyBuff per srcType-layer per entry preserves the canonical
// 3-layer compounding semantics (skill × item × generic) on the pet's
// registry.
static void BridgeOwnerEntriesToPet(ATgPawn* owner, ATgPawn* pet,
                                    int sourceDeviceInstId) {
    if (!owner || !pet) return;
    if (owner->m_EffectBuffInfo.Data == nullptr ||
        owner->m_EffectBuffInfo.Num() == 0)
        return;

    auto reTarget = [](int srcPropId) -> int {
        switch (srcPropId) {
        case 381: return 114;  // PET_RANGE        → RANGE_MODIFIER
        case 383: return 113;  // PET_ACCURACY     → ACCURACY_MODIFIER
        case 382: return 352;  // PET_DMG_RADIUS   → AOE_RADIUS_MODIFIER
        default:  return 0;    // not bridged
        }
    };

    int bridged = 0;
    for (int i = 0; i < owner->m_EffectBuffInfo.Num(); ++i) {
        const FBuffInfo& e = owner->m_EffectBuffInfo.Data[i];
        const int targetPropId = reTarget(e.BuffHeader.nPropId);
        if (targetPropId == 0) continue;

        // devInst filter: entry's stored devInst==0 (wildcard, skill) OR
        // matches the spawning device. Skip entries scoped to OTHER devices.
        const int storedDevInst = e.BuffHeader.nReqDeviceInstId;
        if (storedDevInst > 0 && storedDevInst != sourceDeviceInstId) continue;

        // header.nReqDeviceInstId = 0 on the bridged entry — pet's fire-time
        // queries scope by the pet's INTERNAL device's instId (not owner's
        // spawning device's). Wildcard makes the entry match those queries.
        FBuffHeader header{};
        header.nPropId          = targetPropId;
        header.nReqCategoryCode = 0;
        header.nReqSkillId      = 0;
        header.nReqDeviceInstId = 0;

        // One ApplyBuff per layer that's non-zero. ApplyBuff's calc method:
        //   68 PERC_INCREASE → percent slot; 67 ADD → absolute slot.
        // Source-type → slot mapping matches ApplyBuff's switch
        // (0=SKILL, 1=ITEM, 3=SELF, 4=OTHER).
        auto issue = [&](unsigned char srcType, float pct, float abs) {
            if (pct != 0.0f) {
                TgPawn__ApplyBuff::Call(pet, nullptr, header, /*cm=*/68, pct,
                                        /*bRemove=*/0, srcType);
            }
            if (abs != 0.0f) {
                TgPawn__ApplyBuff::Call(pet, nullptr, header, /*cm=*/67, abs,
                                        /*bRemove=*/0, srcType);
            }
        };
        issue(/*ITEM=*/1, e.fItemPercentModifier,  e.fItemModifier);
        issue(/*SKILL=*/0, e.fSkillPercentModifier, e.fSkillModifier);
        issue(/*SELF=*/3,  e.fSelfPercentModifier,  e.fSelfModifier);
        issue(/*OTHER=*/4, e.fPercentModifier,      e.fModifier);

        ++bridged;
        Logger::Log("pet_spawn",
            "[BridgeOwnerEntriesToPet] owner=0x%p pet=0x%p  src prop=%d -> pet prop=%d"
            "  storedDevInst=%d (filter=%d)\n",
            (void*)owner, (void*)pet, e.BuffHeader.nPropId, targetPropId,
            storedDevInst, sourceDeviceInstId);
    }

    if (bridged > 0) {
        Logger::Log("pet_spawn",
            "[BridgeOwnerEntriesToPet] owner=0x%p pet=0x%p  bridged %d entry/entries\n",
            (void*)owner, (void*)pet, bridged);
    }
}

// Shared loop body for the s_Properties pre-bake. `planFor` resolves the
// per-prop query strategy; logTag picks the diagnostic channel. The pet
// overload additionally calls BridgeOwnerEntriesToPet after this walk.
template <typename TargetT>
static void ScaleProperties(ATgPawn* deployer, TargetT* target,
                            int sourceDeviceInstId,
                            QueryPlan (*planFor)(int),
                            const char* logTag) {
    if (!deployer || !target) return;
    if (target->s_Properties.Data == nullptr || target->s_Properties.Num() == 0)
        return;

    const int deviceSkillId =
        DeviceLookup::SkillIdForDevice(deployer, sourceDeviceInstId);

    int touched = 0;
    for (int i = 0; i < target->s_Properties.Num(); ++i) {
        UTgProperty* prop = target->s_Properties.Data[i];
        if (!prop) continue;

        const int       propId = prop->m_nPropertyId;
        const float     base   = prop->m_fBase;
        const QueryPlan plan   = planFor(propId);

        float buffed = base;
        GetBuffedPropertyNative(
            deployer, /*edx=*/nullptr,
            plan.srcType, plan.queryInputProp,
            /*nReqCategoryCode=*/-1,
            /*nReqSkillId=*/deviceSkillId,
            /*nReqDeviceInstId=*/sourceDeviceInstId,
            /*bUsePotencyModifier=*/0,
            /*fBaseValue=*/base,
            /*fBuffedValue=*/&buffed,
            /*Effect=*/nullptr);

        if (buffed == base) continue;

        // SetProperty dispatches via ProcessEvent → native at the intact
        // address (deployable: 0x10a1c940 wrapped by our Detour; pawn:
        // 0x109bf420). Both linear-scan s_Properties for propId, write m_fRaw,
        // and dispatch the intact ApplyProperty cascade for engine-field
        // mirroring. The deployable Detour additionally mirrors HP/HP_MAX/DRI.
        target->SetProperty(propId, buffed);
        ++touched;

        Logger::Log(logTag,
            "[ScaleTargetProperties] target=0x%p propId=%d base=%.3f -> %.3f"
            "  (queryInput=%d srcType=%u devInst=%d skill=%d)\n",
            (void*)target, propId, base, buffed,
            plan.queryInputProp, (unsigned)plan.srcType,
            sourceDeviceInstId, deviceSkillId);
    }

    if (touched > 0) {
        Logger::Log(logTag,
            "[ScaleTargetProperties] target=0x%p deployer=0x%p devInst=%d"
            "  modified %d prop(s)\n",
            (void*)target, (void*)deployer, sourceDeviceInstId, touched);
    }
}

void ScaleTargetProperties::Apply(ATgPawn* deployer, ATgDeployable* target,
                                  int sourceDeviceInstId) {
    ScaleProperties(deployer, target, sourceDeviceInstId,
                    QueryPlanForDeployableProp, "inventory");
}

void ScaleTargetProperties::Apply(ATgPawn* deployer, ATgPawn* target,
                                  int sourceDeviceInstId) {
    ScaleProperties(deployer, target, sourceDeviceInstId,
                    QueryPlanForPetProp, "pet_spawn");
    // Pet weapon stats (Range/Accuracy/AoE on the pet's device fire-mode
    // m_Properties, NOT on the pawn s_Properties walked above) require
    // a separate bridge so the pet's fire-time queries pick up owner skills.
    BridgeOwnerEntriesToPet(deployer, target, sourceDeviceInstId);
}
