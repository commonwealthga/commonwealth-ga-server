#pragma once

#include "src/pch.hpp"

#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// "Has the match actually started?" gate for score/morale crediting.
//
// Players are already in the map and can damage/heal each other during the
// pre-match setup countdown of every mode, so anything credited in that window
// is farmable at zero risk (the historical PvP stat/morale padding exploit).
//
// Signal: `ATgGame.m_eTimerState` (TgGame.uc:25 GameTimerState).
//   TGMTS_UNSET(0) → TGMTS_SETUP(1) → TGMTS_MISSION_RUNNING(2) → ...
// `TgGame.PostLogin` → `StartGameTimer` arms the setup phase on the first
// player login and `TgGame.MissionTimer` promotes it to MISSION_RUNNING when
// the countdown expires — which is also the moment `TgGame_Arena.MissionTimer`
// calls `GameStarted()` (→ PreRound → RoundInProgress). Retail marks the same
// boundary on the GRI: `TgGame.ChangeTimerState` clears
// `r_bAllowBuildMorale` when entering SETUP and sets it again when leaving it
// (TgGame.uc:1029-1056), which is the flag the stripped AddMoralePoints native
// gated on.
//
// Per-mode handling of TGMTS_UNSET:
//   * Arena subclasses (TgGame_Arena / _CTF / _Defense / _PointRotation)
//     override StartGameTimer with NO mission-time precondition, so a login
//     always moves them 0 → 1; UNSET therefore means "no match has begun".
//   * Other modes (TgGame_Mission / _Ticket / _Control / _City / _OpenWorld /
//     _Escort) only arm a timer when the map carries a mission time. Open-world
//     maps never do and sit at UNSET forever, so UNSET must fail OPEN there —
//     otherwise their stats would be dead for the whole instance.
namespace MatchGate {

// GameTimerState (TgGame.uc:25). Only the two pre-match values matter here.
constexpr int kTimerUnset = 0;
constexpr int kTimerSetup = 1;

inline ATgGame* GameOf(AActor* context) {
	if (!context || !context->WorldInfo || !context->WorldInfo->Game) return nullptr;
	return (ATgGame*)context->WorldInfo->Game;
}

// TgGame_Arena and the subclasses that inherit its round state machine.
// TgGame_CTF / _Defense / _PointRotation are the only ones in the shipped UC.
inline bool IsArenaMode(AActor* context) {
	ATgGame* game = GameOf(context);
	if (!game || !game->Class) return false;
	const std::string& cls = ObjectClassCache::GetClassName(game->Class);
	return cls.find("TgGame_Arena")         != std::string::npos
	    || cls.find("TgGame_CTF")           != std::string::npos
	    || cls.find("TgGame_Defense")       != std::string::npos
	    || cls.find("TgGame_PointRotation") != std::string::npos;
}

// Fail-open by design: missing state must never stop crediting on a live
// match. Only a positively identified pre-match phase returns false.
inline bool MatchLive(AActor* context) {
	ATgGame* game = GameOf(context);
	if (!game) return true;
	const int timerState = (int)game->m_eTimerState;

	bool live;
	if (IsArenaMode(context)) {
		// UNSET and SETUP both precede the first GameStarted().
		live = timerState >= 2;
	} else {
		// Only the setup countdown is pre-match; UNSET fails open (open world).
		live = (timerState != kTimerSetup);
	}
	if (live) return true;

	// Rate-limited so a burst of pre-match damage doesn't flood the log — the
	// whole point is that these paths fire constantly while the window is open.
	static double lastLog = 0.0;
	const double now = (double)GetTickCount64() / 1000.0;
	if (now - lastLog >= 10.0) {
		lastLog = now;
		Logger::Log("matchstats",
			"[MatchGate] pre-match window: credit blocked (game=%s timerState=%d)\n",
			game->Class ? ObjectClassCache::GetClassName(game->Class).c_str() : "<null>",
			timerState);
	}
	return false;
}

}  // namespace MatchGate
