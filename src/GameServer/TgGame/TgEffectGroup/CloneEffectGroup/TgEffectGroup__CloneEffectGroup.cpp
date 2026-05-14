#include "src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/ConstructEffectGroup/TgEffectGroup__ConstructEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffect/CloneEffect/TgEffect__CloneEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeployableOriginRegistry.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// UTgEffectGroup::CloneEffectGroup — original native @ 0x10a6f3c0 is an empty
// stub. UC's TgEffectManager::GetClonedEffectGroup → eg.CloneEffectGroup()
// expects a fresh independent instance: the same template feeds multiple
// targets (DoT/HoT/buffs across players), so returning self aliased the
// template and stomped m_Target / s_OwnerEffectManager / timer state across
// callers.
//
// Effects are cloned independently too. UC declares `native function TgEffect
// CloneEffect();` (also stripped, also reimplemented in this codebase at
// `TgEffect__CloneEffect`). If we shared template effect pointers across
// clones, two simultaneous applications (two buff stations / overlapping
// pulses) would BOTH write to the same `m_fCurrent`, and the first reversal
// would zero it — the second reversal then sees `m_fCurrent==0`, hits the
// phantom-clone skip in `TgEffectGroup__RemoveEffects`, and the buff registry
// / s_Properties stays stuck at `+amount` permanently. Verified against
// effects.txt: BUFF-ROUTE APPLY count exceeds REMOVE count by ≥1 per player
// per buff-station session, and the player ends with `genM=10.000` residual
// on prop 113 long after leaving the radius.
//
// Group-level field copy strategy: split memcpy around the only TArray
// (m_Effects @ 0x60). The ctor-linked fresh instance has m_Effects = {nullptr,
// 0,0}; we re-Add each cloned UTgEffect* so the clone owns an independent
// TArray container with its own per-clone effect copies. FName / FImpactInfo /
// FPointer fields in the second range are inline storage — shallow memcpy is
// correct.
//
// Per-effect cloning is delegated to `TgEffect__CloneEffect::Call` (the
// reimplemented `TgEffect::CloneEffect` native at 0x10a6f2a0). Apply-time
// scaling of damage / non-damage effects by the instigator's buff registry
// is handled by the reimplemented `TgEffect::CheckEffectBuffModifier` native
// at 0x10a6f270 — see `TgEffect__CheckEffectBuffModifier.cpp` for the
// rationale. Before that native was implemented, this function carried two
// "pre-scale m_fBase / m_fMinimum / m_fMaximum at clone time" blocks that
// approximated the same scaling; they're gone now that the canonical apply-
// time path works.
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
		// non-null group rather than crashing in GetClonedEffectGroup. Do not
		// mutate the template's m_bHasVisual flag here: BuildEffectGroup sets
		// it from DB icon_id and clobbering it would either spawn spurious
		// HUD icons (if forced on) or hide legitimate ones (if forced off).
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

		UTgEffect* effClone = TgEffect__CloneEffect::Call(tmplEffect, /*edx=*/nullptr);
		if (!effClone) {
			// CloneEffect couldn't determine class size or ConstructObject
			// failed — fall back to sharing the template pointer. Functionally
			// the same as the pre-CloneEffect implementation's escape hatch;
			// per-effect state will alias the template until GC.
			Logger::Log("effects",
				"[CLONE] effect[%d] CloneEffect returned null — sharing template ptr (legacy)\n", i);
			clone->m_Effects.Add(tmplEffect);
			continue;
		}

		// Wire the cloned effect to the new clone group, not the template.
		// CloneEffect's memcpy copied m_EffectGroup from the template (= the
		// template group), so CalcCategoryProtection / m_EffectGroup.GetProperty
		// / etc. would dereference the template's m_Target / m_Instigator
		// (which aren't set per-instance) and read stale state without this.
		effClone->m_EffectGroup = clone;

		// Do not touch m_bRemovable here. CDO default is true (TgEffect.uc:662);
		// ConstructEffect's UObject template-copy gives every fresh instance
		// m_bRemovable=true, BuildEffectGroup never clears bit 0x02, the
		// engine asm-data loader (FUN_10a6f300) doesn't touch +0x48, and UC
		// has no write site. So the memcpy in CloneEffect already produces
		// m_bRemovable=true on the clone. Force-setting it would silently
		// override any legitimate future `false` (instant-only effects per
		// TgEffectGroup.uc:364) instead of honoring it.

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

		clone->m_Effects.Add(effClone);
	}

	// m_bHasVisual is now seeded by `BuildEffectGroup` from
	// `asm_data_set_effect_groups.icon_id > 0` and propagates through the
	// 0x6C..0x140 memcpy above. Don't touch it here — force-on caused HUD
	// icons to appear for effect groups that should never have shown one
	// (rolled mods, knockback impulses, hold-to-sustain modifier groups,
	// etc., all of which have icon_id=0 in the DB).

	// **Deployable source-device side-map.** For deployable-fired effects,
	// resolve the deploying player's spawn-device origin via the
	// template→fire-mode → deployable chain and stash it on the clone
	// pointer in DeployableOriginRegistry::g_cloneOrigins.
	//
	// Why a side-map instead of writing m_nSourceDeviceInstId on the clone
	// directly: UC's `TgEffectGroup::InitInstance` runs AFTER us and
	// unconditionally CLEARS m_nSourceDeviceInstId / m_nSourceDeviceSkillId
	// before only re-setting them when `Impact.DeviceModeReference != none`.
	// For Medical Station's heal chain the Impact arrives with no
	// DeviceModeReference, so any field write here would be wiped before
	// CheckEffectBuffModifier runs. The side-map survives the clear.
	//
	// Concrete failure (2026-05-14): Medical Station heal egId 6026 cat 1324
	// queried CheckEffectBuffModifier with devInst=0 skill=0; `GetBuffIndex`
	// rejected every entry stored with devInst>0 (Output Mod +70%,
	// per-skill rows). Heal scaled only by the one universal entry
	// (Super Engineer +20%) → 184/tick instead of expected ~470.
	UTgDeviceFire* templateOwner = DeployableOriginRegistry::GetTemplateOwner(eg);
	if (templateOwner && templateOwner->m_Owner && templateOwner->m_Owner->Class) {
		const char* ownerCls = templateOwner->m_Owner->Class->GetFullName();
		if (ownerCls && (std::strstr(ownerCls, "TgDeployable") != nullptr ||
		                 std::strstr(ownerCls, "TgDeploy_")    != nullptr)) {
			ATgDeployable* dep = (ATgDeployable*)templateOwner->m_Owner;
			DeployableOriginRegistry::Origin origin = DeployableOriginRegistry::Lookup(dep);
			if (origin.spawnDeviceInstId != 0) {
				DeployableOriginRegistry::RegisterClone(clone,
					origin.spawnDeviceInstId, origin.spawnDeviceSkillId);
				if (Logger::IsChannelEnabled("effects")) {
					Logger::Log("effects",
						"[CLONE-ORIGIN] clone=%p (template=%p) cat=%d  source from deployable=%p  devInst=%d skill=%d\n",
						clone, eg, clone->m_nCategoryCode, dep,
						origin.spawnDeviceInstId, origin.spawnDeviceSkillId);
				}
			}
		}
	}

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
