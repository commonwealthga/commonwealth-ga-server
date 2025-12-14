#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/Utils/Logger/Logger.hpp"

uint8_t __fastcall CGameClient__MarshalReceived::Call(void* GameClient, void* edx, void* InBunch) {

	LogCallBegin();

	short uVar1 = *(short*)((int)InBunch + 0x26);
	Logger::Log(GetLogChannel(), "Received packet type [0x%04X] (suppressed on server)\n", uVar1);

	LogCallEnd();

	return 0;
}

