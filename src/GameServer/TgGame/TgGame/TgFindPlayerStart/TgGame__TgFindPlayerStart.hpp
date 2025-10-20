#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__TgFindPlayerStart : public HookBase<
	ANavigationPoint*(__fastcall*)(ATgGame*, void*, AController*, FName),
	0x10a744b0,
	TgGame__TgFindPlayerStart> {
public:
	static ANavigationPoint* __fastcall Call(ATgGame* Game, void* edx, AController* Controller, FName IncomingName);
	static inline ANavigationPoint* __fastcall CallOriginal(ATgGame* Game, void* edx, AController* Controller, FName IncomingName) {
		return m_original(Game, edx, Controller, IncomingName);
	};
};


