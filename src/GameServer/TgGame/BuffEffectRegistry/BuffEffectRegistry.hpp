#pragma once

#include <unordered_set>

class UTgEffect;

// BuffEffectRegistry — side-set of UTgEffect*'s that originated from a DB row
// with `class_res_id == 157` (TgEffectBuff). The class itself is stripped from
// this binary's GObjObjects, so `BuildEffectGroup` falls back to base
// `Class TgGame.TgEffect`. UC then dispatches `TgEffect.ApplyEffect` (which
// routes to `ApplyToProperty`/`SetProperty` on the target pawn's
// `s_Properties[propId]->m_fRaw`) — silently no-op for "modifier" props
// (113 Accuracy, 65 Effect Damage Modifier, 352 AOE Radius, …) that the pawn
// never carries in s_Properties.
//
// The proper UC path is `TgEffectBuff.ApplyEffect → Pawn.ApplyBuff` — it
// appends an FBuffInfo to `m_EffectBuffInfo`, where the layered formula in
// `TgPawn::GetBuffedProperty` (vtable[0x56C]) actually consults it. This
// registry lets `UObject__ProcessEvent::Call` recognize "this is a buff effect"
// at apply/remove time and route through `TgPawn__ApplyBuff::Call` instead,
// mirroring `unrealscript/TgGame/Classes/TgEffectBuff.uc:121-200` line for
// line.
//
// Side-set rather than a flag bit on the UTgEffect layout: the binary's
// TgEffect class only declares fields up to offset 0x6C; bytes past that may
// be reused natively by subclasses or alignment. Keeping the flag external
// avoids the risk.
//
// Lifetime: entries are added at `BuildEffectGroup` time (once per template
// effect, never per-clone — clones share the template effect pointer per
// `TgEffectGroup__CloneEffectGroup.cpp:55-57`). Templates live as long as
// their owning `s_EffectGroupList` (effectively the deployable / device's
// fire mode lifetime). We don't currently observe template GC events, so
// stale entries can accumulate; the keys are pointer-typed so the only
// failure mode is a NEW UTgEffect happening to land at the same address as a
// freed one — astronomically unlikely under UE3 GC pacing, and the
// downstream path is still safe (worst case: a non-buff effect routes
// through ApplyBuff once and applies an additive 0 to the registry).
namespace BuffEffectRegistry {

inline std::unordered_set<UTgEffect*>& Set() {
	static std::unordered_set<UTgEffect*> s;
	return s;
}

inline void Mark(UTgEffect* e) {
	if (e) Set().insert(e);
}

inline bool IsBuff(UTgEffect* e) {
	return e != nullptr && Set().count(e) != 0;
}

inline void Forget(UTgEffect* e) {
	if (e) Set().erase(e);
}

}  // namespace BuffEffectRegistry
