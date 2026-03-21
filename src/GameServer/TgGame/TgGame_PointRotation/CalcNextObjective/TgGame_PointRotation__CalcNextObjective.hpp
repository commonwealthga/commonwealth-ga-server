#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_PointRotation__CalcNextObjective : public HookBase<
	void(__fastcall*)(ATgGame_PointRotation*, void*),
	0x10ad9e10,
	TgGame_PointRotation__CalcNextObjective> {
public:
	static void __fastcall Call(ATgGame_PointRotation* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_PointRotation* Game, void* edx) {
		m_original(Game, edx);
	};
};


