#pragma once

#include "src/pch.hpp"

class TcpServerInit {
public:
	static DWORD WINAPI TcpServerThread(LPVOID);
	static void CreateTcpServerThread();
};

