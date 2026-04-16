#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Instrumentation-only trampoline on UTgPawn::AddDamageInfo @ 0x109e5b40.
// Logs the return address of every caller so we can identify who is invoking
// the floating-damage-text path in the shipped binary (static analysis found
// zero code xrefs; this will catch ProcessEvent-internal / vtable dispatch).
class TgPawn__AddDamageInfo : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*, void*, unsigned char),
	0x109e5b40,
	TgPawn__AddDamageInfo> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Source, void* DamageString, unsigned char Type);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Source, void* DamageString, unsigned char Type) {
		m_original(Pawn, edx, Source, DamageString, Type);
	}
};
