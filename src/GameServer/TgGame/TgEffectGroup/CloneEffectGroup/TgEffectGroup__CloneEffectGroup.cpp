#include "src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/ConstructEffectGroup/TgEffectGroup__ConstructEffectGroup.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

#include <cstring>

// UTgEffectGroup::CloneEffectGroup — original native @ 0x10a6f3c0 is an empty
// stub. UC's TgEffectManager::GetClonedEffectGroup → eg.CloneEffectGroup()
// expects a fresh independent instance: the same template feeds multiple
// targets (DoT/HoT/buffs across players), so returning self aliased the
// template and stomped m_Target / s_OwnerEffectManager / timer state across
// callers.
//
// Shape mirrors Inventory.cpp's TgInventoryObject_Device construction:
// StaticConstructObject wrapper → RF_RootSet → field copy. Class pointer comes
// from ClassPreloader::GetClass (per the deprecated-Get*Class-helpers rule).
//
// Field copy strategy: split memcpy around the only TArray (m_Effects @ 0x60).
// The ctor-linked fresh instance has m_Effects = {nullptr,0,0}; we re-Add each
// UTgEffect* so the clone owns an independent TArray container while sharing
// the (template-owned) effect pointers. FName / FImpactInfo / FPointer fields
// in the second range are inline storage — shallow memcpy is correct.
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

	// Independent TArray container, shared template UTgEffect* pointers
	clone->m_Effects.Data = nullptr;
	clone->m_Effects.Count = 0;
	clone->m_Effects.Max = 0;
	for (int i = 0; i < eg->m_Effects.Count; ++i) {
		clone->m_Effects.Add(eg->m_Effects.Data[i]);
	}

	// Templates have m_bHasVisual=0 because the loader native that would set it
	// is stripped; force it on so TgEffectManager::ProcessEffect's HUD gate
	// passes and r_ManagedEffectList is populated.
	clone->m_bHasVisual = 1;

	// Per-instance state — caller (GetClonedEffectGroup) wires s_OwnerEffectManager;
	// InitInstance later sets m_Target / m_Instigator / m_SavedImpact.
	clone->s_OwnerEffectManager = nullptr;
	clone->s_ManagedEffectListIndex = 0;
	clone->s_NonContagiousEffectGroup = nullptr;

	return clone;
}
