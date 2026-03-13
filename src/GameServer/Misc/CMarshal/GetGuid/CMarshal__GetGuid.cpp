#include "src/GameServer/Misc/CMarshal/GetGuid/CMarshal__GetGuid.hpp"
#include "src/Utils/Logger/Logger.hpp"


int __fastcall CMarshal__GetGuid::Call(void* CMarshal, void* edx, int Field, UUID* Out) {
	return CallOriginal(CMarshal, edx, Field, Out);
}

