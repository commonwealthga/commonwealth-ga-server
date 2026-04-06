#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnEquipCharDevice : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, UTgSeqAct_EquipCharDevice*),
	0x109c0d30,
	TgPawn__ServerOnEquipCharDevice> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevice* Action);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevice* Action) {
		m_original(Pawn, edx, Action);
	};
};
