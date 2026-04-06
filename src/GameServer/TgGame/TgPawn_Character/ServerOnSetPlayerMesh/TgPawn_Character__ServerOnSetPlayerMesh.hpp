#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__ServerOnSetPlayerMesh : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int),
	0x109c1340,
	TgPawn_Character__ServerOnSetPlayerMesh> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, int MeshAsmId);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, int MeshAsmId) {
		m_original(Pawn, edx, MeshAsmId);
	};
};
