#include "src/pch.hpp"

#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

void NetConnection__LowLevelSend::Call(UNetConnection* Connection, void* edx, void* Buffer, int Size) {
	// Called per outbound UDP packet. Keep it tight — no heap allocations,
	// no by-value struct copies. The previous version did
	// `ClientConnectionData ConnectionData = GClientConnectionsData[...]`,
	// which copied the entire struct (FURL, multiple std::string members,
	// PlayerInfo's strings + skills vector) and forced ~10 heap alloc/free
	// pairs per packet. Under Wine that adds up fast; combined with O(N)
	// scans on the inbound side it's the leading suspect for replication
	// jitter (manifested as client teleporting on the player side).
	auto it = GClientConnectionsData.find((int32_t)Connection);
	if (it == GClientConnectionsData.end()) return;
	if (it->second.bClosed) return;

	SOCKET Socket = *(SOCKET*)((char*)it->second.SocketInstance + 0x08);
	sendto(Socket, (const char*)Buffer, Size, 0,
	       (sockaddr*)&it->second.RemoteAddr, sizeof(sockaddr_in));
}
