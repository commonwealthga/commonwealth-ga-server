#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/Utils/Logger/Logger.hpp"

uint8_t __fastcall CGameClient__MarshalReceived::Call(void* GameClient, void* edx, void* InBunch) {

	Logger::Log("wtf", "CGameClient__MarshalReceived::Call\n");

	return CallOriginal(GameClient, edx, InBunch);
}

