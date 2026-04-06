#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackKilledPlayer : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c0900,
	TgPawn__TrackKilledPlayer> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
		m_original(Pawn, edx, nDeviceModeID);
	};
};
