#pragma once

#include "src/pch.hpp"

// TgPlayerController.ServerRequestBeaconNetworkHop(TgStartPoint) - the zone
// "Beacon Node" fast travel (omega volume type 1638 -> zone map -> pick a
// node). Native thunk 0x1098b510 calls vtable +0x4ac, which Hi-Rez stripped to
// a shared `ret 4` stub (0x10963360), so it is patched per vtable slot (the
// stub is shared and must not be detoured).
class TgPlayerController__ServerRequestBeaconNetworkHop {
public:
	static void Install();
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, ATgStartPoint* pStartPoint);
	// Called by TgOmegaVolume_Used when a player uses a Beacon Net Node volume;
	// a hop is only accepted shortly after one.
	static void NoteBeaconUse(ATgPlayerController* Controller);
};
