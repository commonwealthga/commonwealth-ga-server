#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnSetPlayerMesh : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c0d10,
	TgPawn__ServerOnSetPlayerMesh> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int MeshAsmId);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int MeshAsmId) {
		m_original(Pawn, edx, MeshAsmId);
	};
};
