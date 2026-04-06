#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SpawnLoot : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgRepInfo_TaskForce*),
	0x109c0a90,
	TgPawn__SpawnLoot> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
		m_original(Pawn, edx, tf);
	};
};
