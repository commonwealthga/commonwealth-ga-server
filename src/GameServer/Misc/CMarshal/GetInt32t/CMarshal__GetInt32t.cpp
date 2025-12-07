#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, uint32_t> CMarshal__GetInt32t::m_values;
bool CMarshal__GetInt32t::bLogEnabled = false;

int __fastcall CMarshal__GetInt32t::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}

	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: %d\n", Field, *Out);
		LogCallEnd();
	}

	return result;
}

