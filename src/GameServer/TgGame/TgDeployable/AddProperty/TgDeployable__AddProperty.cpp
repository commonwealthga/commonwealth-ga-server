#include "src/GameServer/TgGame/TgDeployable/AddProperty/TgDeployable__AddProperty.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"

// Native exec at 0x10a194e0 is a no-op stub (per canonical doc §0 stripped
// list). Reimplement the canonical "add a property to a deployable":
// idempotent upsert into s_Properties + dispatch SetProperty so the engine
// mirrors run.
//
// Upsert (not bare append): callers seed identity defaults first and may
// re-call with DB-driven values to override. Linear scan over s_Properties
// keeps the array unique-by-propId so SetProperty/GetProperty's linear
// lookup reads a single consistent value.
//
// Callers:
//   - TgDeployable__InitializeDefaultProps invokes Deployable->AddProperty(...)
//     for each seeded slot at spawn (identity defaults + DB-driven values).
//   - Any UC bytecode that calls `Deployable.AddProperty(...)` now lands
//     here instead of the empty stub.
//
// Unlike the pawn variant, deployables don't have an s_PropertyIndx TMap —
// `ATgDeployable::SetProperty` (intact @ 0x10a1c940) does a plain linear
// scan over s_Properties, so no map registration is needed. ApplyProperty
// (intact @ 0x10a1ade0, vtable slot 251) only handles props 8 + 278
// natively; other props get their engine-field mirror via our own
// `TgDeployable__SetProperty` Detour extension.
void __fastcall TgDeployable__AddProperty::Call(ATgDeployable* Deployable, void* /*edx*/,
                                                int nPropId, float fBase, float fRaw,
                                                float FMin, float FMax) {
	if (!Deployable) return;

	// Upsert — locate an existing entry for this propId, otherwise allocate.
	// TArray::Add routes through GAllocator::Realloc, matching whatever the
	// engine eventually frees the buffer with.
	UTgProperty* Property = nullptr;
	for (int i = 0; i < Deployable->s_Properties.Count; ++i) {
		UTgProperty* p = Deployable->s_Properties.Data[i];
		if (p && p->m_nPropertyId == nPropId) {
			Property = p;
			break;
		}
	}
	if (!Property) {
		Property = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
			ClassPreloader::GetTgPropertyClass(), -1, 0, 0, 0, 0, 0, 0, 0);
		Deployable->s_Properties.Add(Property);
	}

	Property->m_nPropertyId = nPropId;
	Property->m_fBase       = fBase;
	Property->m_fRaw        = fRaw;
	Property->m_fMinimum    = FMin;
	Property->m_fMaximum    = FMax;

	// Dispatch SetProperty so the intact native (0x10a1c940) runs +
	// our TgDeployable__SetProperty Detour fan-out for HP / HP_MAX / DRI.
	Deployable->SetProperty(nPropId, fRaw);
}
