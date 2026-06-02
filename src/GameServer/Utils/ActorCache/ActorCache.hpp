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
	// Volumes that fire TgSeqEvent_PlayerCountHit when N attackers (or members
	// of a configured TaskForceNumber) are inside. Native Update was a stub;
	// TgPlayerCountVolume__Update hook replaces it and reads/writes the
	// Players array + PlayerCountTarget on each cached entry.
	static std::vector<ATgPlayerCountVolume*> PlayerCountVolumes;
	static ATgBotFactory* BotFactory;

	static void CacheMapActors();
};
