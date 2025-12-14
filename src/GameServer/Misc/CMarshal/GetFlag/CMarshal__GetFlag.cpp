#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CMarshal__GetFlag::bLogEnabled = false;
std::map<int, unsigned char> CMarshal__GetFlag::m_values;

int __fastcall CMarshal__GetFlag::Call(void* CMarshal, void* edx, int Field, unsigned char* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}

	if (bLogEnabled) {
		int iVar2 = *(int *)(0x11943694 + Field * 0x68);
		Logger::Log(GetLogChannel(), "Resolved type: %d\n", iVar2);

		// 0 expects single-char string Y/y/T/t
	}

	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: 0x%02X\n", Field, *Out);
		LogCallEnd();
	}

	return result;
}

