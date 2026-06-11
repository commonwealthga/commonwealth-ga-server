#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// APawn::physicsRotation (0x10ca5330). The binary's PHYS_Flying branch reads
// Controller+0x258 BEFORE its Controller null check — a controller-less
// flying pawn (e.g. a flying-bot corpse after Dying.Timer's
// Controller.Destroy()) crashes with read-AV at 0x258 (2026-06-11 production
// crash @ 0x10ca5412). Guard: for that exact state, run the body under
// PHYS_Falling (its branch null-checks properly; rotation-from-velocity is
// the right behavior for an uncontrolled pawn anyway), then restore.
class Pawn__PhysicsRotation : public HookBase<
	void(__fastcall*)(APawn*, void*, float, float, float),
	0x10ca5330,
	Pawn__PhysicsRotation> {
public:
	static void __fastcall Call(APawn* Pawn, void* edx, float a, float b, float c);
	static inline void __fastcall CallOriginal(APawn* Pawn, void* edx, float a, float b, float c) {
		m_original(Pawn, edx, a, b, c);
	}
};
