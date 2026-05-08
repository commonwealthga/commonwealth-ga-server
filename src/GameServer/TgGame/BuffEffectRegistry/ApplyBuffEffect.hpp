#pragma once

class UTgEffect;
class AActor;

// Port of `TgEffectBuff.ApplyBuff` (UC) for effects originally tagged
// `class_res_id == 157` in the DB. The class itself is stripped from this
// binary, so `BuildEffectGroup` constructs them as base TgEffect and tags
// them via `BuffEffectRegistry`.
//
// `TgEffect.ApplyEffect` is a regular UC function (`unrealscript/TgGame/
// Classes/TgEffect.uc:95`, no `native` keyword). Yet a diagnostic that logs
// every UFunction passing through `UObject::ProcessEvent` (our hook at
// `0x11347C20`) sees `TgEffect.Remove` (because our reimplemented
// `TgEffectGroup__RemoveEffects` explicitly dispatches it via ProcessEvent)
// but NOT `TgEffect.ApplyEffect` — nor any of the upstream apply chain
// (`TgEffectManager.ProcessEffect`, `TgEffectGroup.ApplyEventBasedEffects`,
// `TgPawn.ProcessEffect`, `TgDeviceFire.SubmitEffect`). Yet `m_fCurrent` IS
// being populated per-pulse, so the apply DOES run. Best current
// hypothesis: somewhere in the chain a binary-resident shim invokes the UC
// body via a path that bypasses our hook (direct `Function->Func` call,
// alternate ProcessEvent entry point, or a stripped-and-rewritten exec
// stub). Worth a deeper Ghidra dig later, but practically we route from
// `TgEffectManager::SetEffectRep`'s post-original hook — that one DOES fire
// reliably (visible in the same log as `[SET-REP]` lines), once per applied
// clone, with `s_AppliedEffectGroups` already populated.
//
// Mirror of `unrealscript/TgGame/Classes/TgEffectBuff.uc:121-167`:
//   fProratedAmount = (m_fLifeTime>0 && m_fApplyInterval>0 && m_bUseOnInterval)
//                       ? m_fBase / (m_fLifeTime / m_fApplyInterval)
//                       : m_fBase;
//   header = { m_nPropertyId, m_nPropertyValueId,
//              m_EffectGroup.m_nRequiredSkillId,
//              m_EffectGroup.m_nReqDeviceInstanceId>0 ? ... : 0 };
//   if (!bRemove) { m_fCurrent = fProratedAmount;
//                   Pawn.ApplyBuff(header, calc, fProratedAmount, false, src); }
//   else         { Pawn.ApplyBuff(header, calc, m_fCurrent,        true,  src);
//                  m_fCurrent = 0; }
//
// `BuildEffectGroup` divides class-157 percent base values by 100 (FRACTION
// storage) so the existing class-80 ApplyToProperty path produces consistent
// results for non-buff routes. The buff registry's formula expects PERCENT
// (`v3 = v2 * (1 + ΣfPercent/100) + ΣfMod`), so we re-multiply by 100 here
// for calc 68/69. Matches the symmetric reversal in
// `ReapplyCharacterSkillTree.cpp:339`.
//
// `buffSourceType` = 4 (OTHER) for station-applied effects. Per the formula
// in `TgPawn__ApplyBuff.cpp:14-23`, OTHER and SELF both contribute to the
// final layer (`v3 = v2 * (1 + (ΣfSelfPercent + ΣfPercent)/100)
// + ΣfSelfMod + ΣfMod`), so picking OTHER yields correct math for both the
// placer-self case and the teammate case without needing to derive
// `GetBuffType` from a (template-shared, possibly stale) m_Instigator.
void ApplyBuffEffectFromHook(UTgEffect* effect, AActor* target, bool bRemove);
