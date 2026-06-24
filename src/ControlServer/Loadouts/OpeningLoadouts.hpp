#pragma once

#include <cstdint>
#include <vector>

#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"  // GearSlot, PROFILE_*, SVID_*, Q_*

// =============================================================================
// Opening loadouts — what a brand-new character gets EQUIPPED at creation,
// across all 5 loadout profiles (item_profile_id 1..5).
//
// Distinct from ClassLoadouts.cpp: that file is the account-wide BAG POOL
// (every variant the account owns). This file is a curated SUBSET — one device
// per equip point — that we actually equip into ga_character_devices so a
// fresh character spawns combat-ready and can switch between 5 builds, then
// re-customize any of them later.
//
// Authoring: copy the exact GearSlot row you want straight out of
// ClassLoadouts.cpp. Each entry MUST correspond to a real pool row (same
// device_id + quality + mods + oc) or it won't resolve to an inventory_id and
// the slot is left empty. Omit equip_slot 14 (class device) —
// PlayerSessionStore::PinClassDeviceSlot14 owns it.
//
// Seeded once at character creation by PlayerSessionStore::SeedOpeningLoadout.
// Never re-run, so player edits are never clobbered.
// =============================================================================

namespace Loadouts {

// Returns the equipped loadout for (class profile_id, loadout_slot 1..5).
// Empty vector when the class/slot has no curated build (slot left blank, no
// crash).
const std::vector<GearSlot>& GetOpeningLoadout(uint32_t profile_id, int loadout_slot);

}  // namespace Loadouts
