#include "src/GameServer/TgGame/TgPawn/SetProperty/TgPawn__SetProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Pass-through wrap of TgPawn::SetProperty (binary @ 0x109bf420).
//
// Currently instrumented for props 70 (AirSpeed) and 59 (FlightAcceleration)
// to chase a "flight speed compounds across fly/die/respec cycles" leak —
// SET-REP/REMOVE-EFFECTS counts looked symmetric in the log but the user's
// AirSpeed crept up by ~5% per cycle. Per-call PRE/POST snapshot captures:
//   - the value being requested (NewValue)
//   - s_Properties[propId].m_fRaw before the call
//   - m_fRaw after (binary writes it inside SetProperty before fan-out)
//   - the engine-cached field the fan-out targets (Pawn.AirSpeed at +0x278,
//     Pawn.r_FlightAcceleration at +0xF84) before and after
// Diff PRE→POST exposes whether SetProperty actually mutated the field, and
// whether m_fRaw-after lines up with the requested NewValue (mismatch means
// the binary's apply path is doing something we don't expect, e.g. clamping,
// re-aggregation, or silent no-op).
//
// Stack-trace capture: the hook's caller chain doesn't survive the inline
// VM dispatch, but the immediate __builtin_return_address(0) gives the call
// site that funneled into our hook (usually our own SetPawnProperty or a UC
// ApplyToProperty bytecode site). When that pointer alternates we know there
// are multiple write paths to compare.
//
// To disable: change the `if (PropertyId == 70 || PropertyId == 59)` guard
// to false. To repurpose for another prop: edit that condition.
void __fastcall TgPawn__SetProperty::Call(ATgPawn* Pawn, void* edx, int PropertyId, float NewValue) {
	const bool track = Pawn && (PropertyId == 70 || PropertyId == 59);
	if (!track) {
		CallOriginal(Pawn, edx, PropertyId, NewValue);
		return;
	}

	// Read m_fRaw via linear scan — same as the existing pattern in
	// ReapplyCharacterSkillTree's reversal pass. Linear scan rather than
	// the pawn's TMap lookup so we always find the FIRST matching property
	// (any duplicates from re-init would show up as a divergence between
	// "what we updated" and "what fan-out reads").
	auto findRaw = [&]() -> float {
		if (!Pawn->s_Properties.Data) return -99999.0f;
		for (int j = 0; j < Pawn->s_Properties.Num(); ++j) {
			UTgProperty* p = Pawn->s_Properties.Data[j];
			if (p && p->m_nPropertyId == PropertyId) return p->m_fRaw;
		}
		return -99999.0f;
	};

	const float rawBefore       = findRaw();
	const float airBefore       = Pawn->AirSpeed;                  // engine field @ +0x278
	const float flightAccBefore = Pawn->r_FlightAcceleration;      // engine field @ +0xF84
	void* caller = __builtin_return_address(0);

	CallOriginal(Pawn, edx, PropertyId, NewValue);

	const float rawAfter        = findRaw();
	const float airAfter        = Pawn->AirSpeed;
	const float flightAccAfter  = Pawn->r_FlightAcceleration;
	const bool movementRepChanged =
		(PropertyId == 70 && airBefore != airAfter) ||
		(PropertyId == 59 && flightAccBefore != flightAccAfter);
	if (movementRepChanged) {
		// AirSpeed and r_FlightAcceleration replicate through bNetDirty-gated blocks.
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
	}

	const char* propName = (PropertyId == 70) ? "AirSpeed" : "FlightAccel";
	Logger::Log("effects",
		"[SET-PROP] pawn=%p prop=%d(%s) NewValue=%.4f  m_fRaw %.4f -> %.4f (Δ%+.4f)  "
		"AirSpeed %.3f -> %.3f  r_FlightAccel %.4f -> %.4f  dirty=%d caller=%p\n",
		(void*)Pawn, PropertyId, propName, NewValue,
		rawBefore, rawAfter, rawAfter - rawBefore,
		airBefore, airAfter,
		flightAccBefore, flightAccAfter,
		(int)movementRepChanged,
		caller);
}
