#pragma once

#include "src/pch.hpp"

// ScaleTargetProperties — pure canonical delegation for spawn-time stat
// scaling of deployables / pets from the deploying player's buff registry.
//
// Walks the target's `s_Properties` and, for each prop, asks the deployer's
// intact `GetBuffedProperty(BUFF_DEVICE, prop_id, base)` what the buffed
// value is. Writes the result back via `target->SetProperty(prop_id, buffed)`.
//
// NO editorial tables, NO manual 3-layer formula, NO prop-ID redirects.
// The intact `ConvertPropToPropList` (0x109e5220) + `CheckBuffInfoList`
// (0x109cd4a0) natives do all the work. Where canonical expansion misses
// a scenario (e.g. Station Buff prop 339 → deployable HP), we observe in
// playtest and decide per case — see design Risk Register.
//
// See `.planning/effect-buff-property-canonical.md` §0 (call intact natives)
// and `.planning/2026-05-26-producer-pure-canonical-design.md`.
namespace ScaleTargetProperties {

// Deployable target — no buff registry of its own (extends AActor, not APawn).
// Spawn-time pre-bake into s_Properties is the only mechanism for scaling its
// intrinsic stats from the deployer's buffs.
void Apply(ATgPawn* deployer, ATgDeployable* target, int sourceDeviceInstId);

// Pet target — IS a TgPawn, has m_EffectBuffInfo. The spawn-time bake covers
// engine-mirrored stats (HP_MAX via r_nHealthMaximum, LifeSpan, etc.) that
// are read once and cached rather than lazily re-queried.
void Apply(ATgPawn* deployer, ATgPawn* target, int sourceDeviceInstId);

}  // namespace ScaleTargetProperties
