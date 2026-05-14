#include "src/GameServer/TgGame/TgPlayerController/TraceWeaponFire/TgPlayerController__TraceWeaponFire.hpp"
#include "src/Utils/Logger/Logger.hpp"

float* __fastcall TgPlayerController__TraceWeaponFire::Call(
	void* This, void* edx, float* outHit,
	void* Dev,
	float StartX, float StartY, float StartZ,
	float EndX, float EndY, float EndZ,
	void* TraceActor)
{
	float* ret = CallOriginal(This, edx, outHit, Dev, StartX, StartY, StartZ, EndX, EndY, EndZ, TraceActor);

	// Diagnostic: dump trace inputs + output. Gated on "fire_gate" channel
	// (same channel as the rest of the firing-while-jetpacking dispatcher
	// instrumentation in UObject__ProcessEvent.cpp).
	//
	// The pattern we're hunting: cached aim rotation collapses to (0, 32764,
	// 0) during flight, indicating Rotator(HitLocation - WeaponLoc) is
	// receiving a vector that's effectively (-K, +ε, 0) with the Z component
	// pinned to zero. This hook tells us whether the Z-pin originates in
	// EndTrace itself (geometry-of-input problem upstream in GetBaseAimRotation)
	// or in the trace's hit detection (HitLocation Z matches WeaponLoc.Z by
	// accident or as a clamped-to-trace-plane artifact).
	if (Logger::IsChannelEnabled("fire_gate")) {
		float hx = outHit ? outHit[0] : 0.f;
		float hy = outHit ? outHit[1] : 0.f;
		float hz = outHit ? outHit[2] : 0.f;
		// Compare against EndTrace — if hit==EndTrace exactly, no real hit
		// (function fell through to the "copy EndTrace to outHit" fallback).
		bool noHit = (hx == EndX) && (hy == EndY) && (hz == EndZ);
		Logger::Log("fire_gate",
			"[TraceWeaponFire] this=%p Dev=%p TraceActor=%p\n"
			"  Start=(%.1f, %.1f, %.1f)  End=(%.1f, %.1f, %.1f)\n"
			"  Hit  =(%.1f, %.1f, %.1f)  noHit=%d\n",
			This, Dev, TraceActor,
			StartX, StartY, StartZ,
			EndX, EndY, EndZ,
			hx, hy, hz,
			(int)noHit);
	}

	return ret;
}
