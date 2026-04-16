#pragma once

#include "src/pch.hpp"

// SyncPawnHealth — one-call fan-out for the seven places HP can live on a
// player or bot pawn. Use AT SPAWN TIME (pre- or post-PRI wiring) to keep
// every field consistent in a single line. After spawn, the canonical write
// path is `pawn->SetProperty(TGPID_HEALTH_MAX, x)` — the binary's
// ApplyProperty native does the same fan-out internally, but only when PRI
// is wired and SetProperty is safe to call.
//
// Writes / sync targets:
//   1. APawn::Health (0x2C4)             — engine current HP
//   2. APawn::HealthMax (0x2C8)          — engine max HP
//   3. ATgPawn::r_nHealthMaximum (0x43C) — Tg replicated max HP
//   4. UTgProperty[51 HEALTH].m_fRaw / m_fMaximum
//   5. UTgProperty[304 HEALTH_MAX].m_fRaw / m_fMaximum
//   6. ATgRepInfo_Player::r_nHealthCurrent — via UC UpdateHealth event (if PRI wired)
//   7. ATgRepInfo_Player::r_nHealthMaximum — via UC UpdateHealth event (if PRI wired)
namespace SyncPawnHealth {
	void Apply(ATgPawn* pawn, int hp, int maxHp);
}
