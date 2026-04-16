#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"

// TgPawn::GetProperty — pure read.
//
// Original native @ 0x109dd2e0 uses a TMap at pawn+0x400 (propId → index)
// for O(1) lookup, then indexes into s_Properties (pawn+0x3F4). We hook this
// because InitializeProperty doesn't currently maintain the TMap — only
// appends to s_Properties — so the original would return null for any
// property we add. The linear scan below works regardless of TMap state.
//
// IMPORTANT — this MUST stay side-effect free. A prior version mirrored the
// found property's m_fRaw/m_fMaximum back into APawn::Health, r_nHealthMaximum
// and the PRI on every read. That turned every UC `Pawn.GetProperty(51)` into
// a "resurrect to property's stale value" — directly fighting any code path
// that mutated APawn::Health (e.g. damage handlers). HP sync is the
// ApplyProperty native's job (called via SetProperty), not GetProperty's.
UTgProperty* __fastcall TgPawn__GetProperty::Call(ATgPawn* Pawn, void* /*edx*/, int PropertyId) {
	if (!Pawn || !Pawn->s_Properties.Data) {
		return nullptr;
	}
	for (int i = 0; i < Pawn->s_Properties.Num(); ++i) {
		UTgProperty* prop = Pawn->s_Properties.Data[i];
		if (prop && prop->m_nPropertyId == PropertyId) {
			return prop;
		}
	}
	return nullptr;
}
