#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Finds the best team-appropriate spawn point for the given controller.
// Uses TgTeamPlayerStart.m_nTaskForce to match player's team, then picks
// the highest-rated enabled start point. Falls back to any start if no
// team-specific start is found.
ANavigationPoint* __fastcall TgGame__TgFindPlayerStart::Call(ATgGame* Game, void* edx, AController* Controller, FName IncomingName) {
	LogCallBegin();

	ActorCache::CacheMapActors();

	if (ActorCache::PlayerStarts.empty()) {
		Logger::Log("spawn", "TgFindPlayerStart: no player starts cached, falling back to original\n");
		LogCallEnd();
		return CallOriginal(Game, edx, Controller, IncomingName);
	}

	// Determine the player's task force number for team filtering
	unsigned char playerTaskForce = 0;
	if (Controller != nullptr && Controller->PlayerReplicationInfo != nullptr) {
		ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)Controller->PlayerReplicationInfo;
		if (RepInfo->r_TaskForce != nullptr) {
			playerTaskForce = RepInfo->r_TaskForce->r_nTaskForce;
		}
	}

	// Find best start matching the player's team, using the rating system
	ATgTeamPlayerStart* bestStart = nullptr;
	float bestRating = -1.0f;

	// Also track best fallback (any team) in case no team match is found
	ATgTeamPlayerStart* fallbackStart = nullptr;
	float fallbackRating = -1.0f;

	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (start == nullptr) continue;
		if (!start->bEnabled) continue;

		// Calculate rating: m_fCurrentRating + 100 if bPrimaryStart (matches UC GetRating)
		float rating = start->m_fCurrentRating;
		if (start->bPrimaryStart) {
			rating += 100.0f;
		}

		// Track best for any team (fallback)
		if (rating > fallbackRating) {
			fallbackRating = rating;
			fallbackStart = start;
		}

		// Team-specific match
		if (playerTaskForce != 0 && start->m_nTaskForce != playerTaskForce) continue;

		if (rating > bestRating) {
			bestRating = rating;
			bestStart = start;
		}
	}

	ATgTeamPlayerStart* chosen = bestStart ? bestStart : fallbackStart;

	if (chosen != nullptr) {
		// Adjust rating for next use (matches UC AdjustRating)
		if (chosen->m_fDecreaseRate != 0.0f) {
			chosen->m_fCurrentRating -= chosen->m_fDecreaseRate;
			if (chosen->m_fCurrentRating < chosen->m_fResetRating) {
				chosen->m_fCurrentRating = chosen->m_fStartRating;
			}
		}

		Logger::Log("spawn", "TgFindPlayerStart: player taskForce=%d -> start %s (taskForce=%d, rating=%.1f)\n",
			playerTaskForce, ((UObject*)chosen)->GetName(), chosen->m_nTaskForce, bestRating);

		LogCallEnd();
		return chosen;
	}

	Logger::Log("spawn", "TgFindPlayerStart: no suitable start found, falling back to original\n");
	LogCallEnd();
	return CallOriginal(Game, edx, Controller, IncomingName);
}
