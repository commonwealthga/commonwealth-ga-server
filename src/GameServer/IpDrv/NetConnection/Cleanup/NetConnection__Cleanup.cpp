#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"

void __fastcall NetConnection__Cleanup::Call(UNetConnection* Connection) {
	LogCallBegin();

	int32_t ConnectionId = (int32_t)Connection;

	// Capture session_guid before erasing connection data
	std::string session_guid;
	{
		auto it = GClientConnectionsData.find(ConnectionId);
		if (it != GClientConnectionsData.end()) {
			session_guid = it->second.SessionGuid;
		}
	}

	// Send PLAYER_LEFT if we had a valid session
	if (!session_guid.empty()) {
		nlohmann::json left;
		left["type"]         = IpcProtocol::MSG_PLAYER_LEFT;
		left["instance_id"]  = IpcClient::GetInstanceId();
		left["session_guid"] = session_guid;
		IpcClient::Send(left.dump());

		PlayerRegistry::Unregister(session_guid);
	}

	GClientConnectionsData.erase(ConnectionId);

	// Check if instance is now empty
	if (GClientConnectionsData.empty()) {
		nlohmann::json empty;
		empty["type"]        = IpcProtocol::MSG_INSTANCE_EMPTY;
		empty["instance_id"] = IpcClient::GetInstanceId();
		IpcClient::Send(empty.dump());
	}

	FMallocWindows__Free::bLogEnabled = true;
	CallOriginal(Connection);
	FMallocWindows__Free::bLogEnabled = false;

	LogCallEnd();
}

