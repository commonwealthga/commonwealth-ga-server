#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Stripped native `Function TgGame.TgTeamPlayerStart.LoadObjectConfig` lives
// at 0x109f3980 — a single RET in the shipped binary. Hook applies the
// MapObjectConfig overrides for this actor (m_n_task_force, m_e_coalition,
// m_n_priority, n_prev_priority, m_n_min_level) so the rest of the engine
// (especially TgGame::TgFindPlayerStart) sees correct team/priority data.
class TgTeamPlayerStart__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgTeamPlayerStart*, void*),
	0x109f3980,
	TgTeamPlayerStart__LoadObjectConfig> {
public:
	static void __fastcall Call(ATgTeamPlayerStart* PlayerStart, void* edx);
	static inline void __fastcall CallOriginal(ATgTeamPlayerStart* PlayerStart, void* edx) {
		m_original(PlayerStart, edx);
	}
};
