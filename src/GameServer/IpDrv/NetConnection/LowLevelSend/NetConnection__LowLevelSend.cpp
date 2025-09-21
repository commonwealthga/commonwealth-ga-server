#include "src/pch.hpp"

#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/Storage/ClientConnectionsData.hpp"

void NetConnection__LowLevelSend::Call(UNetConnection* Connection, void* edx, void* Buffer, int Size) {
	int32_t ClientConnectionIndex = (int32_t)Connection;

	if (GClientConnectionsData.find(ClientConnectionIndex) != GClientConnectionsData.end()) {
		ClientConnectionData ConnectionData = GClientConnectionsData[ClientConnectionIndex];
		void* SocketInstance = ConnectionData.SocketInstance;
		SOCKET Socket = *(SOCKET*)((char*)SocketInstance + 0x08);
		sockaddr_in ClientRemoteAddr = ConnectionData.RemoteAddr;

		int bytesSent = sendto(Socket, (const char*)Buffer, Size, 0, (sockaddr*)&ClientRemoteAddr, sizeof(ClientRemoteAddr));
		// LogToFile("C:\\lowlevelsend.txt", "[UNetConnection::LowLevelSend] Bytes sent: %d", bytesSent);
	}
}
