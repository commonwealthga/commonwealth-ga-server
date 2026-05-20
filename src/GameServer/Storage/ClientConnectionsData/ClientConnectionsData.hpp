#pragma once

#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/pch.hpp"

#include <unordered_map>

struct ClientConnectionData {
	sockaddr_in RemoteAddr;
	void* SocketInstance;
	FURL url;
	char RemoteAddrString[32];
	FString* RemoteAddrFString;
	ATgPawn_Character* Pawn;
	bool bClosed;
	std::string SessionGuid;
	PlayerInfo PlayerInfo;
	struct PlayerInfo* pPlayerInfo = nullptr; // stable pointer into PlayerRegistry::by_guid_
};

extern std::map <int32_t, ClientConnectionData> GClientConnectionsData;

// Address → UNetConnection* side map, kept in sync with GClientConnectionsData
// inserts/erases. Sidesteps the per-packet O(N) linear scan + O(log N) per-iter
// std::map lookup that UdpNetDriver::TickDispatch would otherwise need to find
// the matching connection for an incoming packet's source addr. UE3 stock does
// it inline because UTcpipConnection carries `RemoteAddr` at a known offset on
// the connection itself; we never located that offset on the binary's
// connection class, so we keep the addr externally and key on it directly.
//
// Key layout: (sin_addr.s_addr in network byte order) << 16 | sin_port (NBO).
// Both halves stay in network byte order — no htonl/ntohs needed, the bits are
// the same on either side of the comparison.
inline uint64_t MakeRemoteAddrKey(uint32_t sin_addr_nbo, uint16_t sin_port_nbo) {
	return ((uint64_t)sin_addr_nbo << 16) | (uint64_t)sin_port_nbo;
}

extern std::unordered_map<uint64_t, UNetConnection*> GConnectionByAddr;

