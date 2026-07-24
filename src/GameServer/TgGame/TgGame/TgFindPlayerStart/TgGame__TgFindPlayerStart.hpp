#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__TgFindPlayerStart : public HookBase<
	ANavigationPoint*(__fastcall*)(ATgGame*, void*, AController*, FName),
	0x10a744b0,
	TgGame__TgFindPlayerStart> {
public:
	// Yaw of the start last chosen for each controller. SpawnPlayerCharacter
	// consumes it so the pawn faces out of the spawn room instead of keeping
	// the controller's stale rotation.
	static inline std::unordered_map<AController*, int> s_ChosenYaw;

	static ANavigationPoint* __fastcall Call(ATgGame* Game, void* edx, AController* Controller, FName IncomingName);
	static inline ANavigationPoint* __fastcall CallOriginal(ATgGame* Game, void* edx, AController* Controller, FName IncomingName) {
		return m_original(Game, edx, Controller, IncomingName);
	};
};


