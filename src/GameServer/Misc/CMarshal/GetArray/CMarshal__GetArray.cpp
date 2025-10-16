#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, uint32_t> CMarshal__GetArray::m_values;

int __fastcall CMarshal__GetArray::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	return result;
}

