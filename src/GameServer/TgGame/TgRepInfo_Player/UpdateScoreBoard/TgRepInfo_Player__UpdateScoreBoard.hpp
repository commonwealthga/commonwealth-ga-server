#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgRepInfo_Player__UpdateScoreBoard : public HookBase<
	void(__fastcall*)(ATgRepInfo_Player*, void*),
	0x109ee510,
	TgRepInfo_Player__UpdateScoreBoard> {
public:
	static void __fastcall Call(ATgRepInfo_Player* RepInfo, void* edx);
	static inline void __fastcall CallOriginal(ATgRepInfo_Player* RepInfo, void* edx) {
		m_original(RepInfo, edx);
	};
};
