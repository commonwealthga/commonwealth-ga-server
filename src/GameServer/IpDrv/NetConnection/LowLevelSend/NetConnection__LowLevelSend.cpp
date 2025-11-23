#include "src/pch.hpp"

#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

void NetConnection__LowLevelSend::Call(UNetConnection* Connection, void* edx, void* Buffer, int Size) {
	// Logger::Log("debug", "MINE NetConnection__LowLevelSend START\n");
	int32_t ClientConnectionIndex = (int32_t)Connection;

	if (GClientConnectionsData.find(ClientConnectionIndex) != GClientConnectionsData.end()) {
		ClientConnectionData ConnectionData = GClientConnectionsData[ClientConnectionIndex];
		void* SocketInstance = ConnectionData.SocketInstance;
		SOCKET Socket = *(SOCKET*)((char*)SocketInstance + 0x08);
		sockaddr_in ClientRemoteAddr = ConnectionData.RemoteAddr;

		// log contents of the buffer
		// Logger::Log("wtf", "\n\n[UNetConnection::LowLevelSend] Buffer contents:\n");
		//
		// for (int i = 0; i < Size; i++) {
		// 	Logger::Log("wtf", "%02X ", ((unsigned char*)Buffer)[i]);
		// }

		int bytesSent = sendto(Socket, (const char*)Buffer, Size, 0, (sockaddr*)&ClientRemoteAddr, sizeof(ClientRemoteAddr));
		// LogToFile("C:\\lowlevelsend.txt", "[UNetConnection::LowLevelSend] Bytes sent: %d", bytesSent);
	}
	// Logger::Log("debug", "MINE NetConnection__LowLevelSend END\n");
}
