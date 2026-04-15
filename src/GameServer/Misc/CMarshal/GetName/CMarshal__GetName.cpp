#include "src/GameServer/Misc/CMarshal/GetName/CMarshal__GetName.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CMarshal__GetName::bLogEnabled = false;

int __stdcall CMarshal__GetName::Call(void* CMarshal, int Field, void* Out) {
	int result = CallOriginal(CMarshal, Field, Out);

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X\n", Field);
	}

	return result;
}
