#include "src/GameServer/TgGame/TgDeviceFire/GetPropertyValueById/TgDeviceFire__GetPropertyValueById.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeviceCategorySkill.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Diagnostic logging gate. Logs only for the propIds we're actively
// investigating (accuracy variants and damage modifier). Set to 0 to silence.
static constexpr bool kLogTrace = true;
static inline bool ShouldTrace(int propId) {
	return propId == 10  || propId == 245 || propId == 249 || propId == 250 ||
	       propId == 113 || propId == 65;
}

// Dump every FBuffInfo entry whose nPropId matches `targetProp` (the modifier
// prop the binary's BuffFormula would search for). For accuracy queries
// (propId 10/245/249/250) the modifier is 113. For prop 65 (DAMAGE_MODIFIER)
// it queries directly. Log the 8 modifier floats so we can see whether the
// slot we wrote is actually populated at the moment the consumer fires.
static void DumpMatchingBuffEntries(ATgPawn* pawn, int targetProp, const char* label) {
	if (!pawn || !pawn->m_EffectBuffInfo.Data) return;
	const int n = pawn->m_EffectBuffInfo.Num();
	for (int i = 0; i < n; ++i) {
		FBuffInfo& e = pawn->m_EffectBuffInfo.Data[i];
		if (e.BuffHeader.nPropId != targetProp) continue;
		Logger::Log("effects",
			"  [BUFFINFO/%s] idx=%d prop=%d cat=%d skill=%d devInst=%d  "
			"SP=%.3f SM=%.3f IP=%.3f IM=%.3f selfP=%.3f selfM=%.3f genP=%.3f genM=%.3f\n",
			label, i,
			e.BuffHeader.nPropId, e.BuffHeader.nReqCategoryCode,
			e.BuffHeader.nReqSkillId, e.BuffHeader.nReqDeviceInstId,
			e.fSkillPercentModifier, e.fSkillModifier,
			e.fItemPercentModifier,  e.fItemModifier,
			e.fSelfPercentModifier,  e.fSelfModifier,
			e.fPercentModifier,      e.fModifier);
	}
}

// Resolve nPropertyId → UTgProperty* via the binary's O(1) TMap lookup at
// pawn+0x400. Routes through TgPawn__GetProperty, whose Call body is now a
// pass-through to the native (the TMap is populated reliably by our
// InitializeProperty — see reference_pawn_property_tmap.md).
static inline UTgProperty* FindPropertyOnPawn(ATgPawn* pawn, int propertyId) {
	if (!pawn) return nullptr;
	return TgPawn__GetProperty::CallOriginal(pawn, nullptr, propertyId);
}

// Same lookup but on a deployable (different s_Properties offset 0x1F0 vs
// 0x3F4 on pawn — we use the SDK accessor so this is type-safe).
static inline UTgProperty* FindPropertyOnDeployable(ATgDeployable* dep, int propertyId) {
	if (!dep || !dep->s_Properties.Data) return nullptr;
	for (int i = 0; i < dep->s_Properties.Num(); ++i) {
		UTgProperty* p = dep->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propertyId) {
			return p;
		}
	}
	return nullptr;
}

// True if the actor's UC class chain includes "TgDeployable". Class-name prefix
// match — SDK StaticClass() is unreliable on server (memory:
// reference_sdk_staticclass_misalignment).
static bool IsTgDeployable(UObject* obj) {
	if (!obj || !obj->Class) return false;
	const char* fullName = obj->Class->GetFullName();
	if (!fullName) return false;
	// fullName has format "Class TgGame.TgDeployable" or subclass.
	return strstr(fullName, "TgDeployable") != nullptr ||
	       strstr(fullName, "TgDeploy_")    != nullptr;
}

// Modifier application for fire-mode params: when the cached-index path runs
// (CallOriginal with valid index + populated m_Properties), the binary's
// GetPropertyValueById walks to the pawn and calls GetBuffedProperty itself,
// applying the layered Item/Skill/Self/Generic formula from m_EffectBuffInfo.
// Rolled mods are populated by Inventory::ApplyRolledModEffects → our
// reimplemented TgPawn::ApplyBuff; runtime auras (e.g. Techro buff station →
// friendly accuracy) by ApplyBuffEffectFromHook in the SetEffectRep hook.
//
// In our environment the cached path almost never runs — TgDevice::
// ApplyInventoryEquipEffects is stripped, so m_nXxxIndex caches stay -1 and
// m_Properties stays empty. We then fall through to the s_Properties scan
// below, which used to return raw m_fRaw without any buff math — leaving the
// buff registry's entries dead even though they were populated correctly.
//
// `ApplyBuffsToBaseValue` (below) plugs that gap: after we resolve a base
// from s_Properties, run the same BUFF_DEVICE math the cached path would have
// done. Without this, station accuracy/damage modifiers (prop 113, prop 65,
// …) silently no-op even though their FBuffInfo entries are in place.

// Native TgPawn::GetBuffedProperty @ 0x109d7ff0 — direct call avoids the
// SDK wrapper's ProcessEvent round-trip (cheaper, and stays out of the
// diagnostic [PE] noise). Signature derived from the decompile:
//   void __thiscall(this, byte ctx, int prop, int reqCat, int reqSkill,
//                   int reqDevInst, int bUsePotency, float base,
//                   float* out, void* effectPtr)
typedef void(__fastcall* GetBuffedPropertyFn)(ATgPawn*, void*, unsigned char,
                                              int, int, int, int, int, float,
                                              float*, void*);
static const GetBuffedPropertyFn GetBuffedPropertyNative =
    (GetBuffedPropertyFn)0x109d7ff0;

static float ApplyBuffsToBaseValue(ATgPawn* pawn, int propertyId, float baseVal,
                                   int firingDeviceInstId,
                                   int firingDeviceCategorySkillId) {
    if (!pawn) return baseVal;
    float out = baseVal;
    // Context 3 = BUFF_DEVICE — same context the binary's cached-index path
    // uses. ConvertPropToPropList(BUFF_DEVICE, prop) expands the input prop
    // to its modifier counterpart (e.g. 245 Accuracy-Walk → 113 Accuracy
    // Modifier; 6 DamageRadius → 352 AOE Radius Modifier).
    //
    // `firingDeviceInstId` filters by device: per FUN_109cd890 search-mode,
    // an entry stored with devInst > 0 only matches when the query devInst
    // equals it; entries stored with devInst == 0 are wildcards and match
    // any query. So this query picks up:
    //   - the firing device's own rolled-mod entries (stored devInst = X)
    //   - skill-source entries (stored devInst = 0, wildcard)
    //   - cross-pawn aura buffs (stored devInst = 0, wildcard)
    // and skips OTHER equipped devices' rolled-mod entries (stored
    // devInst = Y ≠ X) — fixing the prop-385 (Output Mod) accumulation
    // pathology where 9 equipped devices each contributed 70-75%.
    //
    // `firingDeviceCategorySkillId` filters by device class. Skills like
    // Heavy Impact / Jetpack Power / Super Engineer write per-row entries
    // with `nReqSkillId = required_skill_id` (e.g., 276 Assault Guns,
    // 360 Medic Jetpack). Passing the firing device's classifying skillId
    // here means only the matching row contributes — non-matching rows
    // silently miss per the search rule. 0 = wildcard / unknown device.
    GetBuffedPropertyNative(pawn, /*edx=*/nullptr,
                            /*eRequestContext=*/3,
                            /*nPropId=*/propertyId,
                            /*nReqCategoryCode=*/0,
                            /*nReqSkillId=*/firingDeviceCategorySkillId,
                            /*nReqDeviceInstId=*/firingDeviceInstId,
                            /*bUsePotencyModifier=*/0,
                            /*fBaseValue=*/baseVal,
                            /*fBuffedValue=*/&out,
                            /*Effect=*/nullptr);
    return out;
}

float __fastcall TgDeviceFire__GetPropertyValueById::Call(UTgDeviceFire* DeviceFire, void* /*edx*/, int nPropertyId, int nPropertyIndex) {
	if (!DeviceFire) return 0.0f;

	// Walk m_Owner to find the right s_Properties source.
	//
	// m_Owner can be one of:
	//   • TgDevice — player-held weapon. s_Properties live on Instigator (player pawn).
	//   • TgDeployable — bomb/turret/sensor. s_Properties live on the deployable itself.
	//     For these the player who placed it is in m_Owner→Instigator (offset 0xD4),
	//     used only for the rolled-mod scaling that already baked into the
	//     deployable's s_Properties via ApplyPlayerModsToDeployable at spawn.
	//
	// Same trick the binary uses: classify owner type, route fallback accordingly.
	ATgPawn*       pawn               = nullptr;
	ATgDeployable* deployable         = nullptr;
	int            firingDeviceInstId = 0;  // 0 = wildcard (deployable / unknown owner)
	int            firingDeviceCatSkill = 0;  // 0 = wildcard (deployable / unknown)
	if (DeviceFire->m_Owner) {
		if (IsTgDeployable((UObject*)DeviceFire->m_Owner)) {
			deployable = (ATgDeployable*)DeviceFire->m_Owner;
			// Pawn = the player who placed the deployable (Instigator @ +0xD4).
			pawn = (ATgPawn*)deployable->Instigator;
			// Deployable fire-modes don't tag to a player-side device instId.
			// Their rolled-mod scaling is already baked into the deployable's
			// s_Properties at spawn (ApplyPlayerModsToDeployable), so leaving
			// the query as wildcard here just picks up the placer's wildcard
			// entries (skills, station auras) on top. Same reasoning for the
			// classifying skillId — leave at 0 so any device-class skill the
			// player has registers.
			firingDeviceInstId   = 0;
			firingDeviceCatSkill = 0;
		} else {
			ATgDevice* device    = (ATgDevice*)DeviceFire->m_Owner;
			pawn                 = (ATgPawn*)device->Instigator;
			firingDeviceInstId   = device->r_nDeviceInstanceId;
			// Map device id → classifying skill id so multi-row skills
			// (Jetpack Power, Heavy Impact, Super Engineer, …) deliver the
			// correct row. Looked up from asm_data_set_items.skill_id.
			firingDeviceCatSkill = DeviceCategorySkill::Lookup(device->r_nDeviceId);
		}
	}

	// Runtime gate: even when ShouldTrace matches a watched propId, only pay
	// the diagnostic cost when the "effects" channel is actually enabled.
	const bool trace = kLogTrace && ShouldTrace(nPropertyId) && Logger::IsChannelEnabled("effects");
	const int  buffCount = trace && pawn ? pawn->m_EffectBuffInfo.Num() : -1;

	float val = 0.0f;
	bool resolved = false;
	float rawAtIdx = -1.0f;
	float cachedRet = -1.0f;
	bool  cachedRan = false;

	// First: try the cached-index path. If something else (now or later) ever
	// populates the per-fire-mode m_Properties + cached indices, the original
	// path is faster and supports buff-modifier hooks. Skip it when the index
	// is the UC-default -1, where the original always returns 0.0.
	if (nPropertyIndex >= 0 && nPropertyIndex < DeviceFire->m_Properties.Num()) {
		// Snapshot m_fRaw on the prop at idx, BEFORE the binary applies its
		// own buff math, so we can compare cached vs. raw and tell whether
		// the binary's GetBuffedProperty inside actually changed anything.
		UTgProperty* p = DeviceFire->m_Properties.Data
		                 ? DeviceFire->m_Properties.Data[nPropertyIndex] : nullptr;
		rawAtIdx = p ? p->m_fRaw : -1.0f;
		float cachedVal = CallOriginal(DeviceFire, nullptr, nPropertyId, nPropertyIndex);
		cachedRet = cachedVal;
		cachedRan = true;
		if (cachedVal != 0.0f) {
			val = cachedVal;
			resolved = true;
		}
	}

	if (!resolved) {
		// Fallback resolution by propId, prefer the deployable's s_Properties
		// for deployable-spawned fire modes (where ApplyPlayerModsToDeployable
		// already baked rolled-mod scaling into the deployable's prop slot).
		// Falls through to the pawn's s_Properties otherwise.
		UTgProperty* prop = nullptr;
		if (deployable) prop = FindPropertyOnDeployable(deployable, nPropertyId);
		if (!prop && pawn) prop = FindPropertyOnPawn(pawn, nPropertyId);
		if (!prop) {
			if (trace) {
				Logger::Log("effects",
					"[GETPROP] devFire=%p propId=%d idx=%d  cachedRan=%d cachedRet=%.3f rawAtIdx=%.3f  fallback: NO PROP found  pawn=%p buffCount=%d  -> 0.0\n",
					DeviceFire, nPropertyId, nPropertyIndex,
					cachedRan ? 1 : 0, cachedRet, rawAtIdx, pawn, buffCount);
			}
			return 0.0f;
		}
		val = prop->m_fRaw;
		// Match the clamp-to-[min,max] the original applies when both bounds are
		// non-zero (m_fMaximum > epsilon AND m_fMinimum <= m_fMaximum).
		if (prop->m_fMaximum > 0.0f && prop->m_fMinimum <= prop->m_fMaximum) {
			if (val < prop->m_fMinimum) val = prop->m_fMinimum;
			else if (val > prop->m_fMaximum) val = prop->m_fMaximum;
		}
	}

	const float preBuffVal = val;

	// Apply BUFF_DEVICE buff-registry math ONLY on the fallback path, where
	// the binary's GetPropertyValueById short-circuited to 0 (idx<0 or
	// m_Properties empty) and never reached its internal vtable[0x55c]
	// (GetBuffedProperty) call. When the cached path ran (resolved=true),
	// the binary already applied buff math from m_EffectBuffInfo and our
	// manual call would double-apply.
	//
	// Skip for deployable owners regardless — the placer's rolled mods are
	// already baked into the deployable's s_Properties by
	// ApplyPlayerModsToDeployable at spawn.
	if (!resolved && !deployable && pawn) {
		val = ApplyBuffsToBaseValue(pawn, nPropertyId, val, firingDeviceInstId,
		                            firingDeviceCatSkill);
	}

	if (trace) {
		Logger::Log("effects",
			"[GETPROP] devFire=%p propId=%d idx=%d  cachedRan=%d cachedRet=%.3f rawAtIdx=%.3f  resolved=%d preBuff=%.3f postBuff=%.3f  pawn=%p buffCount=%d deployable=%p\n",
			DeviceFire, nPropertyId, nPropertyIndex,
			cachedRan ? 1 : 0, cachedRet, rawAtIdx,
			resolved ? 1 : 0, preBuffVal, val,
			pawn, buffCount, deployable);
		// Show the entries that the binary's BuffFormula *should* have found
		// for this query. ConvertPropToPropList(BUFF_DEVICE) maps:
		//   10 / 245 / 246 / 249 → 113   (ACCURACY → ACCURACY_MODIFIER)
		//   65 → 65 (no expansion documented; check raw entries with prop=65)
		// Anything else → identity. If the dump shows nonzero modifier slots
		// matching what we wrote in [BUFF-ROUTE], the entries ARE there and
		// the binary's formula is silently dropping them.
		if (pawn) {
			int target = nPropertyId;
			if (nPropertyId == 10 || nPropertyId == 245 || nPropertyId == 246 || nPropertyId == 249) target = 113;
			DumpMatchingBuffEntries(pawn, target, "trace");
		}
	}

	return val;
}
