#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgHUD_Game::DrawActorOverlays @ 0x113aa6f0 — the UC-level pawn iterator that
// per-actor dispatches PostRenderFor (via FUN_10c6e5c0 exec wrapper). If this
// doesn't fire, overhead rendering for ALL pawns is dead.
class TgHUD_Game__DrawActorOverlays : public HookBase<
	void(__fastcall*)(void*, void*),
	0x113aa6f0,
	TgHUD_Game__DrawActorOverlays> {
public:
	static void __fastcall Call(void* HUD, void* edx);
	static inline void __fastcall CallOriginal(void* HUD, void* edx) {
		m_original(HUD, edx);
	}
};
