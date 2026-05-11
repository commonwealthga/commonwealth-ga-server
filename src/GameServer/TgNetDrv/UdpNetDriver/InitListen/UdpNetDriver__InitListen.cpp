#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/IpDrv/SocketWin/CreateDGramSocket/SocketWin__CreateDGramSocket.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

int UdpNetDriver__InitListen::Call(UUdpNetDriver* NetDriver, void* edx, void* Notify, FURL* Url, FString* Error) {
	Logger::Log("debug", "MINE UdpNetDriver__InitListen START\n");

	// pServerUrl = Url;

	int result = UdpNetDriver__InitListen::CallOriginal(NetDriver, Notify, Url, Error);
	if (!result) {
		// Log("Super::InitListen failed");
		return 0;
	}

	// UE3 stock defaults are 15000 B/s per client (LAN-era values). UE3
	// clamps each connection's CurrentNetSpeed to NetDriver->MaxClientRate
	// (see UnConn.cpp), so the 26 Mbps we write to CurrentNetSpeed in
	// UdpNetDriver__TickDispatch is dead code at the stock cap. With many
	// always-relevant deployables/DRIs/projectiles replicating, the per-tick
	// budget at 15 KB/s is easily exceeded; UE3 defers bunches to the next
	// 30 Hz tick, queue grows, ping balloons (fine on LAN where RTT is ~0,
	// catastrophic across a 30-50 ms internet link). 50000 is well within
	// modern home upload and what UDK Internet defaults landed on.
	NetDriver->MaxClientRate         = 50000;
	NetDriver->MaxInternetClientRate = 50000;

	void* SocketInstance = nullptr;
	*(void**)((char*)NetDriver + 0x14C) = SocketInstance = SocketWin__CreateDGramSocket::CallOriginal();

	if (SocketInstance == nullptr) {
		// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] CreateDGramSocket failed: %d", WSAGetLastError());
		return 0;
	}

	SOCKET Socket = *(SOCKET*)((char*)SocketInstance + 0x08);

	bool bAllowBroadcast = true;
	int setBroadcastResult = setsockopt(Socket,SOL_SOCKET,SO_BROADCAST,(char*)&bAllowBroadcast,sizeof(bool));
	if (setBroadcastResult != 0) {
		// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] setsockopt SO_BROADCAST failed: %d", WSAGetLastError());
		return 0;
	}

	bool bAllowReuse = true;
	int setReuseResult = setsockopt(Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&bAllowReuse,sizeof(bool));
	if (setReuseResult != 0) {
		// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] setsockopt SO_REUSEADDR failed: %d", WSAGetLastError());
		return 0;
	}

	int RecvSize = 0x20000000;
	int SendSize = 0x20000000;
	int SizeSize = sizeof(int);

	bool bSetRecvSizeOk = setsockopt(Socket,SOL_SOCKET,SO_RCVBUF,(char*)&RecvSize,sizeof(int)) == 0;
	getsockopt(Socket,SOL_SOCKET,SO_RCVBUF,(char*)&RecvSize, &SizeSize);

	bool bSetSendSizeOk = setsockopt(Socket,SOL_SOCKET,SO_SNDBUF,(char*)&SendSize,sizeof(int)) == 0;
	getsockopt(Socket,SOL_SOCKET,SO_SNDBUF,(char*)&SendSize, &SizeSize);

	// debugf( NAME_Init, TEXT("%s: Socket queue %i / %i"), SOCKET_API, RecvSize, SendSize );
	// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] Socket queue %i / %i", RecvSize, SendSize);

	sockaddr_in addr = *(sockaddr_in*)((char*)NetDriver + 0x148);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Or inet_addr("0.0.0.0")
    addr.sin_port = htons(Config::GetPort());

	int bindResult = bind(Socket, (sockaddr*)&addr, sizeof(addr));
	if (bindResult != 0) {
		// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] bind failed: %d", WSAGetLastError());
		return 0;
	}

	u_long bAllowNonBlocking = 1;
	int setNonBlockingResult = ioctlsocket(Socket,FIONBIO,&bAllowNonBlocking);
	if (setNonBlockingResult != 0) {
		// LogToFile("C:\\mylog.txt", "[UdpNetDriver::InitListen] setNonBlocking failed: %d", WSAGetLastError());
		return 0;
	}

	*(void**)((char*)NetDriver + 0x14C) = SocketInstance;
	*(void**)((char*)NetDriver + 0x54) = Notify;

	Logger::Log("debug", "MINE UdpNetDriver__InitListen END\n");
	return 1;
}

