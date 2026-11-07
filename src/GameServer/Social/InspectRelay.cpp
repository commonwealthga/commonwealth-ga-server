#include "src/GameServer/Social/InspectRelay.hpp"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace InspectRelay {

bool Handle(short opcode, void* InBunch, void* edx) {
	if (opcode != GA_U::SEND_CHARACTER_INVENTORY && opcode != GA_U::SEND_CHARACTER_CRAFTING_SKILLS)
		return false;
	uint32_t src = 0;
	CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::SOURCE_CHARACTER_ID, &src);
	if (src == 0) return false;   // own-agent request: not an inspect

	std::string session_guid;
	int64_t nCharacterId = 0;
	auto it = GClientConnectionsData.find((int32_t)(intptr_t)GCurrentMarshalConnection);
	if (it != GClientConnectionsData.end()) {
		session_guid = it->second.SessionGuid;
		if (it->second.pPlayerInfo) nCharacterId = it->second.pPlayerInfo->selected_character_id;
	}
	Logger::Log("social", "[Inspect] request 0x%04X from char=%lld target=%u guid=%s\n",
		(unsigned)(uint16_t)opcode, (long long)nCharacterId, src, session_guid.c_str());
	if (session_guid.empty() || nCharacterId == 0) return true;

	nlohmann::json ev;
	ev["type"]                = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]             = "inspect_request";
	ev["instance_id"]         = IpcClient::GetInstanceId();
	ev["session_guid"]        = session_guid;
	ev["character_id"]        = nCharacterId;
	ev["source_character_id"] = (int64_t)src;
	ev["packet"]              = (int)(uint16_t)opcode;
	IpcClient::Send(ev.dump());
	return true;
}

}  // namespace InspectRelay
