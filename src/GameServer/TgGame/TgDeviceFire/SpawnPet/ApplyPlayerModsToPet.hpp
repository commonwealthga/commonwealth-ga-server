#pragma once

#include "src/pch.hpp"

// Apply the deploying pawn's pet-related buff-registry modifiers to a freshly
// spawned pet pawn (turret / drone). Mirrors `ApplyPlayerModsToDeployable` —
// player skills like Drone Damage / Drone Range / Pet Max Health register
// buffs on the player's m_EffectBuffInfo with prop ids 350/366/381/382/383
// (etc.), but the pet's fire-mode reads consult the PET's m_EffectBuffInfo,
// not the player's. Without this bridge those skills silently no-op.
//
// We do the same physical-radius style fix used for deployables: walk the
// player's registry, find pet-relevant entries scoped to this device's
// instance id (or wildcards = skills), apply the layered formula
// (v3 = base*(1+IP/100)*(1+SP/100)*(1+(LP+GP)/100) + IM+SM+LM+GM) to the
// matching pet base property, and write the scaled value into the pet's
// s_Properties directly.
//
// `sourceDeviceInstId` filters buff entries to:
//   - this firing device's rolled mods (stored devInst == sourceDeviceInstId)
//   - skills (stored devInst == 0, wildcard)
// Matches the FUN_109cd890 search-mode rules (entry stored 0 = wildcard).
//
// Two bridge modes per mapping:
//   BUFF   — register the modifier on the pet's m_EffectBuffInfo so its
//            fire-time GetBuffedProperty (BUFF_OTHER for damage, BUFF_DEVICE
//            for fire-mode props) consults it. Used for fire-output
//            modifiers (damage, range, radius, accuracy).
//   DIRECT — apply the layered formula directly to pet's s_Properties[]
//            once at spawn. Used for one-shot stats that are read at spawn
//            and don't re-query the buff registry (HP, lifespan).
//
// Mappings (see kPetMap in the .cpp):
//   350 PetDamage      → pet's prop 65  (BUFF, scales pet damage effect)
//   381 PetRange       → pet's prop 114 (BUFF, scales pet's RangeModifier)
//   382 PetDmgRadius   → pet's prop 352 (BUFF, scales AOE radius)
//   383 PetAccuracy    → pet's prop 113 (BUFF, scales accuracy)
//   366 PetMaxHealth   → pet's prop 304 + 51 (DIRECT)
//   355 PetLifespan    → pet's prop 354 (DIRECT)
namespace ApplyPlayerModsToPet {
	void Apply(ATgPawn* deployingPawn, ATgPawn* petPawn, int sourceDeviceInstId);
}
