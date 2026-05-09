#pragma once

#include <vector>

#include "src/ControlServer/Loadouts/ModConstants.hpp"

// Resolves a `Mods::Result` (letter strings) into a concrete egid list,
// using each device's own asm_data_set_blueprint_item_mods pool to map
// letters → props → egids. Same letter on different devices can pick
// different props (e.g. 'd' on a rocket turret = Pet Damage prop 350,
// 'd' on a melee weapon = Melee Damage prop 212, 'd' on a ranged weapon =
// Effect Damage prop 65).
//
// Called from PlayerSessionStore::ResyncCharacterDevicesFromLoadout when
// persisting the loadout to ga_character_devices.mod_effect_group_ids.
namespace ModResolver {

    // Resolve the letter strings in `mods` against this device + quality.
    //
    // Order of returned egids: innate letters first (in the order written),
    // then kit letters / kit pairs.
    //
    // Resolution rules:
    //   - mods.raw_egids non-empty → returned as-is (back-compat for rows
    //     that pass concrete egids via { id, id, id }).
    //   - innate letter X → device-specific innate egid for (X, quality).
    //     Pulled from this device's blueprint pool. Letters with no entry
    //     are silently dropped.
    //   - kit "vn" or "nv" pair → one Survivor kit egid (renders both
    //     letters in the suffix).
    //   - kit single letter X → kit egid keyed by the prop derived from
    //     this device's pool for letter X. If the device's pool has no
    //     entry for X, or if no kit exists for the derived prop, dropped.
    //   - lone 'v' (no adjacent 'n') is dropped — no single-letter 'v'
    //     kit egid exists in the shipped DB.
    std::vector<int> Resolve(int device_id, int quality_value_id, const Mods::Result& mods);

}  // namespace ModResolver
