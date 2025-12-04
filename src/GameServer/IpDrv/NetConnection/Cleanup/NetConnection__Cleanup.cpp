#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"

void __fastcall NetConnection__Cleanup::Call(UNetConnection* Connection) {
	LogCallBegin();

	int32_t ConnectionId = (int32_t)Connection;
	GClientConnectionsData.erase(ConnectionId);

	// NetConnection__CleanupActor::Call(Connection);
	//
	// UUdpNetDriver* Driver = *(UUdpNetDriver**)((char*)Connection + 0x70);
	// if (Driver) {
	// 	Driver->ClientConnections.Clear();
	// 	// get keys of std::map GClientConnectionsData
	// 	for (auto it = GClientConnectionsData.begin(); it != GClientConnectionsData.end(); ++it) {
	// 		if (it->second.bClosed) {
	// 			continue;
	// 		}
	// 		Driver->ClientConnections.Add((UNetConnection*)it->first);
	// 	}
	// }

	FMallocWindows__Free::bLogEnabled = true;
	CallOriginal(Connection);
	FMallocWindows__Free::bLogEnabled = false;

	LogCallEnd();
}

