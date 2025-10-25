#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmBot__LoadBotMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094c730,
	CAmBot__LoadBotMarshal> {
public:
	static bool bPopulateDatabaseBots;
	static bool bPopulateDatabaseBotDevices;
	static std::map<uint32_t, int> m_BotPointers;
	static std::map<uint32_t, uint32_t> m_BotBehaviorIds;
	static void __fastcall Call(void* CAmBotRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmBotRow, void* edx, void* Marshal) {
		m_original(CAmBotRow, edx, Marshal);
	};
};

