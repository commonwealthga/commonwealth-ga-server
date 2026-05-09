#include "src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/ConstructEffectGroup/TgEffectGroup__ConstructEffectGroup.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeviceCategorySkill.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeployableOriginRegistry.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// ConstructObject<UTgEffect> — same call site BuildEffectGroup uses to allocate
// fresh effect instances. UClass*, then 8 unused args, returns the new object.
typedef void*(__cdecl* ConstructEffectFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static ConstructEffectFn ConstructEffect = (ConstructEffectFn)0x10a73f30;

// Per-class instance size for our supported effect classes (from SDK header).
// Used by CloneEffectField to know how many bytes to memcpy from template→clone.
//
// Memcpying past PropertiesSize would write into the next allocation. Looking
// up via UClass.PropertiesSize at runtime would be more general but the SDK
// hides UStruct internals behind unknown-data; the table is small and the set
// of classes we ever construct is fixed (see GetEffectClassById).
static int GetEffectInstanceSize(UTgEffect* effect) {
	if (!effect || !effect->Class) return 0;
	const char* name = effect->Class->GetFullName();
	if (!name) return 0;
	if (strstr(name, "TgEffectDamage") != nullptr)     return 0x80;
	if (strstr(name, "TgEffectHeal") != nullptr)       return 0x74;
	// TgEffectSensor, TgEffectVisibility, TgEffect — all 0x70.
	return 0x70;
}

// UTgEffectGroup::CloneEffectGroup — original native @ 0x10a6f3c0 is an empty
// stub. UC's TgEffectManager::GetClonedEffectGroup → eg.CloneEffectGroup()
// expects a fresh independent instance: the same template feeds multiple
// targets (DoT/HoT/buffs across players), so returning self aliased the
// template and stomped m_Target / s_OwnerEffectManager / timer state across
// callers.
//
// Effects are cloned independently too. UC declares `native function TgEffect
// CloneEffect();` (also stripped) and the original CloneEffectGroup almost
// certainly walked m_Effects calling CloneEffect on each — `m_fCurrent` lives
// on the effect, and Apply/Remove math depends on per-application bookkeeping.
// If we shared template effect pointers across clones, two simultaneous
// applications (two buff stations / overlapping pulses) would BOTH write to
// the same `m_fCurrent`, and the first reversal would zero it — the second
// reversal then sees `m_fCurrent==0`, hits the phantom-clone skip in
// `TgEffectGroup__RemoveEffects`, and the buff registry / s_Properties stays
// stuck at `+amount` permanently. Verified against effects.txt: BUFF-ROUTE
// APPLY count exceeds REMOVE count by ≥1 per player per buff-station session,
// and the player ends with `genM=10.000` residual on prop 113 long after
// leaving the radius. Same shape causes the protection prop 155 buff to stack
// across multiple stations and never go away.
//
// Field copy strategy: split memcpy around the only TArray (m_Effects @ 0x60).
// The ctor-linked fresh instance has m_Effects = {nullptr,0,0}; we re-Add each
// cloned UTgEffect* so the clone owns an independent TArray container with
// its own per-clone effect copies. FName / FImpactInfo / FPointer fields in
// the second range are inline storage — shallow memcpy is correct.
UTgEffectGroup* __fastcall TgEffectGroup__CloneEffectGroup::Call(UTgEffectGroup* eg, void* /*edx*/) {
	if (!eg) {
		return nullptr;
	}

	UClass* effectGroupClass = (UClass*)ClassPreloader::GetClass("Class TgGame.TgEffectGroup");
	UTgEffectGroup* clone = (UTgEffectGroup*)
		TgEffectGroup__ConstructEffectGroup::CallOriginal(
			effectGroupClass, -1, 0, 0, 0, 0, 0, 0, nullptr);

	if (!clone) {
		// Fall back to legacy aliased-self behavior so callers still get a
		// non-null group rather than crashing in GetClonedEffectGroup.
		eg->m_bHasVisual = 1;
		return eg;
	}

	clone->ObjectFlags.A |= 0x4000;  // RF_RootSet — keep alive past UC GC sweeps

	uint8_t* dst = reinterpret_cast<uint8_t*>(clone);
	uint8_t* src = reinterpret_cast<uint8_t*>(eg);

	// Range 1: 0x3C..0x60 (everything before m_Effects)
	std::memcpy(dst + 0x3C, src + 0x3C, 0x60 - 0x3C);
	// Range 2: 0x6C..0x140 (everything after m_Effects, includes FImpactInfo /
	// FName / FPointer — all inline storage, safe to shallow-copy)
	std::memcpy(dst + 0x6C, src + 0x6C, 0x140 - 0x6C);

	// Independent TArray container. Effects must ALSO be cloned per-instance
	// (not shared with template) — see top-of-file comment for the
	// shared-m_fCurrent failure mode.
	clone->m_Effects.Data = nullptr;
	clone->m_Effects.Count = 0;
	clone->m_Effects.Max = 0;
	for (int i = 0; i < eg->m_Effects.Count; ++i) {
		UTgEffect* tmplEffect = eg->m_Effects.Data[i];
		if (!tmplEffect) {
			clone->m_Effects.Add(nullptr);
			continue;
		}

		int instSize = GetEffectInstanceSize(tmplEffect);
		if (instSize <= 0x3C) {
			Logger::Log("effects",
				"[CLONE] effect[%d] unknown class size for %s — sharing template ptr (legacy)\n",
				i, tmplEffect->Class ? tmplEffect->Class->GetFullName() : "<no-class>");
			clone->m_Effects.Add(tmplEffect);
			continue;
		}

		UTgEffect* effClone = (UTgEffect*)
			ConstructEffect(tmplEffect->Class, -1, 0, 0, 0, 0, 0, 0, nullptr);
		if (!effClone) {
			Logger::Log("effects",
				"[CLONE] effect[%d] ConstructEffect failed — sharing template ptr (legacy)\n", i);
			clone->m_Effects.Add(tmplEffect);
			continue;
		}

		// Memcpy fields 0x3C..instSize. Skips UObject header (which the ctor
		// already filled in for the new instance — name, Outer, Class, etc.)
		// and stays within the allocated size for this concrete subclass.
		std::memcpy((uint8_t*)effClone + 0x3C, (uint8_t*)tmplEffect + 0x3C,
			instSize - 0x3C);

		// Wire the cloned effect to the new clone group, not the template —
		// otherwise CalcCategoryProtection / m_EffectGroup.GetProperty etc.
		// would dereference the template's m_Target / m_Instigator (which
		// aren't set per-instance) and read stale state.
		effClone->m_EffectGroup = clone;

		// Force m_bRemovable to its CDO default (true, bit 0x02 at +0x48).
		// Memcpy above copied flags from the template; if any prior code path
		// cleared this, the clone would inherit it. Safe to set unconditionally
		// — see BuildEffectGroup's identical reasoning.
		unsigned int& eflags = *(unsigned int*)((char*)effClone + 0x48);
		eflags |= 0x02;

		// Effect classes 157 (TgEffectBuff) are flagged in the side-set on
		// the TEMPLATE effect during BuildEffectGroup; the clone needs the
		// same flag so SetEffectRep's apply-route + ProcessEvent's Remove
		// intercept both match against IsBuff(clone-effect).
		if (BuffEffectRegistry::IsBuff(tmplEffect)) {
			BuffEffectRegistry::Mark(effClone);
		}

		// Don't RootSet the cloned effect: while the clone group is alive
		// (RootSet'd above) and references this effect via m_Effects, UE3
		// GC keeps it. When the group is removed from s_AppliedEffectGroups
		// and eventually unreferenced, GC will collect the cloned effect.
		// RootSet'ing it here would leak.

		// Damage-buff (prop 65) scaling — performed HERE, not in any
		// CheckEffectBuffModifier or TakeDamage hook. Reasoning:
		//
		// UC's `CheckEffectBuffModifier` is a stripped native; UC-to-UC
		// bytecode dispatches it via inline VM (ProcessInternal → Func),
		// which bypasses our ProcessEvent hook entirely. UC's
		// `Target.TakeDamage(int(fProratedAmount), …)` is *also* dispatched
		// inline (FUNC_Event flag does NOT route UC-to-UC calls through
		// ProcessEvent, despite folklore — verified by zero combat-trace
		// `PE: …TakeDamage…` lines across full damage cycles). Both hook
		// approaches were dead.
		//
		// Working approach: pre-scale the *cloned* damage effect's m_fBase
		// (and m_fMinimum / m_fMaximum for charge weapons) up front, before
		// UC's `TgEffectDamage::ApplyEffect` reads them via GetProratedValue
		// or ChargeModifier. UC then computes:
		//     fProratedAmount = m_fBase                           ← scaled
		//     ChargeModifier → m_fMinimum + (m_fMaximum - m_fMinimum) * pct
		//     × m_fPercAbsorbedDamage, m_fPctDamageDecreaseDueToWeakSpot
		//     × InstigatorPawn.s_fDamageAdjustment
		//     CheckEffectBuffModifier(…)                          ← no-op
		//     CheckDamageTakenModifier(…)
		//     ProtectionModifier(…)
		// Because every step except CheckEffectBuffModifier is a percent
		// scalar, scaling m_fBase up front is mathematically identical to
		// scaling at CheckEffectBuffModifier. Scaling m_fMinimum / m_fMaximum
		// proportionally keeps charge-weapon ramps consistent.
		//
		// `m_fBuffedDamageInitial` cache (TgEffectDamage.uc:103) is fine:
		// for DOT damage (m_bUseOnInterval=true), the first tick computes
		// fProratedAmount from our scaled m_fBase and caches it; later ticks
		// reuse the cached *already-buffed* value. For instant damage
		// (!m_bUseOnInterval) the cache is bypassed every call.
		//
		// Per-clone scaling is the right granularity: the clone's
		// m_Instigator was just memcpy'd from the template (which UC's
		// `InitInstance` set to the firing pawn before `GetClonedEffectGroup`
		// ran), so each clone scales by the firer that produced it.
		const char* effClassName = effClone->Class ? effClone->Class->GetFullName() : nullptr;
		if (effClassName && strstr(effClassName, "TgEffectDamage") != nullptr) {
			AActor* inst = clone->m_Instigator;
			if (inst && effClone->m_fBase > 0.0f) {
				ATgPawn* instPawn = nullptr;
				ATgDeployable* depInstigator = nullptr;
				const char* instClass = inst->Class ? inst->Class->GetFullName() : nullptr;
				if (instClass && strstr(instClass, "TgPawn") != nullptr) {
					instPawn = (ATgPawn*)inst;
				} else if (instClass && strstr(instClass, "TgDeployable") != nullptr) {
					// Damage-dealing deployable (sentry mine, AOE bomb).
					// Walk back to the deploying pawn; we'll override the
					// query's devInst+skillId with the deploy device's
					// values from the registry.
					depInstigator = (ATgDeployable*)inst;
					APawn* depInst = ((AActor*)inst)->Instigator;
					if (depInst && depInst->Class) {
						const char* depCls = depInst->Class->GetFullName();
						if (depCls && strstr(depCls, "TgPawn") != nullptr) {
							instPawn = (ATgPawn*)depInst;
						}
					}
				}

				if (instPawn) {
					typedef void(__fastcall* GetBuffedPropertyFn)(
						ATgPawn*, void*, unsigned char,
						int, int, int, int, int, float, float*, void*);
					static const GetBuffedPropertyFn GetBuffedPropertyNative =
						(GetBuffedPropertyFn)0x109d7ff0;
					float origBase = effClone->m_fBase;
					float buffedBase = origBase;

					// Resolve the firing-device identity for the buff query.
					// Player-fired: clone->m_nSourceDeviceInstId is the
					// equipped device — Inventory tracks it.
					// Deployable-fired: it's the deployable's internal
					// device which the player never equipped, so we use
					// the deploy device's instId+skillId from
					// DeployableOriginRegistry. Without this override
					// damage-dealing deployables ignore all rolled mods
					// and multi-row skills.
					int queryDevInst  = clone->m_nSourceDeviceInstId;
					int queryCatSkill = DeviceCategorySkill::LookupByInstanceId(
						instPawn, queryDevInst);
					if (depInstigator) {
						DeployableOriginRegistry::Origin origin =
							DeployableOriginRegistry::Lookup(depInstigator);
						queryDevInst  = origin.spawnDeviceInstId;
						queryCatSkill = origin.spawnDeviceSkillId;
					} else if (queryDevInst == 0) {
						// Same fallback as [EFFECT-BUFF] — recover firing
						// fire-mode from the per-template owner map for
						// deployable-fired damage effects (sentry mines,
						// AOE bombs) where UC's InitInstance failed to
						// populate m_nSourceDeviceInstId.
						UTgDeviceFire* firingFM = DeployableOriginRegistry::GetTemplateOwner(eg);
						if (firingFM && firingFM->m_Owner && firingFM->m_Owner->Class) {
							const char* fmoCls = firingFM->m_Owner->Class->GetFullName();
							if (fmoCls && (strstr(fmoCls, "TgDeployable") != nullptr ||
							               strstr(fmoCls, "TgDeploy_")    != nullptr)) {
								ATgDeployable* dep = (ATgDeployable*)firingFM->m_Owner;
								DeployableOriginRegistry::Origin origin =
									DeployableOriginRegistry::Lookup(dep);
								queryDevInst  = origin.spawnDeviceInstId;
								queryCatSkill = origin.spawnDeviceSkillId;
							}
						}
					}

					// `nReqCategoryCode = clone->m_nCategoryCode`: pass
					// firing effect-group's category. A sentry mine's
					// damage effect-group has its own cat tag (e.g.
					// "Station Damage" 1326 for damage-dealing stations);
					// only Station Buff's +20% Station Damage entry
					// (stored cat=1326) matches.
					GetBuffedPropertyNative(
						instPawn, /*edx=*/nullptr,
						/*eRequestContext=*/4,         // BUFF_OTHER (identity for prop 65)
						/*nPropId=*/65,                // TGPID_DAMAGE_MODIFIER
						/*nReqCategoryCode=*/clone->m_nCategoryCode,
						/*nReqSkillId=*/queryCatSkill,
						/*nReqDeviceInstId=*/queryDevInst,
						/*bUsePotencyModifier=*/0,
						/*fBaseValue=*/origBase,
						/*fBuffedValue=*/&buffedBase,
						/*Effect=*/effClone);
					if (buffedBase > origBase) {
						const float mult = buffedBase / origBase;
						effClone->m_fBase    = buffedBase;
						effClone->m_fMinimum *= mult;
						effClone->m_fMaximum *= mult;
						Logger::Log("effects",
							"[DAMAGE-BUFF] cloned damage effect: m_fBase %.3f -> %.3f (×%.3f)  instigator=%p egId=%d propId=%d\n",
							origBase, buffedBase, mult, instPawn,
							clone->m_nEffectGroupId, effClone->m_nPropertyId);
					}
				}
			}
		}

		// [EFFECT-BUFF] — instigator-side effect-modifier scaling for non-damage
		// effects. UC's `TgEffect.ApplyEffect:114-115` was supposed to call
		// `CheckEffectBuffModifier(fProratedAmount)` between GetProratedValue
		// and ApplyToProperty to scale the effect's amount by the firing pawn's
		// buff registry. That UC native is stripped, and UC-to-UC dispatches
		// it via inline VM (bypassing our ProcessEvent intercept). Same
		// workaround shape as the [DAMAGE-BUFF] block above: pre-scale the
		// cloned effect's m_fBase here, before UC reads it.
		//
		// Source of truth for the prop-id mapping: the binary's
		// `ConvertPropToPropList` (FUN_109e5220), which we hand off to via
		// GetBuffedPropertyNative below — so this block automatically picks
		// up every (effect-prop → modifier-prop) expansion the function
		// defines, no per-prop hardcoding here.
		//
		// Concrete example chains:
		//
		// (1) Super Tank + AOE Shield (self-cast modifier on self-cast effect):
		//   - Super Tank skill effect: prop 66 calc 69 base 100 → routed
		//     through ApplyBuff(SKILL) → m_EffectBuffInfo entry on player
		//     with nPropId=66, fSkillPercentModifier=-100.
		//   - Player activates AOE Shield → cloned effect on prop 49 with
		//     instigator=player, target=player (self-cast).
		//   - Here: GetBuffedProperty(BUFF_PAWN, 49) on player →
		//     ConvertPropToPropList(1, 49) → 66 → finds the entry → formula
		//     collapses 0.1 → 0 → no speed penalty.
		//
		// (2) Heal-output skill + deployed medical station:
		//   - Skill puts prop 385 (Heal Output Modifier) in deployer's buff
		//     registry.
		//   - Medical station fires its heal effect → m_Instigator = the
		//     deployable. We walk the deployable's engine `Instigator` field
		//     back to the deploying pawn and look up THAT pawn's prop 385.
		//   - The healed teammate gets the buffed amount even though they
		//     don't have the skill themselves.
		//
		// Why INSTIGATOR-side: the props in the BUFF_PAWN expansion table
		// (heal output, damage output, effect-on-target modifiers) are
		// SOURCE-side modifiers — properties of the actor producing the
		// effect, not the receiver. For self-cast (instigator=target) it's
		// indistinguishable. For cross-target effects, the firer's modifier
		// is what should scale outgoing effect amounts.
		//
		// Deployable instigator walkback: medical stations / turrets / bombs
		// fire effects with m_Instigator set to the deployable itself
		// (TgDeployable.uc:528 passes `self` to ApplyHit). The deployable's
		// engine `Instigator` field points to the deploying pawn (set by
		// SpawnDeployableActor). Walking through this chain means a player's
		// heal-output / damage-output skills propagate to the things they
		// deploy without per-call site code.
		//
		// TgEffectDamage skipped: damage gets prop-65 instigator scaling
		// above + target-side mitigation via CalcCategoryProtection /
		// CalcDamageTypeProtection / CalcAttackTypeProtection at apply time
		// + SubmitMitigationDamage hook. Adding generic scaling on top would
		// double-count.
		//
		// Scaling DOWN allowed (no `if (buffedBase > origBase)` guard): the
		// registry can legitimately reduce effect strength (e.g. Super Tank's
		// -100% speed-effect modifier collapses shield speed-penalty to 0).
		if (effClassName && strstr(effClassName, "TgEffectDamage") == nullptr
		    && effClone->m_fBase != 0.0f) {
			ATgPawn* srcPawn = nullptr;
			ATgDeployable* depInstigator = nullptr;
			AActor* inst = clone->m_Instigator;
			if (inst && inst->Class) {
				const char* instCls = inst->Class->GetFullName();
				if (instCls && strstr(instCls, "TgPawn") != nullptr) {
					srcPawn = (ATgPawn*)inst;
				} else if (instCls && strstr(instCls, "TgDeployable") != nullptr) {
					// Walk back to the deploying pawn via the deployable's
					// engine Instigator field. Cast through APawn — both
					// TgDeployable and TgPawn inherit Instigator from AActor.
					depInstigator = (ATgDeployable*)inst;
					APawn* depInst = ((AActor*)inst)->Instigator;
					if (depInst && depInst->Class) {
						const char* depCls = depInst->Class->GetFullName();
						if (depCls && strstr(depCls, "TgPawn") != nullptr) {
							srcPawn = (ATgPawn*)depInst;
						}
					}
				}
			}

			if (srcPawn) {
				typedef void(__fastcall* GetBuffedPropertyFn)(
					ATgPawn*, void*, unsigned char,
					int, int, int, int, int, float, float*, void*);
				static const GetBuffedPropertyFn GetBuffedPropertyNative =
					(GetBuffedPropertyFn)0x109d7ff0;
				float origBase = effClone->m_fBase;
				float buffedBase = origBase;

				// Resolve the firing-device identity for the buff query.
				// For a player-fired effect: clone->m_nSourceDeviceInstId is
				// the player's own equipped device — Inventory tracks it,
				// so DeviceCategorySkill::LookupByInstanceId resolves the
				// skillId from item_id.
				//
				// For a deployable-fired effect (medical station heal,
				// sensor scan, etc.): clone->m_nSourceDeviceInstId is the
				// DEPLOYABLE's internal fire-mode device — a separate
				// device the player has never equipped, so it's not in
				// Inventory, the skillId lookup misses, AND the rolled
				// mods (registered on the player's deploy device) don't
				// match either. Override with the deploy device's instId
				// + skillId from DeployableOriginRegistry, which we
				// recorded at SpawnDeployableActor time.
				int queryDevInst   = clone->m_nSourceDeviceInstId;
				int queryCatSkill  = DeviceCategorySkill::LookupByInstanceId(
					srcPawn, queryDevInst);
				const char* pathTag = "player";
				if (depInstigator) {
					DeployableOriginRegistry::Origin origin =
						DeployableOriginRegistry::Lookup(depInstigator);
					queryDevInst  = origin.spawnDeviceInstId;
					queryCatSkill = origin.spawnDeviceSkillId;
					pathTag = "deployable";
				} else if (queryDevInst == 0) {
					// Player-instigator path (e.g. medical-station heals where
					// UC sets m_Instigator = the deploying player) AND UC's
					// InitInstance failed to populate m_nSourceDeviceInstId
					// because Impact.DeviceModeReference was null.
					//
					// Recover the firing fire-mode from the per-template
					// owner map (set when GetEffectGroup lazy-populates
					// s_EffectGroupList). The map is keyed on the TEMPLATE
					// effect group pointer, which the caller passed to us
					// as `eg` (closed over by the surrounding function).
					// Templates are 1:1 with their owning fire-mode and
					// set-once at init, so this is race-free even if
					// multiple players deploy simultaneously.
					UTgDeviceFire* firingFM = DeployableOriginRegistry::GetTemplateOwner(eg);
					if (firingFM && firingFM->m_Owner && firingFM->m_Owner->Class) {
						const char* fmoCls = firingFM->m_Owner->Class->GetFullName();
						if (fmoCls && (strstr(fmoCls, "TgDeployable") != nullptr ||
						               strstr(fmoCls, "TgDeploy_")    != nullptr)) {
							ATgDeployable* dep = (ATgDeployable*)firingFM->m_Owner;
							DeployableOriginRegistry::Origin origin =
								DeployableOriginRegistry::Lookup(dep);
							queryDevInst  = origin.spawnDeviceInstId;
							queryCatSkill = origin.spawnDeviceSkillId;
							pathTag = "deployable-via-template";
						}
					}
				}

				// Diagnostic: see what cat/skill/devInst we end up querying with.
				Logger::Log("effects",
					"[EFFECT-BUFF/probe] propId=%d cloneCat=%d cloneSrcDevInst=%d  path=%s  "
					"queryCat=%d querySkill=%d queryDevInst=%d  inst=%p depInst=%p src=%p\n",
					effClone->m_nPropertyId, clone->m_nCategoryCode,
					clone->m_nSourceDeviceInstId, pathTag,
					clone->m_nCategoryCode, queryCatSkill, queryDevInst,
					inst, depInstigator, srcPawn);

				// Dump matching prop-330/385/376 buff entries on srcPawn so we
				// can see whether Station Buff/Cyber Specialist/rolled mods
				// are even in m_EffectBuffInfo, with what stored gating values.
				if (srcPawn && srcPawn->m_EffectBuffInfo.Data) {
					int n = srcPawn->m_EffectBuffInfo.Num();
					for (int bi = 0; bi < n; ++bi) {
						FBuffInfo& e = srcPawn->m_EffectBuffInfo.Data[bi];
						int p = e.BuffHeader.nPropId;
						if (p == 330 || p == 385 || p == 376 || p == 51) {
							Logger::Log("effects",
								"   [BUFFINFO/probe] idx=%d prop=%d cat=%d skill=%d devInst=%d  "
								"IP=%.2f IM=%.2f SP=%.2f SM=%.2f selfP=%.2f selfM=%.2f genP=%.2f genM=%.2f\n",
								bi, p, e.BuffHeader.nReqCategoryCode,
								e.BuffHeader.nReqSkillId, e.BuffHeader.nReqDeviceInstId,
								e.fItemPercentModifier, e.fItemModifier,
								e.fSkillPercentModifier, e.fSkillModifier,
								e.fSelfPercentModifier, e.fSelfModifier,
								e.fPercentModifier,     e.fModifier);
						}
					}
				}

				// `bUsePotencyModifier=1`: ConvertPropToPropList(BUFF_PAWN, …)
				// adds prop 376 (Effect Potency Modifier) to the search list
				// only when the potency flag is set. Skills like Station Buff
				// (+20/50/20% prop 376 gated to "Stations") and Cyber
				// Specialist's stations row (+15/15/50% prop 376) are
				// otherwise unreachable. Safe in BUFF_PAWN context — the
				// binary's expansion only adds 376 in this branch.
				//
				// `nReqCategoryCode = clone->m_nCategoryCode`: pass the
				// firing effect-group's category. For a Medical Station's
				// heal effect-group 6026 this is 1324 (Station Healing) —
				// only Station Buff's +20% Station Healing entry matches,
				// while +50% Station Protection / +20% Station Damage
				// entries (stored cat 1327 / 1326) miss per the
				// stored>0-must-equal-query rule. Without this filter all
				// three prop-376 entries collapse into one slot and stack
				// to +90% (the "absurd over-buffing" path).
				GetBuffedPropertyNative(
					srcPawn, /*edx=*/nullptr,
					/*eRequestContext=*/1,         // BUFF_PAWN — effect-on-pawn
					/*nPropId=*/effClone->m_nPropertyId,
					/*nReqCategoryCode=*/clone->m_nCategoryCode,
					/*nReqSkillId=*/queryCatSkill,
					/*nReqDeviceInstId=*/queryDevInst,
					/*bUsePotencyModifier=*/1,
					/*fBaseValue=*/origBase,
					/*fBuffedValue=*/&buffedBase,
					/*Effect=*/effClone);
				if (buffedBase != origBase) {
					const float mult = (origBase != 0.0f) ? (buffedBase / origBase) : 1.0f;
					effClone->m_fBase    = buffedBase;
					effClone->m_fMinimum *= mult;
					effClone->m_fMaximum *= mult;
					Logger::Log("effects",
						"[EFFECT-BUFF] cloned effect: propId=%d  m_fBase %.3f -> %.3f (×%.3f)  src=%p egId=%d\n",
						effClone->m_nPropertyId, origBase, buffedBase, mult,
						srcPawn, clone->m_nEffectGroupId);
				}
			}
		}

		clone->m_Effects.Add(effClone);
	}

	// Templates have m_bHasVisual=0 because the loader native that would set it
	// is stripped; force it on so TgEffectManager::ProcessEffect's HUD gate
	// passes and r_ManagedEffectList is populated.
	clone->m_bHasVisual = 1;

	// Per-instance state — caller (GetClonedEffectGroup) wires s_OwnerEffectManager;
	// InitInstance later sets m_Target / m_Instigator / m_SavedImpact.
	clone->s_OwnerEffectManager = nullptr;
	// -1 means "no managed-list slot yet". UC `ProcessEffect` overwrites this
	// with `SetEffectRep`'s return when the visual gate passes. If the gate is
	// skipped (e.g. ApplyEventBasedEffects bailed on protection-zeroed lifetime,
	// so no SetEffectRep call), the index stays -1 — and our `RemoveEffectGroup`
	// / `RemoveAllEffectGroups` cleanup `if (slot >= 0 && slot < 0x10)` correctly
	// skips slot zeroing instead of clobbering slot[0] (which belongs to a
	// different group). Setting it to 0 here was the cause of an EMP-second-stun
	// no-icon bug: phantom 6995 (Apply blocked by cat-653 prot) inherited slot 0
	// by default, then on its reversal wiped the just-placed 6994 stun icon.
	clone->s_ManagedEffectListIndex = -1;
	clone->s_NonContagiousEffectGroup = nullptr;

	return clone;
}
