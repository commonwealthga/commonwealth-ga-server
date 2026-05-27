#include "src/GameServer/TgGame/TgPawn/AddProperty/TgPawn__AddProperty.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/GameServer/Core/TMap/Set/TMap__Set.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native exec at 0x109bf410 is a no-op stub (per decompiled/.../ATgPawn/STUBS.md).
// Reimplement the canonical "add a property to a pawn" semantics: construct a
// UTgProperty, populate the 5 fields, append to s_Properties, register the
// (propId → array-index) entry in the s_PropertyIndx TMap @ pawn+0x400, then
// dispatch SetProperty so the intact ApplyProperty cascade fans the value
// into any engine-side mirror (r_nHealthMaximum, GroundSpeed, etc.).
//
// Callers:
//   - TgPawn__InitializeDefaultProps invokes Pawn->AddProperty(...) per stat
//     to seed the pawn's property table at spawn.
//   - Any UC bytecode that calls `Pawn.AddProperty(...)` now lands here
//     instead of the empty stub (matches the canonical contract that this
//     is the one stripped writer for the pawn property table).
//
// The TMap registration is load-bearing: the binary's `TgPawn::SetProperty`
// (0x109bf420) does its lookup via vtable[0x4F0] → `FUN_10b52ba0` which
// queries this exact TMap. Without an entry, SetProperty silently no-ops
// for the prop. See `reference_property_id_label_lookup.md` for the
// historical bug pattern.
void __fastcall TgPawn__AddProperty::Call(ATgPawn* Pawn, void* /*edx*/,
                                          int nPropId, float fBase, float fRaw,
                                          float FMin, float FMax) {
	if (!Pawn) return;

	UTgProperty* Property = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0, 0, 0, 0, 0, 0);

	Property->m_nPropertyId = nPropId;
	Property->m_fBase       = fBase;
	Property->m_fRaw        = fRaw;
	Property->m_fMinimum    = FMin;
	Property->m_fMaximum    = FMax;

	// TArray::Add routes through GAllocator::Realloc, matching whatever the
	// engine eventually frees the buffer with.
	const int Count = Pawn->s_Properties.Count;
	Pawn->s_Properties.Add(Property);

	// Register (propId → s_Properties index) in the TMap at pawn+0x400.
	// The s_PropertyIndx field is `UnknownData00` in the SDK header — the
	// TMap is a UE3 MapProperty (TMap<int,int>, 0x3C bytes wide). Required
	// for the binary's SetProperty (0x109bf420) vtable lookup to find this
	// prop on subsequent calls.
	TMap_Set::Call((void*)((char*)Pawn + 0x400),
		(unsigned int)nPropId,
		(unsigned int)Count);

	// SetProperty fans the value out through the intact ApplyProperty
	// cascade — engine-side mirrors (r_nHealthMaximum, GroundSpeed,
	// FlightAccel, …), DRI sync, and any UC OnPropertyApplied event.
	Pawn->SetProperty(nPropId, fRaw);
}
