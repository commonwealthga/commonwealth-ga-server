#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, uint32_t> CMarshal__GetInt32t::m_values;

int __fastcall CMarshal__GetInt32t::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	return result;
}

