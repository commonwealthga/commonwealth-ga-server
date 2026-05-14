#include "src/GameServer/TgGame/TgEffect/CheckEffectBuffModifier/TgEffect__CheckEffectBuffModifier.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeviceCategorySkill.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeployableOriginRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// TgPawn::GetBuffedProperty native @ 0x109d7ff0. Reads the pawn's
// m_EffectBuffInfo TArray and applies the binary's three-layer formula:
//   v1 = base * (1 + ΣIP/100) + ΣIM         (Item)
//   v2 = v1   * (1 + ΣSP/100) + ΣSM         (Skill)
//   v3 = v2   * (1 + (ΣSelfP + ΣP)/100) + ΣSelfM + ΣM   (Self + Generic)
// Returns v3 in *fBuffedValue. fBaseValue stays the input.
//
// Last arg `Effect` is the UTgEffect* doing the query — used by the binary's
// internal filtering (skill/devInst gating).
typedef void(__fastcall* GetBuffedPropertyFn)(
	ATgPawn*, void*,
	unsigned char /*eRequestContext*/,
	int /*nPropId*/,
	int /*nReqCategoryCode*/,
	int /*nReqSkillId*/,
	int /*nReqDeviceInstId*/,
	int /*bUsePotencyModifier*/,
	float /*fBaseValue*/,
	float* /*fBuffedValue*/,
	void* /*Effect*/);
static const GetBuffedPropertyFn GetBuffedPropertyNative =
	(GetBuffedPropertyFn)0x109d7ff0;

// BUFF request context (per binary's ConvertPropToPropList @ FUN_109e5220):
//   1 = BUFF_PAWN  / EFFECT       — effect-on-pawn, full prop expansion
//   2 = BUFF_PAWN  / SELFEFFECT   — self-cast / target-side, narrow expansion
//   3 = BUFF_DEVICE               — fire-mode param (radius, cooldown)
//   4 = BUFF_OTHER                — mostly identity (e.g. {prop} alone), with a
//                                    couple of {prop, 385} hardcoded entries
//                                    (350 Pet Damage, 366) — NOT 65
//
// Canonical query for both damage and non-damage effects is BUFF_PAWN with
// the effect's OWN m_nPropertyId. ConvertPropToPropList resolves the
// expansion list per-effect:
//   prop 51 (HP) on TgEffectDamage  → {65, 385, 214/212/321 by damage type,
//                                       attack-type 184/185, +172/376/etc}
//   prop 51 (HP) on TgEffectHeal    → {330, 385, ...}
//   prop 49 (GroundSpd)             → {66}
//   prop 65 (DAMAGE_MODIFIER)       → {65, 361, +376 if potency}
//   prop 6  (Radius)                → {352}
//
// For damage effects in particular, prop 385 (Heal Output Modifier — used
// universally as the "+X% output" multiplier) and the per-damage-type
// modifiers (214 Range, 212 Melee, 321 AOE) only appear via this expansion.
// Querying BUFF_OTHER/65 instead — what we used to do — returned only entries
// stored against prop 65 itself, so the {385, 214, ...} layer was silently
// dropped from outgoing damage. Concrete impact: Ballista's innate Output
// Mod (eg 26736, prop 385 +70%) and per-shot 'd' kit Damage Mods stored
// against prop 65/214 collapsed to "+9% from kit only" instead of the
// expected ~+90%+ from full expansion. (Diagnosed 2026-05-14.)
//
// `bUsePotencyModifier=1` is always set so prop 376 (Effect Potency
// Modifier) entries layer in — that's how skills like Station Buff scale
// station effects without targeting each individual effect prop. The flag
// only adds prop 376 to the BUFF_PAWN expansion; it has no effect in
// BUFF_OTHER, so it's safe to leave on for every call.
constexpr unsigned char BUFF_PAWN  = 1;

// Resolve a candidate source actor to its owning pawn. Handles three cases:
//   - Already a pawn: cast and return.
//   - TgDeployable / TgDeploy_*: walk Instigator → deploying pawn.
//   - Anything else: return null (no instigator-side scaling possible).
//
// Class-name strstr per reference_sdk_staticclass_misalignment — IsA() is
// unreliable on this server build.
static ATgPawn* ResolveSourcePawn(AActor* inst, ATgDeployable** outDeployable) {
	if (outDeployable) *outDeployable = nullptr;
	if (!inst || !inst->Class) return nullptr;
	const char* cn = inst->Class->GetFullName();
	if (!cn) return nullptr;
	if (strstr(cn, "TgPawn") != nullptr) {
		return (ATgPawn*)inst;
	}
	if (strstr(cn, "TgDeployable") != nullptr || strstr(cn, "TgDeploy_") != nullptr) {
		ATgDeployable* dep = (ATgDeployable*)inst;
		if (outDeployable) *outDeployable = dep;
		APawn* deployer = ((AActor*)inst)->Instigator;
		if (deployer && deployer->Class) {
			const char* dn = deployer->Class->GetFullName();
			if (dn && strstr(dn, "TgPawn") != nullptr) {
				return (ATgPawn*)deployer;
			}
		}
	}
	return nullptr;
}

// Reimplementation of TgEffect::CheckEffectBuffModifier — stripped stub at
// 0x10a6f270.
//
// Single query for every effect class: BUFF_PAWN with the effect's own
// m_nPropertyId, potency expansion enabled. The binary's
// ConvertPropToPropList does the per-effect work — for TgEffectDamage on
// prop 51 it resolves to {65, 385, per-damage-type 214/212/321,
// attack-type 184/185, ...}; for TgEffectHeal on prop 51 to {330, 385,
// ...}; for movement/stealth/sensor effects it resolves to whatever
// modifier prop they layer through. Routing damage through BUFF_OTHER/65
// (the previous shape) excluded prop 385 (Output Mod) and the per-type
// damage modifiers from the sum — concrete failure: Ballista's innate
// Output Mod (eg 26736, +70%) and 'd' kits stored against 65/214 produced
// only +9% scaling instead of the full ~+90%+, so the weapon dealt
// roughly half of expected damage. (Diagnosed 2026-05-14.)
//
// Source-device / skill / category-code resolution mirrors the per-clone
// scaling block we previously had in CloneEffectGroup. For deployable-fired
// effects (medical station heals, sentry mine damage, etc.) UC's InitInstance
// leaves m_nSourceDeviceInstId at 0 because Impact.DeviceModeReference was
// null; recover the deploying-pawn's device instance + skill id from
// DeployableOriginRegistry, which we record at SpawnDeployableActor time.
void __fastcall TgEffect__CheckEffectBuffModifier::Call(UTgEffect* effect, void* /*edx*/, float* NewValue) {
	if (!effect || !NewValue) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	const float origValue = *NewValue;
	if (origValue == 0.0f) return;  // nothing to scale

	AActor* inst = g->m_Instigator;
	if (!inst) return;

	ATgDeployable* depInst = nullptr;
	ATgPawn* srcPawn = ResolveSourcePawn(inst, &depInst);
	if (!srcPawn) return;

	// Single query path for every effect class. ConvertPropToPropList
	// (FUN_109e5220) does the per-effect expansion based on the effect's
	// concrete subclass — see the constant block above for the resolved
	// expansion lists.
	const unsigned char queryCtx    = BUFF_PAWN;
	const int           usePotency  = 1;
	const char*         clsName     = effect->Class ? effect->Class->GetFullName() : nullptr;

	// **Heal-prop normalization.** ConvertPropToPropList only routes
	// `{330 Healing Modifier, 385 Output Mod}` for the prop-51 (HP) and
	// prop-260 (Repair) cases when `IsA(effect, "TgEffectHeal")` is true.
	// Medical Station heals use prop 211 (Missing Health regen), which has
	// NO entry in any context's dispatch table — querying with prop 211
	// returns nothing, dropping every heal-side mod and Output Mod entry on
	// the floor. Diagnosed 2026-05-14 after an earlier "wrong damage path"
	// fix surfaced that Medical Station was still 184/tick instead of the
	// expected 474/tick.
	//
	// Normalize the query propId to 51 for any TgEffectHeal effect so the
	// engine routes to the heal-class branch ({330, 385, +376 if potency}).
	// The actual write-back propId on the effect is unchanged — only the
	// buff-registry-query propId is rewritten. Safe for prop 51 heals
	// (already 51, no change) and prop 260 repair heals (260 already routes
	// to {330, 385} via case 0x104; rewriting to 51 yields the same set).
	int queryPropId = effect->m_nPropertyId;
	if (clsName && strstr(clsName, "TgEffectHeal") != nullptr) {
		queryPropId = 51;
	}

	// Resolve device-instance + category-skill for the buff query's filter
	// chain. Player-fired effects use the effect-group's source device
	// (UC's InitInstance populates m_nSourceDeviceInstId from
	// Impact.DeviceModeReference). Deployable-fired effects override with
	// the deploy-device's identity from DeployableOriginRegistry — the
	// deployable's internal fire-mode device is not in the player's
	// inventory so DeviceCategorySkill::LookupByInstanceId would miss, and
	// rolled mods (registered on the deploy device) wouldn't match.
	int queryDevInst  = g->m_nSourceDeviceInstId;
	int queryCatSkill = DeviceCategorySkill::LookupByInstanceId(srcPawn, queryDevInst);
	if (depInst) {
		// m_Instigator is the deployable directly (repair effects, station
		// damage). Walk to the deploy-device via the deployable's origin.
		DeployableOriginRegistry::Origin origin =
			DeployableOriginRegistry::Lookup(depInst);
		queryDevInst  = origin.spawnDeviceInstId;
		queryCatSkill = origin.spawnDeviceSkillId;
	} else if (queryDevInst == 0) {
		// m_Instigator is the deploying pawn (not the deployable) AND
		// UC's InitInstance left m_nSourceDeviceInstId at 0 because
		// Impact.DeviceModeReference was none. Common for Medical
		// Station's heal chain. Recover from the clone-origin side-map
		// (populated at CloneEffectGroup time from the template→fire-mode
		// → deployable chain).
		DeployableOriginRegistry::Origin origin =
			DeployableOriginRegistry::LookupClone(g);
		if (origin.spawnDeviceInstId != 0) {
			queryDevInst  = origin.spawnDeviceInstId;
			queryCatSkill = origin.spawnDeviceSkillId;
		}
	}

	float buffedValue = origValue;
	GetBuffedPropertyNative(
		srcPawn, /*edx=*/nullptr,
		queryCtx,
		queryPropId,
		/*nReqCategoryCode=*/g->m_nCategoryCode,
		/*nReqSkillId=*/queryCatSkill,
		/*nReqDeviceInstId=*/queryDevInst,
		/*bUsePotencyModifier=*/usePotency,
		/*fBaseValue=*/origValue,
		/*fBuffedValue=*/&buffedValue,
		/*Effect=*/effect);

	if (buffedValue != origValue) {
		*NewValue = buffedValue;
		if (Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects",
				"[CHECK-BUFF-MOD] effect=%p class=%s effPropId=%d queryPropId=%d ctx=%u  %.3f -> %.3f  src=%p egId=%d cat=%d devInst=%d skill=%d\n",
				effect, clsName ? clsName : "<no-class>",
				effect->m_nPropertyId, queryPropId, (unsigned)queryCtx,
				origValue, buffedValue,
				srcPawn, g->m_nEffectGroupId, g->m_nCategoryCode,
				queryDevInst, queryCatSkill);
		}
	}
}
