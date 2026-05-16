#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Stripped native `Function TgGame.TgTeamPlayerStart.GetRating` at 0x109f3990.
// Binary stub is `FLDZ; RET 0x4` — pushes 0.0 to ST(0), returns, callee
// pops one 4-byte stack arg (the Controller* parameter).
//
// ABI match: __thiscall `float(this, AController*)` is equivalent to
// __fastcall `float(this, edx_junk, AController*)`:
//   ECX = this,  EDX = unused,  [esp+4] = Player,  return float in ST(0),
//   callee cleans 4 bytes (ret 4).
//
// Mirrors the parent TgStartPoint::GetRating UC logic — task-force gating
// is already enforced externally by TgGame::TgFindPlayerStart, so this is
// just a property lookup that returns m_fCurrentRating (+100 for primary
// start, 0 when disabled).
class TgTeamPlayerStart__GetRating : public HookBase<
	float(__fastcall*)(ATgTeamPlayerStart*, void*, AController*),
	0x109f3990,
	TgTeamPlayerStart__GetRating> {
public:
	static float __fastcall Call(ATgTeamPlayerStart* PlayerStart, void* edx, AController* Player);
	static inline float __fastcall CallOriginal(ATgTeamPlayerStart* PlayerStart, void* edx, AController* Player) {
		return m_original(PlayerStart, edx, Player);
	}
};
