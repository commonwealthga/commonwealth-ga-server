#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CMarshal__GetFloat::bLogEnabled = false;
std::map<int, float> CMarshal__GetFloat::m_values;

int __fastcall CMarshal__GetFloat::Call(void* CMarshal, void* edx, int Field, float* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}
	// iVar2 = *(int *)(&DAT_11943694 + *(int *)((int)this + 0x10) * 0x68);

	if (bLogEnabled) {
		int iVar2 = *(int *)(0x11943694 + Field * 0x68);
		Logger::Log(GetLogChannel(), "Resolved type: %d\n", iVar2);

		// 7 = expects double
	}


	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: %f\n", Field, *Out);
		LogCallEnd();
	}

	return result;
}

