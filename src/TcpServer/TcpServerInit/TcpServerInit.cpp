#include "src/TcpServer/TcpServerInit/TcpServerInit.hpp"
#include "src/TcpServer/TcpServer/TcpServer.hpp"

DWORD WINAPI TcpServerInit::TcpServerThread(LPVOID) {
	try {

		asio::io_context io;
		std::vector<std::shared_ptr<TcpServer>> servers;
		servers.push_back(std::make_shared<TcpServer>(io, 9000));
		io.run();

		// io.run();
	} catch (...) {
		// LogToFile("C:\\tcplog.txt", "[TCP] Exception caught");
		// handle errors, log to file maybe
	}
    return 0;
}

void TcpServerInit::CreateTcpServerThread() {
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return;

    DWORD dwBytes = 0;

    GUID guidAcceptEx = WSAID_ACCEPTEX;
    WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
             &guidAcceptEx, sizeof(guidAcceptEx),
             &lpfnAcceptEx, sizeof(lpfnAcceptEx),
             &dwBytes, NULL, NULL);

    GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
             &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs),
             &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
             &dwBytes, NULL, NULL);

    closesocket(s);

	CreateThread(NULL, 0, TcpServerThread, NULL, 0, NULL);
}

