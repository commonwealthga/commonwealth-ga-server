#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, uint32_t> CMarshal__GetByte::m_values;

int __fastcall CMarshal__GetByte::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	return result;
}

