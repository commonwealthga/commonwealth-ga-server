#pragma once

#include "src/pch.hpp"

#include <map>
#include <vector>

// Custom "Hardcore Security" difficulty: a standard PvE mission (boss fight
// ending, no extra objectives) above Ultra-Max. Active only under
// GA_G::DIFFICULTY_VALUE_ID_CUSTOM_HARDCORE_SECURITY.
//
// Gameplay deltas vs Ultra-Max:
//   - difficulty scalar (Config::GetDifficultyScalar)
//   - spawn-table cascade inherits 1471, own 5000 rows win (LoadObjectConfig)
//   - SpawnTableComposites() / ExplodeOnDeathBotIds(), same mechanics as the
//     Super Agent lists of the same name. Authored in Hardcore.cpp.

class ATgPawn;

namespace Hardcore {

// True when this match runs under the Hardcore Security difficulty.
bool IsActive();

// Target spawn table id -> ordered source table ids, concatenated to rebuild
// the target. Same semantics as SuperAgent::SpawnTableComposites().
const std::map<int, std::vector<int>>& SpawnTableComposites();

// Bot ids that die with r_eDeathReason = DR_DESPAWN (no ragdoll). Same
// semantics as SuperAgent::ExplodeOnDeathBotIds().
const std::vector<int>& ExplodeOnDeathBotIds();

// Called from the TgPawn::TrackDeath hook for bot deaths.
void NotifyBotDeath(ATgPawn* Pawn);

}  // namespace Hardcore
