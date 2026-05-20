#include "src/pch.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"

std::map <int32_t, ClientConnectionData> GClientConnectionsData;
std::unordered_map<uint64_t, UNetConnection*> GConnectionByAddr;

