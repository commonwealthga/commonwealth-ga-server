#include "src/GameServer/TgGame/TgPlayerController/ServerChangeTaskForce/TgPlayerController__ServerChangeTaskForce.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"

void __fastcall TgPlayerController__ServerChangeTaskForce::Call(ATgPlayerController* Controller, void* edx, unsigned char nTaskForce) {
	LogCallBegin();
	const int target_task_force = (nTaskForce == 1) ? 1 : (nTaskForce == 2 ? 2 : 0);
	if (!Controller || !Controller->Pawn || target_task_force == 0) {
		Logger::Log("team", "[ServerChangeTaskForce] invalid request controller=%p pawn=%p tf=%u; using original\n",
			Controller, Controller ? Controller->Pawn : nullptr, (unsigned)nTaskForce);
		CallOriginal(Controller, edx, nTaskForce);
		LogCallEnd();
		return;
	}

	TgPawn__SetTaskForceNumber::Call((ATgPawn*)Controller->Pawn, nullptr, target_task_force);

	const int32_t connection_index = (int32_t)Controller->Player;
	auto conn_it = GClientConnectionsData.find(connection_index);
	if (conn_it != GClientConnectionsData.end()) {
		ClientConnectionData& conn = conn_it->second;
		conn.PlayerInfo.task_force = target_task_force;
		if (conn.pPlayerInfo) {
			conn.pPlayerInfo->task_force = target_task_force;
		}

		if (!conn.SessionGuid.empty()) {
			nlohmann::json joined;
			joined["type"]         = IpcProtocol::MSG_PLAYER_JOINED;
			joined["instance_id"]  = IpcClient::GetInstanceId();
			joined["session_guid"] = conn.SessionGuid;
			joined["task_force"]   = target_task_force;
			IpcClient::Send(joined.dump());
		}
		Logger::Log("team", "[ServerChangeTaskForce] guid=%s tf=%d\n",
			conn.SessionGuid.c_str(), target_task_force);
	} else {
		Logger::Log("team", "[ServerChangeTaskForce] no connection entry for controller=%p index=%d tf=%d\n",
			Controller, connection_index, target_task_force);
	}
	LogCallEnd();
}
