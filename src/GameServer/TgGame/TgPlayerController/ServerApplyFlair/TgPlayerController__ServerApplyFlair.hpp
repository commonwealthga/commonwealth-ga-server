#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerApplyFlair : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, FString*),
	0x10963ab0,
	TgPlayerController__ServerApplyFlair> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, FString* flairName);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, FString* flairName) {
		m_original(Controller, edx, flairName);
	};
};
