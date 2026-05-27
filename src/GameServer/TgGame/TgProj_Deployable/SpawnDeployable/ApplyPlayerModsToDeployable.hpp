#pragma once

#include "src/pch.hpp"

// Spawn-time pre-bake of the deploying player's buffs onto the deployable's
// `s_Properties`. Thin wrapper — all logic lives in
// `_effect_core/ScaleTargetProperties` (pure canonical delegation, no
// editorial tables). See `.planning/2026-05-26-producer-pure-canonical-design.md`.
//
// `sourceDeviceInstId` is the deploying device's `r_nDeviceInstanceId`; it
// scopes the canonical `GetBuffedProperty` query so only the deploy device's
// own rolled mods + wildcard skills fold in (not every other equipped device's
// mods). Pass 0 for skill-spawned deployables; only wildcard entries fold then.
//
// Idempotent per spawn — runs once at deploy and snapshot stays until the
// deployable is destroyed (deployables have no `m_EffectBuffInfo` to re-query
// from, so reactive re-baking on deployer buff changes is out of scope).
namespace ApplyPlayerModsToDeployable {
	void Apply(ATgPawn* pawn, ATgDeployable* deployable, int sourceDeviceInstId);
}
