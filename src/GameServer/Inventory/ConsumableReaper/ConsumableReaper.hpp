#pragma once

#include "src/pch.hpp"

// Deferred removal of a used consumable (ES13, r_bConsumedOnUse) from its slot.
//
// Consumables are unlimited (ClassLoadouts), so "consume" unequips instead of
// deleting: the live device leaves its slot, the control server drops the slot
// from the saved loadout and resyncs the client. The inventory row stays, so
// the client's record count keeps matching r_ItemCount (which every loot drop
// restates from the DB). The player re-equips it in the equip screen.
//
// Why deferred, not at DeviceFiring.EndState: devices with a long refire (e.g.
// Lasting Regeneration) stay in DeviceFiring until death or re-equip, so an
// EndState removal landed far too late. Removing inside BeginState would run
// while the device's own fire code is still on the stack.
namespace ConsumableReaper {

// Called from DeviceFiring.BeginState for a consumable that just fired.
void Arm(class ATgPawn* Pawn, class ATgDevice* Device);

// Per-frame drain. Returns immediately when nothing is pending.
void Tick(float DeltaSeconds);

}  // namespace ConsumableReaper
