#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmBot__LoadBotSpawnTableMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*, int),
	0x109437b0,
	CAmBot__LoadBotSpawnTableMarshal> {
public:
	static bool bPopulateDatabase;
	static void __fastcall Call(void* param_1, void* edx, void* param_2, int param_3);
	static inline void __fastcall CallOriginal(void* param_1, void* edx, void* param_2, int param_3) {
		m_original(param_1, edx, param_2, param_3);
	};
};

