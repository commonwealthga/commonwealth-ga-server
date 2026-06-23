#include "src/GameServer/TgGame/MissionVODirector/MissionVODirector.hpp"
#include "src/GameServer/TgGame/MissionVO/MissionVO.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>

namespace {
// Single-threaded in-game state. Latches reset per round.
int   s_round      = -1;   // round currently in RoundInProgress
int   s_lastActive = -1;   // last satellite round seen (drives PostRound herald)
bool  s_firedProg  = false;
bool  s_firedHalf  = false;
bool  s_fired30    = false;
bool  s_fired10    = false;
bool  s_firedArrive = false;  // PostRound herald, one per between-round gap
float s_roundDur   = 0.0f;
float s_throttle   = 0.0f;
}  // namespace

void MissionVODirector::Tick(float dt) {
	if (Config::GetMapNameChar() != std::string("Raid_DomeCityDefense_P")) return;

	// ~4Hz: enough for "30s/10s/5s" VO precision, keeps native timer queries off
	// the per-frame path.
	s_throttle += dt;
	if (s_throttle < 0.25f) return;
	s_throttle = 0.0f;

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game == nullptr) return;
	if (!ObjectClassCache::ClassNameContains((UObject*)Game, "TgGame_Defense")) return;
	ATgGame_Defense* Defense = (ATgGame_Defense*)Game;

	const char* stRaw = Game->GetStateName().GetName();
	const std::string state(stRaw ? stRaw : "");
	const int maxRound = Defense->s_nMaxRoundNumber;  // 5; boss = maxRound (untimed)

	if (state == "RoundInProgress") {
		const int round = Defense->s_nRoundNumber;
		if (round < 1) return;

		if (round != s_round) {
			s_round = round;
			s_firedProg = s_firedHalf = s_fired30 = s_fired10 = false;
			s_firedArrive = false;  // re-arm the herald for the upcoming gap
			s_roundDur = Defense->GetRoundDuration(round);
		}

		// Boss round (>= maxRound) is untimed (GetRoundDuration==0) — no satellite
		// countdown. Note Bancroft_Start / Bancroft_BossIncoming drive no PlaySound
		// in this map's kismet, so there's nothing to fire at round start.
		const bool satelliteRound = (maxRound <= 0) || (round < maxRound);
		if (!satelliteRound) return;

		s_lastActive = round;

		const float remaining = Game->GetRemainingTimeForTimer(FName("MissionTimer"), nullptr);
		if (remaining <= 0.0f) return;
		if (s_roundDur <= 0.0f) s_roundDur = remaining;  // fallback: ~full at start

		// Descending satellite-charge countdown — each fires once per round. The
		// map kismet's own counter on Bancroft_10sRemaining diverts round 4 to the
		// "satellite failed" + dismantler lines, so we fire it every satellite round.
		if (!s_firedProg && s_roundDur > 0.0f && remaining <= s_roundDur * 0.75f) {
			s_firedProg = true;  // 25% elapsed = satellite quarter charged
			MissionVO::CauseEvent("Bancroft_Progress");
		}
		if (!s_firedHalf && s_roundDur > 0.0f && remaining <= s_roundDur * 0.5f) {
			s_firedHalf = true;
			MissionVO::CauseEvent("Bancroft_HalfwayPoint");
		}
		if (!s_fired30 && remaining <= 30.0f) {
			s_fired30 = true;
			MissionVO::CauseEvent("Bancroft_30sRemaining");
		}
		if (!s_fired10 && remaining <= 10.0f) {
			s_fired10 = true;
			MissionVO::CauseEvent("Bancroft_10sRemaining");
		}
		return;
	}

	if (state == "PostRound") {
		// "Bancroft arrives at location" — 5s before the NEXT wave starts. The
		// between-round gap is the NextRoundStart timer (s_nBetweenRoundDelay=30s).
		// Skip when the next wave is the boss (s_lastActive+1 >= maxRound).
		if (s_firedArrive || s_lastActive < 1) return;
		if (maxRound > 0 && (s_lastActive + 1) >= maxRound) return;
		const float delayLeft = Game->GetRemainingTimeForTimer(FName("NextRoundStart"), nullptr);
		if (delayLeft > 0.0f && delayLeft <= 5.0f) {
			s_firedArrive = true;
			MissionVO::CauseEvent("Bancroft_ArriveAtLocation");
		}
		return;
	}
}
