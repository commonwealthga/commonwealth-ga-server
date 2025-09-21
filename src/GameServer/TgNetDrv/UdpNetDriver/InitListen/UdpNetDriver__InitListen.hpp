#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class UdpNetDriver__InitListen : public HookBase<
	int(__fastcall*)(UUdpNetDriver*, void*, FURL*, FString*),
	0x10C16030,
	UdpNetDriver__InitListen> {
public:
    static int __fastcall Call(UUdpNetDriver* NetDriver, void* edx, void* Notify, FURL* Url, FString* Error);
	static inline int __fastcall CallOriginal(UUdpNetDriver* NetDriver, void* Notify, FURL* Url, FString* Error) {
		return m_original(NetDriver, Notify, Url, Error);
	}
};

