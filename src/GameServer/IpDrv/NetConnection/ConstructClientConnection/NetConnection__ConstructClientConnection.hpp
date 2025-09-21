#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__ConstructClientConnection : public HookBase<
	void*(__cdecl*)(void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, void*),
	0x1092F2F0,
	NetConnection__ConstructClientConnection> {
public:
	static void* Call(UClass* Class, int32_t Outer, int32_t param3, int32_t param4, int32_t param5, int32_t param6, int32_t param7, int32_t param8, void* param9);
	static inline void* CallOriginal(UClass* Class, int32_t Outer, int32_t param3, int32_t param4, int32_t param5, int32_t param6, int32_t param7, int32_t param8, void* param9) {
		return m_original(Class, Outer, param3, param4, param5, param6, param7, param8, param9);
	}
};

