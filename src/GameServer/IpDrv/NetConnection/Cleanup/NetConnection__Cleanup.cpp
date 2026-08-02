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
#include "src/GameServer/Storage/ActiveSpectatorCount/ActiveSpectatorCount.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Stats/SpectatorOverlayFeed/SpectatorOverlayFeed.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/Markers/Markers.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/FxBrowse/FxBrowse.hpp"
#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall NetConnection__Cleanup::Call(UNetConnection* Connection) {
	LogCallBegin();

	int32_t ConnectionId = (int32_t)Connection;

	// Capture session_guid, pawn, and source addr before erasing connection data
	std::string session_guid;
	ATgPawn_Character* pawn = nullptr;
	uint32_t remote_ip_be   = 0;
	uint16_t remote_port_be = 0;
	uint64_t register_generation = 0;
	bool was_spectator = false;
	{
		auto it = GClientConnectionsData.find(ConnectionId);
		if (it != GClientConnectionsData.end()) {
			session_guid    = it->second.SessionGuid;
			pawn            = it->second.Pawn;
			remote_ip_be    = it->second.RemoteAddr.sin_addr.s_addr;
			remote_port_be  = it->second.RemoteAddr.sin_port;
			register_generation = it->second.PlayerInfo.register_generation;
			was_spectator   = it->second.PlayerInfo.is_spectator;
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

		// Match stats: bank + upsert + LEAVE before the pawn is torn down.
		{
			auto pinfo = PlayerRegistry::GetByGuid(session_guid);
			MatchStats::OnPlayerLeft((ATgPawn*)pawn,
				pinfo ? pinfo->user_id : 0);
		}

		bool has_other_connection = false;
		for (const auto& kv : GClientConnectionsData) {
			if (kv.first != ConnectionId && kv.second.SessionGuid == session_guid) {
				has_other_connection = true;
				break;
			}
		}
		if (has_other_connection) {
			Logger::Log(GetLogChannel(),
				"NetConnection__Cleanup: keeping registry for guid=%s; another connection is active\n",
				session_guid.c_str());
		} else if (register_generation == 0) {
			Logger::Log(GetLogChannel(),
				"NetConnection__Cleanup: keeping registry for guid=%s; connection never bound PLAYER_REGISTER\n",
				session_guid.c_str());
		} else {
			PlayerRegistry::UnregisterIfGeneration(session_guid, register_generation);
		}
	}

	// Drop the reverse pawn->session mapping. NonPersistAddDevice /
	// NonPersistRemoveDevice walk GPawnSessions to route IPC events, and the
	// pawn pointer is about to be freed by the engine's CleanUpActor below —
	// leaving a dangling key would cause those routes to hit free'd memory
	// on the next inventory change.
	if (pawn) {
		// Leaving the mission takes your live deployables/pets with you —
		// otherwise a player parks a power/med station in a PvE boss room,
		// disconnects or swaps character, and the station keeps working for
		// the team. Same teardown team-change and profile-switch already do.
		if (!pawn->bDeleteMe) {
			TgPawn__KillDeployables::KillAllOwned((ATgPawn*)pawn);
		}
		GPawnSessions.erase((ATgPawn*)pawn);
		// Same reasoning: drop this pawn's SpectatorOverlayFeed rate-limit
		// entry now rather than leaving it to accumulate for the life of the
		// process (see SpectatorOverlayFeed.cpp's g_lastPushMs comment).
		SpectatorOverlayFeed::ForgetPawn((int)pawn->r_nPawnId);
	}

	// Same reasoning again: a session that leaves with -markers still on would
	// otherwise keep the per-frame sweep armed for the life of the instance.
	if (!session_guid.empty()) {
		TgPlayerActions::MarkersCmd::ForgetSession(session_guid);
		TgPlayerActions::FxBrowseCmd::ForgetSession(session_guid);
	}

	// Matches the increment in the TgGamePostLogin ProcessEvent case. Clamped
	// at 0 as defensive insurance -- an errant double-decrement should never
	// be able to wedge the counter negative and permanently disable the
	// (already-gated-off) SpectatorOverlayFeed check for the rest of this
	// instance's lifetime.
	if (was_spectator && GActiveSpectatorCount > 0) {
		--GActiveSpectatorCount;
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
