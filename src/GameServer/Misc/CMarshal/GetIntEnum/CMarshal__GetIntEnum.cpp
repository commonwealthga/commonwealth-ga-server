#include "src/GameServer/Misc/CMarshal/GetIntEnum/CMarshal__GetIntEnum.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CMarshal__GetIntEnum::bLogEnabled = false;
std::map<int, int> CMarshal__GetIntEnum::m_values;

int __fastcall CMarshal__GetIntEnum::Call(void* CMarshal, void* edx, int Field, int* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}

	if (bLogEnabled) {
		int iVar2 = *(int *)(0x11943694 + Field * 0x68);
		Logger::Log(GetLogChannel(), "Resolved type: %d\n", iVar2);
	}

	// uint32_t value = *(uint32_t*)((char*)CMarshal + 4);
	int OutTmp;

	// int result = CallOriginal(CMarshal, edx, Field, Out);
	int result = CallOriginal(CMarshal, edx, Field, &OutTmp);

	// m_values[Field] = *Out;
	m_values[Field] = OutTmp;

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: %d\n", Field, OutTmp);
		LogCallEnd();
	}

	*Out = OutTmp;

	return result;
}

