#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__SendPackageMap : public HookBase<
	void(__fastcall*)(void*),
	0x10c1c820,
	NetConnection__SendPackageMap> {
public:
	static void __fastcall Call(void* Connection);
	static inline void __fastcall CallOriginal(void* Connection) {
		LogCallOriginalBegin();
		m_original(Connection);
		LogCallOriginalEnd();
	}
};
