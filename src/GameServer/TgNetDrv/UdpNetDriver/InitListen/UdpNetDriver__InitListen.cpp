#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/IpDrv/SocketWin/CreateDGramSocket/SocketWin__CreateDGramSocket.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

int UdpNetDriver__InitListen::Call(UUdpNetDriver* NetDriver, void* edx, void* Notify, FURL* Url, FString* Error) {
	Logger::Log("debug", "MINE UdpNetDriver__InitListen START\n");

	int result = UdpNetDriver__InitListen::CallOriginal(NetDriver, Notify, Url, Error);
	if (!result) {
		Logger::Log("debug", "[UdpNetDriver::InitListen] CallOriginal failed\n");
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
		Logger::Log("debug", "[UdpNetDriver::InitListen] CreateDGramSocket failed\n");
		return 0;
	}

	SOCKET Socket = *(SOCKET*)((char*)SocketInstance + 0x08);

	bool bAllowBroadcast = true;
	Logger::Log("debug", "[UdpNetDriver::InitListen] set SO_BROADCAST\n");
	int setBroadcastResult = setsockopt(Socket,SOL_SOCKET,SO_BROADCAST,(char*)&bAllowBroadcast,sizeof(bool));
	if (setBroadcastResult != 0) {
		Logger::Log("debug", "[UdpNetDriver::InitListen] setsockopt SO_BROADCAST failed: %d\n", WSAGetLastError());
		return 0;
	}

	bool bAllowReuse = true;
	Logger::Log("debug", "[UdpNetDriver::InitListen] set SO_REUSEADDR\n");
	int setReuseResult = setsockopt(Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&bAllowReuse,sizeof(bool));
	if (setReuseResult != 0) {
		Logger::Log("debug", "[UdpNetDriver::InitListen] setsockopt SO_REUSEADDR failed: %d\n", WSAGetLastError());
		return 0;
	}

	// UDP socket buffer request.
	//
	// 1 MB each way. Sized to absorb ~4 seconds of full-rate game traffic at
	// 16-player scale (peak ~240 KB/s outbound) without dropping packets during
	// a GC pause or a brief network hiccup, while staying well clear of the
	// bufferbloat tail that begins around 4+ MB (where the kernel queues so
	// many packets that they go stale before we drain them — worse than
	// dropping, because clients trigger position corrections on data they
	// already extrapolated past).
	//
	// UE3 stock asks for 128 KB on the server (TcpNetDriver.cpp:554, "0x20000")
	// which is tight; one GC pause can fill it. We pick 8× that.
	//
	// IMPORTANT: this is a REQUEST. The kernel silently clamps to
	// `net.core.rmem_max` / `wmem_max`. On a default Debian/Ubuntu install
	// those are ~208 KB and the request gets clamped down. To actually get
	// 1 MB, bump the kernel limits on the host:
	//     echo "net.core.rmem_max = 4194304" > /etc/sysctl.d/99-ga-server.conf
	//     echo "net.core.wmem_max = 4194304" >> /etc/sysctl.d/99-ga-server.conf
	//     sysctl --system
	// Container note: docker spawn uses `--network=host`, so the host's
	// sysctls apply directly to our setsockopt. Dockerfile needs no change.
	//
	// The actual granted size is read back via getsockopt and logged below;
	// check the "init" channel after a server start to see what the kernel
	// actually gave us vs what we asked for.
	int RecvSize = 0x100000; // 1 MB
	int SendSize = 0x100000; // 1 MB
	int SizeSize = sizeof(int);

	const int reqRecv = RecvSize;
	const int reqSend = SendSize;

	Logger::Log("debug", "[UdpNetDriver::InitListen] set SO_RCVBUF\n");
	bool bSetRecvSizeOk = setsockopt(Socket,SOL_SOCKET,SO_RCVBUF,(char*)&RecvSize,sizeof(int)) == 0;
	getsockopt(Socket,SOL_SOCKET,SO_RCVBUF,(char*)&RecvSize, &SizeSize);

	Logger::Log("debug", "[UdpNetDriver::InitListen] set SO_SNDBUF\n");
	bool bSetSendSizeOk = setsockopt(Socket,SOL_SOCKET,SO_SNDBUF,(char*)&SendSize,sizeof(int)) == 0;
	getsockopt(Socket,SOL_SOCKET,SO_SNDBUF,(char*)&SendSize, &SizeSize);


	sockaddr_in addr = *(sockaddr_in*)((char*)NetDriver + 0x148);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Or inet_addr("0.0.0.0")
    addr.sin_port = htons(Config::GetPort());

	Logger::Log("debug", "[UdpNetDriver::InitListen] bind port=%d\n", Config::GetPort());
	int bindResult = bind(Socket, (sockaddr*)&addr, sizeof(addr));
	if (bindResult != 0) {
		Logger::Log("debug", "[UdpNetDriver::InitListen] bind failed: %d\n", WSAGetLastError());
		return 0;
	}

	u_long bAllowNonBlocking = 1;
	Logger::Log("debug", "[UdpNetDriver::InitListen] set nonblocking\n");
	int setNonBlockingResult = ioctlsocket(Socket,FIONBIO,&bAllowNonBlocking);
	if (setNonBlockingResult != 0) {
		Logger::Log("debug", "[UdpNetDriver::InitListen] setNonBlocking failed: %d\n", WSAGetLastError());
		return 0;
	}

	*(void**)((char*)NetDriver + 0x14C) = SocketInstance;
	*(void**)((char*)NetDriver + 0x54) = Notify;

	Logger::Log("debug", "MINE UdpNetDriver__InitListen END\n");
	return 1;
}

