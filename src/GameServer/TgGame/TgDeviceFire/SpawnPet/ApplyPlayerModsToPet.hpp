#pragma once

#include "src/pch.hpp"

// Spawn-time pre-bake of the deploying player's buffs onto the pet's
// `s_Properties`. Thin wrapper — all logic lives in
// `_effect_core/ScaleTargetProperties` (pure canonical delegation, no
// editorial tables). See `.planning/2026-05-26-producer-pure-canonical-design.md`.
//
// `sourceDeviceInstId` scopes the canonical `GetBuffedProperty` query to the
// spawning device's own rolled mods + wildcard skills. Pass 0 if the spawn
// device is unknown.
//
// What this DOES NOT do (intentional — see design Risk Register):
//   - Bridge player's `m_EffectBuffInfo` entries onto the pet's
//     `m_EffectBuffInfo`. Pet range / accuracy / AoE mods (player props
//     381 / 383 / 382) won't reach the pet's fire-time queries until/unless
//     playtest confirms they should — then add a one-shot copy here.
//   - Scale pet damage. That flows through `TgEffect::CheckOwnerPetBuff`
//     (0x10a6f290) at fire time — orthogonal to spawn-time bake.
namespace ApplyPlayerModsToPet {
	void Apply(ATgPawn* deployingPawn, ATgPawn* petPawn, int sourceDeviceInstId);
}
