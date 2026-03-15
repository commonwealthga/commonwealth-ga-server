#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/Utils/Logger/Logger.hpp"


uint8_t __fastcall CGameClient__MarshalReceived::Call(void* GameClient, void* edx, void* InBunch) {

	LogCallBegin();

	short uVar1 = *(short*)((int)InBunch + 0x26);
	Logger::Log(GetLogChannel(), "Received packet type [0x%04X] (suppressed on server)\n", uVar1);

	if (uVar1 == GA_U::QUEST_UPDATE_REQUEST) {
		uint32_t nQuestId = 0;

		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::QUEST_ID, &nQuestId);

		unsigned char bAbandonFlag = 0;
		CMarshal__GetFlag::CallOriginal(InBunch, edx, GA_T::ABANDON_FLAG, &bAbandonFlag);

		QuestAction action = bAbandonFlag ? QuestAction::Abandon : QuestAction::Request;

		// GCurrentMarshalConnection is the UNetConnection set by MarshalChannel__MarshalReceived
		// before dispatching here — one per player, proper multi-player identity.
		std::string session_guid;
		auto it = GClientConnectionsData.find((int32_t)(intptr_t)GCurrentMarshalConnection);
		if (it != GClientConnectionsData.end()) {
			session_guid = it->second.SessionGuid;
		}

		Logger::Log(GetLogChannel(), "[QUEST_UPDATE_REQUEST] questId=%u abandon=%d session_guid=%s\n",
			nQuestId, (int)bAbandonFlag, session_guid.c_str());

		if (!session_guid.empty()) {
			GQuestEvents[session_guid].push_back({ (int)nQuestId, action });
		}
	}

	LogCallEnd();

	return 0;
}
