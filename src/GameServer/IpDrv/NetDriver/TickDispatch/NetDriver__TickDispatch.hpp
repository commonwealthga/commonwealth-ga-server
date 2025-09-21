#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class NetDriver__TickDispatch : public HookBase<
	void(__fastcall*)(UNetDriver*, void*, float),
	0x10C171A0,
	NetDriver__TickDispatch> {
public:
    static void __fastcall Call(UNetDriver* NetDriver, void* edx, float DeltaTime);
	static inline void __fastcall CallOriginal(UNetDriver* NetDriver, void* edx, float DeltaTime) {
		return m_original(NetDriver, edx, DeltaTime);
	}
};

