#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackCompleteKillInfo : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, int, int, int, FVector, FVector, FVector, unsigned long),
	0x109c0920,
	TgPawn__TrackCompleteKillInfo> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nKillerCharacterID, int nKillerDeviceModeID, int nVictimCharacterID, int nVictimDeviceModeID, FVector KillerLocation, FVector VictimLocation, FVector PetLocation, unsigned long bPetKill);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nKillerCharacterID, int nKillerDeviceModeID, int nVictimCharacterID, int nVictimDeviceModeID, FVector KillerLocation, FVector VictimLocation, FVector PetLocation, unsigned long bPetKill) {
		m_original(Pawn, edx, nKillerCharacterID, nKillerDeviceModeID, nVictimCharacterID, nVictimDeviceModeID, KillerLocation, VictimLocation, PetLocation, bPetKill);
	};
};
