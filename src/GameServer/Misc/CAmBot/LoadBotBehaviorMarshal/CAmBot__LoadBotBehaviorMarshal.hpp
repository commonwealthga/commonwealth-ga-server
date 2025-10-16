#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmBot__LoadBotBehaviorMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094ef80,
	CAmBot__LoadBotBehaviorMarshal> {
public:
	static std::map<uint32_t, int> m_BotBehaviorPointers;
	static void __fastcall Call(void* CAmBotBehaviorRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmBotBehaviorRow, void* edx, void* Marshal) {
		m_original(CAmBotBehaviorRow, edx, Marshal);
	};
};

