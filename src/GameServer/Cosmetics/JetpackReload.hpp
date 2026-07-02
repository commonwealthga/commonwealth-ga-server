#pragma once

#include "src/pch.hpp"

// ── Owner-side jetpack/backpack re-dye on profile switch ─────────────────────
//
// The persistent backpack mesh (TgPawn_Character.c_BackpackMesh) is built from
// the slot-5 jetpack device's form. On a profile switch the owner rebuilds via
// the r_CustomCharacterAssembly repnotify → ApplyPawnSetup → … →
// OnWaitingForPawnDone → DeviceFormChanged(false) (TgPawn.uc:6711). The
// non-forced reload only rebuilds forms whose cached
// c_EquipDeviceInfo.nDeviceInstanceId differs from the pawn's replicated
// r_EquipDeviceInfo[slot] (compare per DoubleCheckSimulatedProxy,
// TgPawn.uc:8953) — and the jetpack keeps the same inventory id across
// profiles, so its form (and the backpack MIC's baked dye) survives. Observers
// are unaffected: sim proxies get DeviceFormChanged(true) in PostPawnSetup.
//
// Note: TgPawn.ReplicatedEvent has NO case for r_EquipDeviceInfo (despite the
// repnotify declaration) — deltas apply silently. A transient blip therefore
// triggers nothing on the owner; the mismatch must still be present when the
// owner's own post-rebuild DeviceFormChanged(false) runs. So: write a fresh
// epoch into the high bits of the replicated slot-5 instance id and leave it
// there. The device ACTOR keeps the real invId (inventory recovery matches
// actor ids, not this array), and the PRI's separate r_EquipDeviceInfo (UI)
// is untouched. Native UpdateClientDevices may later rewrite the slot from
// the device — silent on the client, benign.
namespace JetpackReload {

// Give the pawn's replicated slot-5 descriptor a fresh instance id so the
// owner's next DeviceFormChanged(false) rebuilds (and re-dyes) the jetpack
// form. Call after all native equip paths have run for the switch.
void MarkJetpackFormDirty(ATgPawn* Pawn);

}  // namespace JetpackReload
