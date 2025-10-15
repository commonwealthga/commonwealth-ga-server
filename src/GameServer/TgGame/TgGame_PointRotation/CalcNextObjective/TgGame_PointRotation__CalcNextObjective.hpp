#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_PointRotation__CalcNextObjective : public HookBase<
	void(__fastcall*)(ATgGame_PointRotation*, void*, int),
	0x10ad9bf0,
	TgGame_PointRotation__CalcNextObjective> {
public:
	static void __fastcall Call(ATgGame_PointRotation* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_PointRotation* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


