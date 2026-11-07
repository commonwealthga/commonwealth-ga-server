#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnRequestMission : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, UTgSeqAct_RequestMission*),
	0x109c0d20,
	TgPawn__ServerOnRequestMission> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, UTgSeqAct_RequestMission* Action);
	static bool SetCameraFade(AController* Controller, bool bOn, float Alpha, float Time);
	static void Tick(float DeltaSeconds);
	static bool HoldBlack(AController* Controller);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, UTgSeqAct_RequestMission* Action) {
		m_original(Pawn, edx, Action);
	};
};
