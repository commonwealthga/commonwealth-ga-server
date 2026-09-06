#pragma once

#include "src/pch.hpp"

// Deferred verification that a deployed beacon's carry device actually left
// equip slot 11.
//
// Why this exists
// ---------------
// The beacon pickup device (id 1918) has exactly ONE removal path in our
// server: the `TgDevice.DeviceFiring.EndState` case in
// `UObject__ProcessEvent.cpp`. The binary's own consume path is structurally
// dead for this device — `TgDevice.ConsumeDevice` →
// `RemoveConsumableFromOwnerInventory` (0x10a1dd00) early-outs unless
// `owner->Weapon == this`, and `asm_data_set_devices` gives device 1918
// `in_hand_device_flag = 0`, so `Pawn->Weapon` is never the beacon.
//
// If that single EndState cleanup is missed, the device stays in slot 11 for
// the rest of the match, and every downstream beacon behaviour breaks at once,
// because `TgPawn::IsCarryingBeacon` (0x109be2d0) is literally
// "slot 11 holds a device with m_bIsBeaconPlacing":
//
//   * the carry FX stays applied and stealth stays blocked;
//   * the player can fire the leftover device again — and `RegisterBeacon`
//     (0x109f1ed0) overwrites `r_Beacon` without destroying the incumbent, so
//     the team ends up with two live exit beacons;
//   * `CheckBeacon`'s carrier scan keeps reporting him as carrying, which
//     SUPPRESSES the team's beacon respawn entirely when the real beacon dies.
//
// So: when a beacon actually enters the world from a player's carry device, we
// record (pawnId, invId) and re-check a couple of seconds later. Idempotent by
// construction — the sweep fires only if slot 11 still holds that exact
// inventory id, so the normal EndState path (which runs first, on the device's
// one-shot RefireCheckTimer) simply makes this a no-op.
namespace BeaconCarryReaper {

// Record that `Pawn` just put a beacon in the world using the slot-11 device
// whose inventory id is `invId`. `delaySecs` should sit comfortably past the
// firing device's refire time so the normal cleanup wins the race.
void ArmConsume(class ATgPawn* Pawn, int invId, float delaySecs);

// Drop a pending record — called by the EndState cleanup once it has actually
// removed the device, so the sweep never runs for a device that is already
// gone (and can never touch a *different* device that later reuses slot 11).
void Disarm(int pawnId, int invId);

// Per-frame drain. Cheap: returns immediately when nothing is pending.
void Tick(float DeltaSeconds);

// Deploy-refusal handshake (hazard-volume placement gate).
//
// `TgDeviceFire::Deploy` can refuse a placement outright — nothing spawns and
// the player must keep his beacon. But UC still runs the device's
// `DeviceFiring.EndState`, whose cleanup keys purely off `m_bIsBeaconPlacing`
// and would consume the carry device anyway, destroying a beacon that was
// never placed. Deploy marks the refused fire cycle; the EndState case
// consumes the mark and skips its cleanup exactly once.
//
// A mark that is never consumed (EndState somehow not reached) is self-
// healing: it only ever suppresses ONE later cleanup, and ArmConsume's sweep
// still clears slot 11 a couple of seconds after any successful deploy.
void MarkDeployRefused(int pawnId, int invId);
bool ConsumeDeployRefused(int pawnId, int invId);

}  // namespace BeaconCarryReaper
