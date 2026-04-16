#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgPawn::TGPostRenderFor @ 0x109e5f40 — native invoked per pawn during HUD
// render. Signature: TGPostRenderFor(APlayerController* PC, UCanvas* Canvas,
// FVector CameraPosition, FVector CameraDir). Stack args total 32 bytes
// (PC* + Canvas* + 3*float + 3*float). Thiscall: this in ECX.
// We log the pawn + count at +0xFFC to see whether bots get iterated.
class TgPawn__TGPostRenderFor : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, void*, void*, float, float, float, float, float, float),
	0x109e5f40,
	TgPawn__TGPostRenderFor> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, void* PC, void* Canvas,
		float cpX, float cpY, float cpZ, float cdX, float cdY, float cdZ);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, void* PC, void* Canvas,
		float cpX, float cpY, float cpZ, float cdX, float cdY, float cdZ) {
		m_original(Pawn, edx, PC, Canvas, cpX, cpY, cpZ, cdX, cdY, cdZ);
	}
};
