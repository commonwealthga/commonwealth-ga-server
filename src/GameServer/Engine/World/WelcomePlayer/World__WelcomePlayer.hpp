#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class World__WelcomePlayer : public HookBase<
	void(__fastcall*)(UWorld*, void*, UNetConnection*),
	0x10BEFE20,
	World__WelcomePlayer> {
public:
	static void __fastcall Call(UWorld* World, void* edx, UNetConnection* Connection);
	static inline void __fastcall CallOriginal(UWorld* World, void* edx, UNetConnection* Connection) {
		return m_original(World, edx, Connection);
	}
};

