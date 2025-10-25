#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"


std::map<int, char[4096]> CMarshal__Translate::m_values;

wchar_t* __fastcall CMarshal__Translate::Call(void* CMarshal, void* edx, uint32_t a2, void* a3) {
	wchar_t* result = CallOriginal(CMarshal, edx, a2, a3);

	WideCharToMultiByte(CP_UTF8, 0, result, -1, m_values[a2], sizeof(m_values[a2]) - 1, NULL, NULL);

	return result;
}

