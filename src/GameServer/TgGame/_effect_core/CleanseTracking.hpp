#pragma once

#include "src/pch.hpp"

// CleanseTracking — attributes RemoveEffectGroupsByCategory purges to the
// player + device that cast the cleanse, so the count the reimplementation
// already computes (and discarded until now) reaches match stats.
//
// A cleanse is a property-140 ("Remove Effect") effect: TgEffect.ApplyEffect
// sees m_nPropertyId == 140 and calls RemoveEffectGroupsByCategory on the
// TARGET's manager. The manager call carries no instigator, so provenance is
// captured one step earlier: CheckEffectBuffModifier runs on every effect
// before its value is consumed (TgEffect.uc:115 — the same ordering the
// HitSituationalMitigation brackets rely on), and there the effect still
// knows its group → instigator, target, and source device.
//
//   NotePendingCleanse()  ← TgEffect::CheckEffectBuffModifier, m_nPropertyId 140
//   OnRemoved()           ← TgEffectManager::RemoveEffectGroupsByCategory,
//                           after the purge loop, with the `removed` count
//
// The pending record is single-slot: within one ApplyEffect the note is
// immediately followed by that effect's own Remove call, nothing interleaves
// (ApplyEffects applies effects one at a time, fully synchronously). OnRemoved
// consumes only when the manager's owner matches the noted target; the
// (431, 99) mitigation bracket — which enters the same function on every
// damaging impact — is skipped before the consume check, so it can neither
// count as a cleanse nor eat a pending record.
//
// If the note never fires for a real cleanse (ApplyEffect branching to the
// purge before CheckEffectBuffModifier would contradict the KI ordering, but
// TgEffect.uc:115's surroundings are unverified for the 140 branch — doc
// §8.3), removals show up as "unattributed" on the `devusage` channel instead
// of being miscounted. Self-cleanse counts: Sealed Systems and Purity are
// self-target devices, so stripping your own debuffs IS effective use.
namespace CleanseTracking {

// Record the pending cleanse context off a property-140 effect. No-op for
// any other property id.
void NotePendingCleanse(UTgEffect* effect);

// Consume the pending record and credit `removed` stripped groups to the
// noted player + device. Call after the purge loop with the loop's count;
// safe to call for every invocation (internal callers are filtered here).
void OnRemoved(ATgEffectManager* Manager, int nCategoryCode, int nQuantity,
               int removed);

}  // namespace CleanseTracking
