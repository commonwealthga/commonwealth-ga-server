#include "src/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/NetDriver/TickDispatch/NetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/IpDrv//NetConnection/ConstructClientConnection/NetConnection__ConstructClientConnection.hpp"
#include "src/GameServer/Engine/PackageMapLevel/Create/PackageMapLevel__Create.hpp"
#include "src/GameServer/Engine/PackageMapLevel/Initialize/PackageMapLevel__Initialize.hpp"
#include "src/GameServer/IpDrv/NetConnection/InitOut/NetConnection__InitOut.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.hpp"
#include "src/GameServer/Engine/World/NotifyAcceptedConnection/World__NotifyAcceptedConnection.hpp"
#include "src/GameServer/IpDrv/NetConnection/ReceivedRawPacket/NetConnection__ReceivedRawPacket.hpp"
#include "src/Utils/Macros.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

UClass* UdpNetDriver__TickDispatch::NetConnectionClass = nullptr;
bool UdpNetDriver__TickDispatch::bNetConnectionVTableHooked = false;


void __fastcall UdpNetDriver__TickDispatch::Call(UUdpNetDriver* NetDriver, void* edx, float DeltaTime) {

	// Logger::Log("debug", "MINE UdpNetDriver__TickDispatch START\n");
	NetDriver__TickDispatch::CallOriginal(NetDriver, edx, DeltaTime);

	TARRAY_INIT(NetDriver, ClientConnections, UNetConnection*, 0x44, 128); // todo: move this elsewhere for performance

	void* socketInstance = *(void**)((char*)NetDriver + 0x14C);
	SOCKET Socket = *(SOCKET*)((char*)socketInstance + 0x08);

	sockaddr_in from;
	int fromLen = sizeof(from);

	char buffer[4096];

	while (socketInstance != nullptr) {
		int bytesRead = 0;

		bytesRead = recvfrom(Socket, buffer, sizeof(buffer), 0, (sockaddr*)&from, &fromLen);
		bool bRecvFromOk = bytesRead >= 0;

		if (bytesRead > 0) {
			// LogToFile("C:\\tickdispatch.txt", "[UdpNetDriver::TickDispatch] Received %d bytes from %s", bytesRead, frombuf);
			// this is ok
		} else {
			if (bytesRead == SOCKET_ERROR) {
				int err = WSAGetLastError();
				if (err == 10035) { // WSAEWOULDBLOCK
					break;
				}
				// if (err == SE_NO_ERROR) {
				// 	break;
				// }
				// LogToFile("C:\\tickdispatch.txt", "[UdpNetDriver::TickDispatch] Socket error: %d", WSAGetLastError());
			}
			break;
		}

		UNetConnection* Connection = nullptr;

		if (NetDriver->ClientConnections.Num() > 0) {
			for (int i = NetDriver->ClientConnections.Num() - 1; i >= 0; i--) {
				UNetConnection* ClientConnection = NetDriver->ClientConnections.Data[i];
				int32_t ClientConnectionIndex = (int32_t)ClientConnection;

				if (GClientConnectionsData.find(ClientConnectionIndex) != GClientConnectionsData.end()) {
					ClientConnectionData ConnectionData = GClientConnectionsData[ClientConnectionIndex];
					sockaddr_in ClientRemoteAddr = ConnectionData.RemoteAddr;

					if (ClientRemoteAddr.sin_addr.s_addr == from.sin_addr.s_addr && ClientRemoteAddr.sin_port == from.sin_port) {
						Connection = ClientConnection;
						// LogToFile("C:\\tickdispatch.txt", "[UdpNetDriver::TickDispatch] Found existing client connection");
					}
				}
			}
		}

		if (!bRecvFromOk) {
			if (Connection) {
				// int* ClientConnectionsCountPtr = (int*)((char*)NetDriver + 0x48);
				uint32_t ClientConnectionState = *(int*)((char*)Connection + 0x74);
				if (ClientConnectionState != 3) { // != USOCK_Open
					NetConnection__Cleanup::CallOriginal(Connection);
				}
			}
		}

		if (!Connection) {

			// LogToFile("C:\\tickdispatch.txt", "[UdpNetDriver::TickDispatch] Client connection not found, creating new one");
			// FName ObjName = MakeUniqueConnectionFName();


			// LogToFile("C:\\mylog.txt", "[UdpNetDriver::TickDispatch] flags: %d", CachedNetConnectionFlagsB);

			if (!UdpNetDriver__TickDispatch::NetConnectionClass) {
				for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
					if (UObject::GObjObjects()->Data[i] && strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "Class Engine.NetConnection") == 0) {
						UdpNetDriver__TickDispatch::NetConnectionClass = (UClass*)UObject::GObjObjects()->Data[i];
						UdpNetDriver__TickDispatch::NetConnectionClass->ClassFlags &= ~0x00000001; // remove CLASS_Abstract flag

						// patch the global class pointer to trick the engine into accepting our class
						void** ClientConnectionClass = Globals::Get().ClientConnectionClass;
						*ClientConnectionClass = UdpNetDriver__TickDispatch::NetConnectionClass;

						break;
					}
				}
			}

			Connection = (UNetConnection*)NetConnection__ConstructClientConnection::CallOriginal(
				UdpNetDriver__TickDispatch::NetConnectionClass,
				-1, 0, 0, 0, 0, 0, 0, nullptr);

			ClientConnectionData ConnectionData;
			ConnectionData.RemoteAddr = from;
			ConnectionData.SocketInstance = socketInstance;
			ConnectionData.url = FURL();

			sprintf_s(ConnectionData.RemoteAddrString, sizeof(ConnectionData.RemoteAddrString), "%s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));

			Logger::Log("debug", "New client connection: %s\n", ConnectionData.RemoteAddrString);

			/* /- UNetConnection::InitConnection START */

			// *(FURL*)((char*)Connection + 0x6C) = url; // set Connection->URL
			void* Something = (void*)((char*)Connection + 0x70);
			*(uint32_t*)((char*)Connection + 0x74) = 3; // set Connection->State
			*(uint32_t*)((char*)Connection + 0xCC) = 512; // set Connection->MaxPacket = 512 (apparently that's what's in the disassembly)
			*(uint32_t*)((char*)Connection + 0xD0) = 0; // set Connection->Overhead = 0 (apparently that's what's in the disassembly)

			*(uint32_t*)((char*)Connection + 0x4F94) = *(uint32_t*)((char*)Something + 0x134);
			*(uint32_t*)((char*)Connection + 0x4F98) = *(uint32_t*)((char*)Something + 0x138);
			*(uint32_t*)((char*)Connection + 0x4F9C) = *(uint32_t*)((char*)Something + 0x13C);
			*(uint32_t*)((char*)Connection + 0x4FA0) = *(uint32_t*)((char*)Something + 0x140);
			*(uint32_t*)((char*)Connection + 0x4FA4) = *(uint32_t*)((char*)Something + 0x144);
			
			*(void**)((char*)Connection + 0x70) = (void*)NetDriver; // set Connection->Driver:

			Connection->CurrentNetSpeed = 2600;

			UObject* PackageMap = (UObject*)PackageMapLevel__Create::CallOriginal(0xC4, Connection, 0, 0, 0, 0);
			PackageMapLevel__Initialize::CallOriginal(PackageMap);

			*(void**)((char*)PackageMap + 0xC0) = Connection; // set PackageMap->Connection
			//
			*(void**)((char*)PackageMap) = Globals::Get().PackageMapVtable;

			*(void**)((char*)Connection + 0xBC) = PackageMap; // set Connection->PackageMap

			NetConnection__InitOut::CallOriginal(Connection);

			GClientConnectionsData.insert(std::pair<int32_t, ClientConnectionData>((int32_t)Connection, ConnectionData));
			/* \- UNetConnection::InitConnection END */

			void* Notify = *(void**)((char*)NetDriver + 0x54);

			*(void**)((char*)Notify + 0x98) = (void*)NetDriver; // set Notify->NetDriver

			if (!UdpNetDriver__TickDispatch::bNetConnectionVTableHooked) {
				// LowLevelGetRemoteAddress is a virtual method without implementation, we need to work around that

				UdpNetDriver__TickDispatch::bNetConnectionVTableHooked = true;

				void** vtable = *(void***)Connection;
				DWORD oldProtect;
				VirtualProtect(&vtable[0x120 / 4], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
				vtable[0x120 / 4] = (void*)&NetConnection__LowLevelGetRemoteAddress::Call;
				VirtualProtect(&vtable[0x120 / 4], sizeof(void*), oldProtect, &oldProtect);
			}

			Logger::Log("debug", "about to call notify accepted connection\n");

			// World__NotifyAcceptedConnection::CallOriginal(Notify, edx, Connection);

			// typedef void (*tNotifyAcceptedConnectionRaw)();  // no args, we push manually
			// tNotifyAcceptedConnectionRaw pOriginalWorldNotifyAcceptedConnection = (tNotifyAcceptedConnectionRaw)0x10BEBFD0;
			//
			// asm volatile(
			// 	"push %[conn] \n\t"
			// 	"mov  %[notify], %%ecx \n\t"   // FIXED: move notify into ecx
			// 	"call *%[func] \n\t"
			// 	:
			// 	: [func] "r" (pOriginalWorldNotifyAcceptedConnection),
			// 	[notify] "r" (Notify),
			// 	[conn] "r" (Connection)
			// 	: "ecx", "memory"
			// );

			Logger::Log("debug", "notify accepted connection called\n");

			TARRAY_ADD(ClientConnections, Connection); 
		}

		if (Connection) {
			// LogToFile("C:\\tickdispatch.txt", "[UdpNetDriver::TickDispatch] processing %d bytes:\n", bytesRead);
			// for (int i = 0; i < bytesRead; i++) {
			// 	LogToFileInline("C:\\tickdispatch.txt", "%02X", (uint8_t)buffer[i]);
			// }

			// DumpNetDriver("C:\\netdriverpostcreateclientconnection.txt", NetDriver);

			NetConnection__ReceivedRawPacket::CallOriginal(Connection, nullptr, &buffer, bytesRead);
		}
	}
	// \-- reimplement UTcpNetDriver::TickDispatch END
	// Logger::Log("debug", "MINE UdpNetDriver__TickDispatch END\n");

}
