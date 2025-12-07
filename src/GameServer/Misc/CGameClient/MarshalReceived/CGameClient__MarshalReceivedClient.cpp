#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/Utils/Logger/Logger.hpp"

uint8_t __fastcall CGameClient__MarshalReceived::Call(void* GameClient, void* edx, void* InBunch) {

	LogCallBegin();

	short uVar1 = *(short*)((int)InBunch + 0x26);
	Logger::Log(GetLogChannel(), "Received packet type [0x%04X]\n", uVar1);

	CMarshal__GetByte::bLogEnabled = true;
	CMarshal__GetInt32t::bLogEnabled = true;
	CMarshal__GetArray::bLogEnabled = true;

	uint8_t result = CallOriginal(GameClient, edx, InBunch);

	CMarshal__GetByte::bLogEnabled = false;
	CMarshal__GetInt32t::bLogEnabled = false;
	CMarshal__GetArray::bLogEnabled = false;

	LogCallEnd();

	return result;
}

