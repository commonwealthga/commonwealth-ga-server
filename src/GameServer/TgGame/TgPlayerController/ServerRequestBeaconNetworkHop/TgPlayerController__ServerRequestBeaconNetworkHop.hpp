#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerRequestBeaconNetworkHop : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, ATgStartPoint*),
	0x10963360,
	TgPlayerController__ServerRequestBeaconNetworkHop> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, ATgStartPoint* pStartPoint);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, ATgStartPoint* pStartPoint) {
		m_original(Controller, edx, pStartPoint);
	}
};
