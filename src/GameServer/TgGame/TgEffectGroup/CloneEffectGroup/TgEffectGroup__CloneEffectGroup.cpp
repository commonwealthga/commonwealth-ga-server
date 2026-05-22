#include "src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/ConstructEffectGroup/TgEffectGroup__ConstructEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffect/CloneEffect/TgEffect__CloneEffect.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// UTgEffectGroup::CloneEffectGroup — original native @ 0x10a6f3c0 is an empty
// stub. UC's TgEffectManager::GetClonedEffectGroup → eg.CloneEffectGroup()
// expects a fresh independent instance: the same template feeds multiple
// targets (DoT/HoT/buffs across players), so returning self aliased the
// template and stomped m_Target / s_OwnerEffectManager / timer state.
//
// Effects are cloned independently too (via the reimplemented TgEffect::
// CloneEffect at 0x10a6f2a0). Sharing template effect pointers across clones
// makes two simultaneous applications BOTH write the same m_fCurrent; the
// first reversal zeros it, the second then sees m_fCurrent==0, hits the
// phantom-clone skip in RemoveEffects, and the property/buff stays stuck at
// +amount permanently.
//
// Clean-room rebuild notes:
//   * No buff-ness tagging — class_res_id-157 effects are real TgEffectBuff
//     instances (preserved by CloneEffect via source->Class), so their
//     ApplyEffect/Remove route to ATgPawn::ApplyBuff canonically via UC virtual
//     dispatch. Nothing to tag here.
//   * No DeployableOriginRegistry registration — OriginResolver recovers the
//     spawning device at query time by walking the effect group's
//     m_Instigator (deployable) → s_SpawnerDeviceMode → device. The side-map
//     that survived UC InitInstance's m_nSourceDeviceInstId clear is no longer
//     needed; the walk doesn't depend on that field at all.
//   * Apply-time scaling stays in the reimplemented CheckEffectBuffModifier /
//     CheckOwnerPetBuff natives — not pre-baked into m_fBase here.
//
// Group-level field copy: split memcpy around the only TArray (m_Effects @
// 0x60). The ctor-linked fresh instance has m_Effects = {nullptr,0,0}; we
// re-Add each cloned UTgEffect* so the clone owns an independent container.
// FName / FImpactInfo / FPointer fields in the second range are inline storage
// — shallow memcpy is correct.
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
		// mutate the template's m_bHasVisual flag here (BuildEffectGroup set it
		// from DB icon_id).
		return eg;
	}

	clone->ObjectFlags.A |= 0x4000;  // RF_RootSet — keep alive past UC GC sweeps

	uint8_t* dst = reinterpret_cast<uint8_t*>(clone);
	uint8_t* src = reinterpret_cast<uint8_t*>(eg);

	// Range 1: 0x3C..0x60 (everything before m_Effects)
	std::memcpy(dst + 0x3C, src + 0x3C, 0x60 - 0x3C);
	// Range 2: 0x6C..0x140 (everything after m_Effects — FImpactInfo / FName /
	// FPointer, all inline storage, safe to shallow-copy)
	std::memcpy(dst + 0x6C, src + 0x6C, 0x140 - 0x6C);

	// Independent TArray container with per-instance effect clones.
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
			Logger::Log("effects",
				"[CLONE] effect[%d] CloneEffect returned null — sharing template ptr (legacy)\n", i);
			clone->m_Effects.Add(tmplEffect);
			continue;
		}

		// Wire the cloned effect to the new clone group, not the template, so
		// m_EffectGroup.GetProperty / CalcCategoryProtection / etc. read the
		// per-instance m_Target / m_Instigator instead of stale template state.
		effClone->m_EffectGroup = clone;

		// Do not touch m_bRemovable: CDO default true (TgEffect.uc:662) is
		// preserved by CloneEffect's memcpy; force-setting would override any
		// legitimate instant-only `false`.

		// Don't RootSet the cloned effect: the RootSet'd clone group keeps it
		// alive via m_Effects; RootSet'ing here would leak after group removal.
		clone->m_Effects.Add(effClone);
	}

	// Per-instance state — caller (GetClonedEffectGroup) wires
	// s_OwnerEffectManager; InitInstance later sets m_Target / m_Instigator /
	// m_SavedImpact.
	clone->s_OwnerEffectManager = nullptr;

	// -1 = "no managed-list slot yet". UC ProcessEffect overwrites with
	// SetEffectRep's return when the visual gate passes; if skipped, staying -1
	// makes RemoveEffectGroup/RemoveAllEffectGroups' `if (slot >= 0 && slot <
	// 0x10)` correctly skip slot zeroing instead of clobbering slot[0] (a
	// different group). Setting 0 here caused the EMP-second-stun no-icon bug.
	clone->s_ManagedEffectListIndex = -1;
	clone->s_NonContagiousEffectGroup = nullptr;

	return clone;
}
