#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__GetJetpackTrailId : public HookBase<
	int(__fastcall*)(ATgPawn*, void*),
	0x109bfbd0,
	TgPawn__GetJetpackTrailId> {
public:
	static int __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline int __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		return m_original(Pawn, edx);
	};
};
