#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"

std::map<int, char[4096]> CMarshal__GetString2::m_values;

int __fastcall CMarshal__GetString2::Call(void *CMarshal, void *edx, uint32_t a2, wchar_t *a3, uint32_t *a4) {
	int result = CallOriginal(CMarshal, edx, a2, a3, a4);

	wchar_t safeCopy[4096] = {0};
	for (int i = 0; i < 4096; i++) {
		safeCopy[i] = a3[i];
		if (a3[i] == L'\0') {
			break;
		}
	}

	WideCharToMultiByte(CP_UTF8, 0, safeCopy, -1, m_values[a2], sizeof(m_values[a2]) - 1, NULL, NULL);

	return result;
}

