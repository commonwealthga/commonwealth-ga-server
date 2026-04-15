#include "src/GameServer/Misc/CMarshal/GetWcharT/CMarshal__GetWcharT.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, char[4096]> CMarshal__GetWcharT::m_values;
bool CMarshal__GetWcharT::bLogEnabled = false;

int __fastcall CMarshal__GetWcharT::Call(void* CMarshal, void* edx, int Field, int* Out) {
	int result = CallOriginal(CMarshal, edx, Field, Out);

	if (Out != nullptr && *Out != 0) {
		wchar_t* w = reinterpret_cast<wchar_t*>(*Out);
		WideCharToMultiByte(CP_UTF8, 0, w, -1, m_values[Field], sizeof(m_values[Field]) - 1, NULL, NULL);
	}

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X utf8: %s\n", Field, m_values[Field]);
	}

	return result;
}
