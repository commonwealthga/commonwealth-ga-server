#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"

std::map<uint32_t, int> CAmBot__LoadBotBehaviorMarshal::m_BotBehaviorPointers;

void __fastcall CAmBot__LoadBotBehaviorMarshal::Call(void* CAmBotBehaviorRow, void* edx, void* Marshal) {
	CallOriginal(CAmBotBehaviorRow, edx, Marshal);

	void** tmp = (void**)Marshal;
	m_BotBehaviorPointers[CMarshal__GetInt32t::m_values[GA_T::GA_T::BEHAVIOR_ID]] = (int)(*tmp);

	Logger::Log("marshalbotbehavior", "%d: 0x%p\n", CMarshal__GetInt32t::m_values[GA_T::GA_T::BEHAVIOR_ID], Marshal);

	// m_BotPointers[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = Marshal;
	// m_BotBehaviorIds[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID];

	// uint32_t* botId;
	// CMarshal__GetByte::CallOriginal(Marshal, edx, GA_T::GA_T::BOT_ID, botId);
	//
	// Logger::Log("marshalbot.txt", "%d\n", *botId);
}

