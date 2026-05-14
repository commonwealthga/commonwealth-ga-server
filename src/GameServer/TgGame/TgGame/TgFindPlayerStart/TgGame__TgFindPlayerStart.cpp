#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

// Temporary hardcoded m_nMapObjectId -> taskForce mapping.
// The proper fix is hooking TgTeamPlayerStart::LoadObjectConfig() to populate
// m_nTaskForce from a DB table (asm_data_set_team_player_starts or similar).
// Until that lands, this table seeds the picker with team identity per map.
//
// Add entries as new maps are discovered. Values: 1=Attackers, 2=Defenders.
static const std::unordered_map<int, int> kHardcodedStartTaskForceByMapObjectId = {
	// Sonora Desert (working name) — discovered 2026-05-11 from spawn log.
	{ 11717, 1 }, // Attackers spawn (Z=603)
	{ 12772, 2 }, // Defenders spawn (Z=-333)
};

static int ResolveStartTaskForce(ATgTeamPlayerStart* start) {
	if (start == nullptr) return 0;
	// Honor any value LoadObjectConfig actually populated (future-proofing for
	// when the proper hook lands).
	if (start->m_nTaskForce != 0) return start->m_nTaskForce;
	auto it = kHardcodedStartTaskForceByMapObjectId.find(start->m_nMapObjectId);
	if (it != kHardcodedStartTaskForceByMapObjectId.end()) return it->second;
	return 0;
}

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

	const bool spawnLogEnabled = Logger::IsChannelEnabled("spawn");
	if (spawnLogEnabled) {
		Logger::Log("spawn", "TgFindPlayerStart: %zu start(s) cached, playerTaskForce=%d\n",
			ActorCache::PlayerStarts.size(), playerTaskForce);
	}

	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (start == nullptr) {
			if (spawnLogEnabled) Logger::Log("spawn", "TgFindPlayerStart:   (null start entry skipped)\n");
			continue;
		}

		// Calculate rating: m_fCurrentRating + 100 if bPrimaryStart (matches UC GetRating)
		float rating = start->m_fCurrentRating;
		if (start->bPrimaryStart) {
			rating += 100.0f;
		}

		// Resolve effective task-force: prefer the actor's own m_nTaskForce
		// (set by LoadObjectConfig, currently stubbed), fall back to hardcoded
		// map_object_id table.
		const int startTaskForce = ResolveStartTaskForce(start);

		// Log every start before filtering so we can see disabled/team-zero entries too.
		if (spawnLogEnabled) {
			Logger::Log("spawn",
				"TgFindPlayerStart:   start %s mapObjId=%d enabled=%d taskForce=%d(raw=%d) coalition=%d priority=%d rating=%.1f primary=%d loc=(%.0f,%.0f,%.0f)\n",
				((UObject*)start)->GetName(),
				start->m_nMapObjectId,
				start->bEnabled ? 1 : 0,
				startTaskForce,
				(int)start->m_nTaskForce,
				(int)start->m_eCoalition,
				(int)start->m_nPriority,
				rating,
				start->bPrimaryStart ? 1 : 0,
				start->Location.X, start->Location.Y, start->Location.Z);
		}

		if (!start->bEnabled) continue;

		// Track best for any team (fallback)
		if (rating > fallbackRating) {
			fallbackRating = rating;
			fallbackStart = start;
		}

		// Team-specific match
		if (playerTaskForce != 0 && startTaskForce != playerTaskForce) continue;

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

		if (Logger::IsChannelEnabled("spawn")) {
			Logger::Log("spawn", "TgFindPlayerStart: player taskForce=%d -> start %s (taskForce=%d, mapObjId=%d, rating=%.1f)\n",
				playerTaskForce, ((UObject*)chosen)->GetName(),
				ResolveStartTaskForce(chosen), chosen->m_nMapObjectId, bestRating);
		}

		LogCallEnd();
		return chosen;
	}

	Logger::Log("spawn", "TgFindPlayerStart: no suitable start found, falling back to original\n");
	LogCallEnd();
	return CallOriginal(Game, edx, Controller, IncomingName);
}
