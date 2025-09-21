#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class UdpNetDriver__TickDispatch : public HookBase<
	void(__fastcall*)(UUdpNetDriver*, void*, float),
	0x1092D9D0,
	UdpNetDriver__TickDispatch> {
public:
	static UClass* NetConnectionClass;
	static bool bNetConnectionVTableHooked;
    static void __fastcall Call(UUdpNetDriver* NetDriver, void* edx, float DeltaTime);
	static inline void __fastcall CallOriginal(UUdpNetDriver* NetDriver, void* edx, float DeltaTime) {
		return m_original(NetDriver, edx, DeltaTime);
	}
};

