#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, uint32_t> CMarshal__GetArray::m_values;
bool CMarshal__GetArray::bLogEnabled = false;

int __fastcall CMarshal__GetArray::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}

	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: %d \n", Field, *Out);
		void* this_00 = *(void **)((*Out) + 4);
		if (this_00 != nullptr && (int)this_00 > 0x1000) {
			Logger::DumpMemory(GetLogChannel(), this_00, 0x200, 0);
		}
		LogCallEnd();
	}

	return result;
}

