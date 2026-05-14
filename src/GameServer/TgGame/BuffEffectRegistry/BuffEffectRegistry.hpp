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
// Lifetime: TEMPLATE effects are added at `BuildEffectGroup` time (once per
// template, lives with the device's fire mode `s_EffectGroupList`). CLONE
// effects are added per-instance in `TgEffectGroup__CloneEffectGroup.cpp` —
// each apply creates fresh `UTgEffect`s via `TgEffect__CloneEffect::Call` and
// marks them if their template is marked. Clones are deliberately NOT
// RootSet: while the clone group is alive (RootSet'd) and references the
// clone via m_Effects, UE3 GC keeps it; once the group is removed from
// `s_AppliedEffectGroups`, GC eventually reclaims the clone effects too.
//
// **Stale-entry hazard**: clone pointers are never `Forget()`ed when the
// group is removed, so the set accumulates dead-clone addresses over a
// session. When a fresh allocation lands on a reused address, `IsBuff()`
// returns a false positive. This is benign for the apply path
// (`SetEffectRep`'s buff-route runs alongside UC `ApplyEffect`, which is
// already a silent no-op for modifier props the pawn doesn't carry in
// `s_Properties`) AND for the remove path (`UObject__ProcessEvent`'s
// `TgEffectRemove` hook is ADDITIVE — `ApplyBuffEffectFromHook(bRemove=true)`
// runs alongside UC `TgEffect.Remove`, not instead of it). The asymmetric
// behavior we used to have — buff-route Remove REPLACING UC Remove — caused
// the iMINIGUN stuck-root bug; see UObject__ProcessEvent.cpp:507-535 for the
// fix. Until clones get `Forget()`ed at remove time, false positives are
// occurring in normal play but are no longer load-bearing.
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
