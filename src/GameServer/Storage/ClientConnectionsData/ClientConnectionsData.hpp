#pragma once

#include "src/pch.hpp"

struct ClientConnectionData {
	sockaddr_in RemoteAddr;
	void* SocketInstance;
	FURL url;
	char RemoteAddrString[32];
	FString* RemoteAddrFString;
	ATgPawn_Character* Pawn;
	bool bClosed;
};

extern std::map <int32_t, ClientConnectionData> GClientConnectionsData;

