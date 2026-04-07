#pragma once

#include "src/pch.hpp"

// Provides typed access to cached map actors via ObjectCache.
// CacheMapActors() is kept as the call point but now just triggers
// ObjectCache to scan to completion, then builds typed vectors from it.
class ActorCache {
private:
	static bool bCached;

public:
	static std::vector<ATgTeamPlayerStart*> PlayerStarts;
	static std::vector<ATgMissionObjective*> MissionObjectives;
	static std::vector<ATgRandomSMActor*> RandomSMActors;
	static ATgBotFactory* BotFactory;

	static void CacheMapActors();
};
