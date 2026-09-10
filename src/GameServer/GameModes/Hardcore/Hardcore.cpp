#include "src/GameServer/GameModes/Hardcore/Hardcore.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Utils/Logger/Logger.hpp"

#include <algorithm>

namespace Hardcore {

bool IsActive() {
	return Config::GetDifficultyValueId() == GA_G::DIFFICULTY_VALUE_ID_CUSTOM_HARDCORE_SECURITY;
}

// ============================================================================
//  COMPOSITE SPAWN TABLES
// ============================================================================
// REBUILD a spawn table by concatenating the groups of other tables. List a
// table as its own source to keep its original groups (e.g. { 34, { 34, 100 } }).
// Sources always expand to their ORIGINAL rows. Super Agent's live set, for
// reference (SuperAgentMission.cpp):
//   { 34,  { 34, 100, 101, 104, 148 } },      // support enemies
//   { 28,  { 28, 167, 157, 148 } },           // normal spawns
//   { 29,  { 29, 29, 212, 212, 72, 210 } },   // first spawn
//   { 102, { 33, 102, 102 } },                // small group of responders
//   { 33,  { 33, 102, 102, 33, 102, 102 } },  // large group of responders
//   { 40,  { 40, 40, 100, 101, 104, 148 } },  // support enemies
//   { 58,  { 58, 58, 167, 157 } },            // normal spawns + guardian
//
//   target table -> { source tables, in append order }
const std::map<int, std::vector<int>>& SpawnTableComposites() {
	static const std::map<int, std::vector<int>> composites = {
		{ 34,  { 34, 100, 101, /*104,*/ 148 } },      // support enemies
		{ 28,  { 28, 167, 157, 148 } },           // normal spawns
		{ 29,  { 29, 29, 212, 212, 72/*, 210*/ } },   // first spawn
		{ 102, { 33, 102 } },                // small group of responders
		{ 33,  { 33, 102, 102 } },  // large group of responders
		{ 40,  { 40, 40, 100, 101, /*104,*/ 148 } },  // support enemies
		{ 58,  { 58, 58, 167, 157 } },            // normal spawns + guardian

		// dweller maps
		{78, { 78, 167, 157, 148 } },
		{79, { 79, 100, 101, 148 } },
		{80, { 80, 80, 80} },
		{81, { 81, 81, 81, 158 } },
		{82, { 82, 82, 82, 171 } },

		// colony maps
		{85, {85, 85} },
		{98, {98, 98} },
		{99, {99, 99, 167, 157, 148} },
		{100, {100, 100} },
		{101, {101, 101} },
		{102, {102, 102} },
		{147, {147, 147} },
		{148, {148, 148} },
		{162, {162, 162} },
		{167, {167, 167, 100, 101, 148} },
		{168, {168, 168} },
		{170, {170, 170} },
		{182, {182, 182} },
		{183, {183, 183} },
		{184, {184, 184} },
		{153, {153, 153, 102, 102} },
		{85, {85, 85, 102, 102, 102, 102} },
	};
	return composites;
}

// ============================================================================
//  EXPLODE-ON-DEATH BOTS (ragdoll suppression)
// ============================================================================
// Bots listed here die with r_eDeathReason = DR_DESPAWN: 'Despawned' FX +
// instant mesh hide instead of a ragdolled corpse. Perf knob for big waves.
const std::vector<int>& ExplodeOnDeathBotIds() {
	static const std::vector<int> ids = {
		1335,   // Alarm Responder (table 33)
		1349,   // Alarm Responder (table 33)
		1458,   // Sand Spider     (table 102) — mesh has no 'Despawned' FX, vanishes without a flash
		1468,   // Colony Drone    (table 102)
		1492,   // Colony Soldier  (table 102)
	};
	return ids;
}

// Same stamp as SuperAgent::NotifyBotDeath — runs inside TgPawn.Died so it
// replicates in the same bunch as PlayDying.
void NotifyBotDeath(ATgPawn* Pawn) {
	if (!IsActive() || !Pawn || !Pawn->r_bIsBot) return;
	const std::vector<int>& ids = ExplodeOnDeathBotIds();
	if (std::find(ids.begin(), ids.end(), Pawn->r_nProfileId) == ids.end()) return;
	Pawn->r_eDeathReason = 1;  // TG_DEATH_REASON DR_DESPAWN
	Logger::Log("hardcore", "explode-on-death: bot %d (pawn %d) death reason -> despawn\n",
		Pawn->r_nProfileId, Pawn->r_nPawnId);
}

}  // namespace Hardcore
