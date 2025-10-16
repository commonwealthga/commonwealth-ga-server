#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"

std::map<uint32_t, int> CAmBot__LoadBotMarshal::m_BotPointers;
std::map<uint32_t, uint32_t> CAmBot__LoadBotMarshal::m_BotBehaviorIds;

void __fastcall CAmBot__LoadBotMarshal::Call(void* CAmBotRow, void* edx, void* Marshal) {
	CallOriginal(CAmBotRow, edx, Marshal);

	// void** tmp = (void**)Marshal;

	m_BotPointers[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = (int)(CAmBotRow);
	m_BotBehaviorIds[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID];

	// Logger::Log("marshalbot_data", "\n\n%d: 0x%p\n", CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID], Marshal);

	// Logger::DumpMemory("marshalbot_data", CAmBotRow, 0x300, 0);

	// uint32_t* botId;
	// CMarshal__GetByte::CallOriginal(Marshal, edx, GA_T::GA_T::BOT_ID, botId);
	//
	// Logger::Log("marshalbot", "%d\n", *botId);
}

