#pragma once

#include "src/pch.hpp"

// MoraleCredit — owns the morale-system gameplay logic that the stripped
// TgEffect::TrackStats + TgPawn::AddMoralePoints natives used to handle.
//
// Architectural boundary: TrackStats is for *stats* and only dispatches
// here as the morale-credit driver. All morale-specific filtering, rate
// math, Output Mod scaling, accumulator math, gates, and replicated-mirror
// writes live below.
//
// Anti-feedback filter: per the binary's intact TgPawn::AddMoralePoints @
// 0x109d2f00 decompile, the engine blocks morale credit when the damage
// source's parent device has `slot_used_value_id == 476` ("Morale Device"
// in valid_value_group 67). The DB has 12 such devices — they are the
// player's morale devices AND the deployable bombs they spawn (Shatter
// Bomb et al.). One DB-driven category check replaces the deeper
// Impact→fireMode→m_Owner→r_Owner Impact-trace TrackStats was doing.
//
// Rate constants: no DB-driven HP→points scaling exists. The catalog has
// a registered "Boost Charge Rate" property (337) but zero data rows
// reference it. The constants below are calibrated from old-GA playtest
// evidence: 27000 damage / 32000 heal = 100 morale, with retail morale
// devices carrying their innate +70% Output Mod. Reversed to "before
// Output Mod" base rates: 450 dmg/point and 540 heal/point. Runtime
// Output Mod factor lookup reproduces retail within ~2%.
namespace MoraleCredit {

// Award morale credit for one damage/heal event.
//
//   recipient        — pet/turret credit lands on the deploying player,
//                      so caller passes the resolved human (not the
//                      pet pawn).
//   magnitude        — positive HP magnitude actually applied. For heals,
//                      gets clamped to fMissingHealth here (overheals
//                      don't credit, matching the existing STYPE_HEALING
//                      contract).
//   isHeal           — true for heal events.
//   fMissingHealth   — target's missing HP at the moment of apply.
//                      Heal-only (ignored for damage). <0 treated as 0.
//   deviceModeId     — source TgDeviceFire's nDeviceModeId. Used for the
//                      morale-source anti-feedback filter (DB lookup
//                      via `asm_data_set_devices.slot_used_value_id`).
//   originDeviceId   — for damage from a morale-spawned deployable (Shatter
//                      Bomb etc.), the static device_id of the spawning morale
//                      device, recovered by the caller via the deployable's
//                      s_SpawnerDeviceMode chain. 0 when not deployable-sourced.
//                      Closes the bomb-chain anti-feedback hole that deviceModeId
//                      alone can't (the explosion carries the deployable's mode).
void Award(ATgPawn* recipient, float magnitude, bool isHeal,
           float fMissingHealth, int deviceModeId, int originDeviceId = 0);

}  // namespace MoraleCredit
