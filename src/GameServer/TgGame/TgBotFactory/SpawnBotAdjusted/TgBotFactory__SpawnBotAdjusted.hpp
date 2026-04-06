#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__SpawnBotAdjusted : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, int, float),
	0x10a8c310,
	TgBotFactory__SpawnBotAdjusted> {
public:
	static void __fastcall Call(ATgBotFactory* Factory, void* edx, int nBotId, float fAdjustment);
	static inline void __fastcall CallOriginal(ATgBotFactory* Factory, void* edx, int nBotId, float fAdjustment) {
		m_original(Factory, edx, nBotId, fAdjustment);
	};
};
