#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Spawn-point picker for the stripped native `TgGame.TgFindPlayerStart`.
// UC entry: TgGame.uc:631 FindPlayerStart → TgFindPlayerStart.
//
// Filter (eligibility):
//   * bEnabled
//   * Task-force match — if both player and start carry a non-zero task force
//     they must agree; zero on either side means "team-agnostic, accept any".
//   * Priority gate — TgTeamPlayerStart.m_nPriority drives the forward-spawn
//     handoff: <= 0 means always available, > 0 means available only while
//     Game.s_nCurrentPriority matches. Mirrors TgLevelCamera.uc:53.
//     s_nCurrentPriority is advanced by TgGame::UnlockObjective when an
//     objective at the current priority completes.
//
// Ranking: m_fCurrentRating + (bPrimaryStart ? 100 : 0)  (TgStartPoint.uc:27).
// After picking, AdjustRating() (TgStartPoint.uc:49) decays the winner's
// rating so consecutive spawns rotate across eligible starts.
//
// Field seeding (m_nTaskForce, m_nPriority, nPrevPriority, m_nMinLevel,
// m_eCoalition) is handled by TgTeamPlayerStart::LoadObjectConfig pulling
// from map_object_config. If a map has no override rows, fields stay at
// their UC defaults and selection degrades gracefully.

ANavigationPoint* __fastcall TgGame__TgFindPlayerStart::Call(ATgGame* Game, void* edx, AController* Controller, FName IncomingName) {
	LogCallBegin();

	ActorCache::CacheMapActors();

	if (ActorCache::PlayerStarts.empty()) {
		Logger::Log("spawn", "TgFindPlayerStart: no player starts cached, falling back to original\n");
		LogCallEnd();
		return CallOriginal(Game, edx, Controller, IncomingName);
	}

	unsigned char playerTaskForce = 0;
	if (Controller != nullptr && Controller->PlayerReplicationInfo != nullptr) {
		ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)Controller->PlayerReplicationInfo;
		if (RepInfo->r_TaskForce != nullptr) {
			playerTaskForce = RepInfo->r_TaskForce->r_nTaskForce;
		}
	}

	const int currentPriority = Game ? Game->s_nCurrentPriority : 0;
	const bool logEnabled = Logger::IsChannelEnabled("spawn");

	if (logEnabled) {
		Logger::Log("spawn",
			"TgFindPlayerStart: %zu candidate(s), playerTaskForce=%d, s_nCurrentPriority=%d\n",
			ActorCache::PlayerStarts.size(), playerTaskForce, currentPriority);
	}

	ATgTeamPlayerStart* best = nullptr;
	float bestRating = -1.0f;

	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (start == nullptr) continue;

		const float rating = start->m_fCurrentRating + (start->bPrimaryStart ? 100.0f : 0.0f);

		const bool enabled       = start->bEnabled;
		const bool taskForceOk   = playerTaskForce == 0 || start->m_nTaskForce == 0 || start->m_nTaskForce == playerTaskForce;
		const bool priorityOk    = start->m_nPriority <= 0 || start->m_nPriority == currentPriority;
		const bool eligible      = enabled && taskForceOk && priorityOk;

		if (logEnabled) {
			Logger::Log("spawn",
				"  %s mapObjId=%d enabled=%d tf=%d prio=%d rating=%.1f primary=%d eligible=%d loc=(%.0f,%.0f,%.0f)\n",
				((UObject*)start)->GetName(),
				start->m_nMapObjectId,
				enabled ? 1 : 0,
				(int)start->m_nTaskForce,
				start->m_nPriority,
				rating,
				start->bPrimaryStart ? 1 : 0,
				eligible ? 1 : 0,
				start->Location.X, start->Location.Y, start->Location.Z);
		}

		if (!eligible) continue;
		if (rating > bestRating) {
			bestRating = rating;
			best = start;
		}
	}

	if (best == nullptr) {
		Logger::Log("spawn", "TgFindPlayerStart: no eligible start, falling back to original\n");
		LogCallEnd();
		return CallOriginal(Game, edx, Controller, IncomingName);
	}

	if (best->m_fDecreaseRate != 0.0f) {
		best->m_fCurrentRating -= best->m_fDecreaseRate;
		if (best->m_fCurrentRating < best->m_fResetRating) {
			best->m_fCurrentRating = best->m_fStartRating;
		}
	}

	if (Controller != nullptr) {
		s_ChosenYaw[Controller] = best->Rotation.Yaw;
	}

	if (logEnabled) {
		Logger::Log("spawn",
			"TgFindPlayerStart: chose %s mapObjId=%d tf=%d prio=%d rating=%.1f yaw=%d\n",
			((UObject*)best)->GetName(), best->m_nMapObjectId,
			(int)best->m_nTaskForce, best->m_nPriority, bestRating, best->Rotation.Yaw);
	}

	LogCallEnd();
	return best;
}
