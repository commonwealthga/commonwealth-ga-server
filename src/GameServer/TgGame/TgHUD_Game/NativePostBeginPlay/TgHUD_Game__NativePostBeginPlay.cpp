#include "src/GameServer/TgGame/TgHUD_Game/NativePostBeginPlay/TgHUD_Game__NativePostBeginPlay.hpp"
#include "src/Utils/Logger/Logger.hpp"

// GIsNonCombat @ 0x119a01c5 — 1 = city/non-combat mode, 0 = mission/combat mode
// DAT_119a01fc              — game type value ID; must be a mission value for mission HUD
//                             callbacks to register (0x144 = standard mission type)
//
// Both globals default to city-mode values. NativePostBeginPlay fires at map load
// (before the TCP login flow), so we patch them here before calling the original
// so the HUD initialises in mission mode.

void __fastcall TgHUD_Game__NativePostBeginPlay::Call(void* HUD, void* edx) {
	*(int*)0x119a01fc  = 0x144;  // game type value ID: standard mission
	*(char*)0x119a01c5 = 0;       // GIsNonCombat = false

	Logger::Log("debug", "TgHUD_Game__NativePostBeginPlay: patched globals for mission HUD\n");

	CallOriginal(HUD, edx);
}
