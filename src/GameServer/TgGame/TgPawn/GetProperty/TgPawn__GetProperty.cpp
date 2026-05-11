#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"

// TgPawn::GetProperty — pure read.
//
// Original native @ 0x109dd2e0 uses the TMap at pawn+0x400 (propId → s_Properties
// index) for O(1) lookup, then indexes into s_Properties (pawn+0x3F4).
//
// Historical context: this hook used to do a linear scan because our
// InitializeProperty didn't maintain the TMap, so the native returned null
// for any property we added. That gap is closed (memory: reference_pawn_property_tmap.md
// — fixed 2026-05-07 in InitializeProperty via TMap_Set::Call). The TMap is
// now reliable, so we delegate straight back to the native and pay zero hook
// overhead. The hook stays installed so future modifications have a place to
// land without re-plumbing call sites.
//
// IMPORTANT — must stay side-effect free. A prior version mirrored the found
// property's m_fRaw/m_fMaximum back into APawn::Health, r_nHealthMaximum and
// the PRI on every read. That turned every UC `Pawn.GetProperty(51)` into a
// "resurrect to property's stale value" — directly fighting any code path
// that mutated APawn::Health (e.g. damage handlers). HP sync is the
// ApplyProperty native's job (called via SetProperty), not GetProperty's.
UTgProperty* __fastcall TgPawn__GetProperty::Call(ATgPawn* Pawn, void* edx, int PropertyId) {
	return CallOriginal(Pawn, edx, PropertyId);
}
