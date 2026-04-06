#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnEquipCharDevices : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, UTgSeqAct_EquipCharDevices*),
	0x109c0d40,
	TgPawn__ServerOnEquipCharDevices> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action) {
		m_original(Pawn, edx, Action);
	};
};
