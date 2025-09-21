#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class World__GetGameInfo : public HookBase<
	ATgGame*(__fastcall*)(UWorld*),
	0x10BF1960,
	World__GetGameInfo> {
public:
	static ATgGame* __fastcall Call(UWorld* World);
	static inline ATgGame* __fastcall CallOriginal(UWorld* World) {
		return m_original(World);
	}
};

