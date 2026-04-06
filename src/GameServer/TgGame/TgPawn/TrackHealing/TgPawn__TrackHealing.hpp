#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackHealing : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float, float, int),
	0x109c0890,
	TgPawn__TrackHealing> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth) {
		m_original(Pawn, edx, nDeviceModeID, fDamage, fMissingHealth, nMaxHealth);
	};
};
