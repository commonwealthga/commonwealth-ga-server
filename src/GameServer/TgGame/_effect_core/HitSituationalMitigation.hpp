#pragma once

#include "src/pch.hpp"

// HitSituationalMitigation — stops a type-505 "Hit-Situational" protection
// debuff from mitigating the very shot that triggered it.
//
// THE DEFECT (Killer Instinct, skill 836 / eg 16596, prop 155 −10, gate
// "target HP > 75%"). `TgDeviceFire.ApplyHit` submits the HP-gated situational
// pass BEFORE the pass that carries the base damage:
//
//   ApplyHit:1409-1412   SubmitHitEffects(.., 0/1, 505, 1270/1271)   ← debuff lands
//   ApplyHit:1445        SubmitHitEffects(.., 0,   264)              ← base damage
//   ApplyHit:1495        SubmitHitEffects(.., 1,   264)
//
// `SubmitEffect → ProcessEffect → ApplyEffects → ApplyToProperty` is fully
// synchronous, so by the time `TgEffectDamage.ProtectionModifier` reads prop
// 155 the debuff is already in `m_fRaw`. Every other 505 dispatch in ApplyHit
// (backstab 509 at 1480/1485, situational melee at 1491) sits AFTER the damage
// submit — only the 1270/1271 block is ahead of it.
//
// Measured on a bare target (max HP 1300, Physical protection 30, Ballista OC,
// raw 1600, attack rating 100): shot 1 dealt 1170 with Killer Instinct slotted
// vs 1120 without. 1170 is NOT a mitigation figure — it is the anti-one-shot
// health cap in `TgEffectDamage.ApplyEffect:174-200` (`Health − ceil(0.1 ×
// maxHealth)` = 1300 − 130), which arms only at exactly full HP because
// `ApplyHit:1287` compares two ints (`Health / r_nHealthMaximum > 90/100`).
// The uncapped figure is 1600 × 0.80 = 1280, i.e. the FULL −10 reaches the
// triggering shot. The cap also inflates the reported "mitigated" number,
// since `SendDamageMessages` reports `pre − post` after the clamp.
//
// THE FIX. Everything in that chain is UnrealScript, so the apply order is not
// ours to change. Instead the debuff applies exactly as it does today —
// canonical gate, potency query, lifetime, HUD slot, Remove path all untouched
// — and we lend the target its pre-debuff `m_fRaw` back for the duration of
// this impact's mitigation only:
//
//   NoteDebuffApplied()   ← TgEffect::CheckEffectBuffModifier, on a 505/1270/1271
//                           protection effect. Runs at TgEffect.uc:115, i.e.
//                           BEFORE ApplyToProperty, so m_fRaw is still clean.
//   BeginImpactMitigation() ← same hook, on a TgEffectDamage effect. Runs at
//                           TgEffectDamage.uc:131, before ProtectionModifier.
//                           Swaps the recorded pre-value back in.
//   EndImpactMitigation()  ← TgEffectManager::RemoveEffectGroupsByCategory(431, 99).
//                           Runs at TgEffectDamage.uc:206 — after mitigation and
//                           the health cap, before TakeDamage. Puts the debuff back.
//
// The two brackets sit inside one UC function with no `return` between them,
// and TgEffectDamage.uc:202 gates the closing call on the target being a pawn —
// which is why we only ever arm for pawn targets. So the swap cannot leak past
// the damage application that opened it.
//
// Not covered, deliberately: category-963 damage groups (they skip the opening
// bracket at TgEffectDamage.uc:128) and DOT interval ticks after the first
// (`m_bUseOnInterval` short-circuits the same branch). Neither is a triggering
// shot, so both keep today's behaviour.
namespace HitSituationalMitigation {

// Record a type-505 Hit-Situational protection debuff about to be written to
// the target, capturing the property's value before the write. No-op unless
// the effect group is type 505 with situational 1270/1271, the property is one
// `ProtectionModifier` consumes, and the target is a pawn.
void NoteDebuffApplied(UTgEffect* effect);

// Open the mitigation window for a TgEffectDamage effect: lend back the
// pre-debuff value of anything this shot's own 505 pass just applied to the
// same victim from the same instigator.
void BeginImpactMitigation(UTgEffect* damageEffect);

// Close the window and restore the debuff. Safe to call when no window is open.
void EndImpactMitigation(ATgEffectManager* Manager);

}  // namespace HitSituationalMitigation
