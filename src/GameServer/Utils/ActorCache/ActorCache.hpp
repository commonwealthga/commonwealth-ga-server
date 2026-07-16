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
	// Safe-zone / Omega volumes are baked into maps and never change post-load.
	// SpawnPlayerCharacter walks these to repair Touching overlaps on respawn
	// (engine Touched events don't fire from SetLocation teleports).
	static std::vector<ATgModifyPawnPropertiesVolume*> ModifyPawnPropertiesVolumes;
	static std::vector<ATgOmegaVolume*> OmegaVolumes;
	// Map-baked deployable factories (EMP posts, kismet-timed FX). InitGameRepInfo
	// kicks the s_bAutoSpawn ones — TgDeployableFactory.PostBeginPlay only
	// auto-spawns under NM_DedicatedServer, which is never true on this build.
	static std::vector<ATgDeployableFactory*> DeployableFactories;
	static ATgBotFactory* BotFactory;

	static void CacheMapActors();
};
