#pragma once

#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/pch.hpp"

struct ClientConnectionData {
	sockaddr_in RemoteAddr;
	void* SocketInstance;
	FURL url;
	char RemoteAddrString[32];
	FString* RemoteAddrFString;
	ATgPawn_Character* Pawn;
	bool bClosed;
	std::string SessionGuid;
	PlayerInfo PlayerInfo;
	struct PlayerInfo* pPlayerInfo = nullptr; // stable pointer into PlayerRegistry::by_guid_
};

extern std::map <int32_t, ClientConnectionData> GClientConnectionsData;

