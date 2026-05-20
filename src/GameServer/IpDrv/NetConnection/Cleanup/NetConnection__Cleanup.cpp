#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/RecentlyClosedAddrs/RecentlyClosedAddrs.hpp"
#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall NetConnection__Cleanup::Call(UNetConnection* Connection) {
	LogCallBegin();

	int32_t ConnectionId = (int32_t)Connection;

	// Capture session_guid, pawn, and source addr before erasing connection data
	std::string session_guid;
	ATgPawn_Character* pawn = nullptr;
	uint32_t remote_ip_be   = 0;
	uint16_t remote_port_be = 0;
	{
		auto it = GClientConnectionsData.find(ConnectionId);
		if (it != GClientConnectionsData.end()) {
			session_guid    = it->second.SessionGuid;
			pawn            = it->second.Pawn;
			remote_ip_be    = it->second.RemoteAddr.sin_addr.s_addr;
			remote_port_be  = it->second.RemoteAddr.sin_port;
		} else {
			// Already cleaned up (caller-side erased by hand, or this is a
			// second reap pass on a connection we've already torn down). Skip
			// the engine cleanup too to avoid hitting
			// `Driver->ClientConnections.RemoveItem(this) == 1` assert.
			Logger::Log(GetLogChannel(),
				"NetConnection__Cleanup: connection 0x%p already gone; skipping\n",
				Connection);
			LogCallEnd();
			return;
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

	// Drop the reverse pawn->session mapping. NonPersistAddDevice /
	// NonPersistRemoveDevice walk GPawnSessions to route IPC events, and the
	// pawn pointer is about to be freed by the engine's CleanUpActor below —
	// leaving a dangling key would cause those routes to hit free'd memory
	// on the next inventory change.
	if (pawn) {
		GPawnSessions.erase((ATgPawn*)pawn);
	}

	GClientConnectionsData.erase(ConnectionId);
	if (remote_ip_be != 0 || remote_port_be != 0) {
		GConnectionByAddr.erase(MakeRemoteAddrKey(remote_ip_be, remote_port_be));
	}

	// Block late in-flight UDP packets from this source addr from being
	// accepted as a brand-new connection in UdpNetDriver::TickDispatch. Cap is
	// short — a fraction of a second covers re-ordered ACK / FIN-equivalent
	// stragglers without locking out a legitimate reconnect from the same
	// IP:port (which usually takes seconds anyway, since the client tears down
	// its socket first).
	if (remote_ip_be != 0 || remote_port_be != 0) {
		RecentlyClosedAddrs::Mark(remote_ip_be, remote_port_be);
	}

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
