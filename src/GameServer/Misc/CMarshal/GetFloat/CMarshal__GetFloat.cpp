#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, float> CMarshal__GetFloat::m_values;

int __fastcall CMarshal__GetFloat::Call(void* CMarshal, void* edx, int Field, float* Out) {
	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	return result;
}

