#pragma once

#include "src/pch.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// Win/loss for asymmetric objective-bot missions (Raid_DomeCityDefense_P: the
// Colony Dismantler boss + the defended NPC "Bancroft" — both are
// TgMissionObjective_Bots in m_MissionObjectives). The stock "all active
// objectives captured" test can't express this because the two objectives are
// owned by OPPOSITE task forces:
//   nDefaultOwnerTaskForce 1 = attacker-owned (the boss) — destroying it is the
//     defenders' goal → DEFENDERS win.
//   nDefaultOwnerTaskForce 2 = defender-owned (the NPC) — destroying it is the
//     attackers' goal → ATTACKERS win.
//
// "Destroyed" = r_eStatus == 8, the engine's canonical capture/death status.
// ServerCalcStatus sets it once when the bot pawn dies and it stays put — it
// survives the bot's m_bCaptureOnlyOnce self-deactivate and the r_ObjectiveBot
// pointer being nulled in ResetObjective, so we read the persistent status the
// engine already maintains instead of polling pawn health. A bot that has never
// died is status 7/6 (or never spawned), so a future-wave boss never counts as
// "all attacker objectives destroyed" early. (Requires StampObjective to seed a
// freshly-spawned bot at status 7, not 8 — see TgMissionObjective_Bot__SpawnObjectiveBot.)
//
// Returns 0 (no decision), 1 (defenders win), 2 (attackers win).
inline int ObjectiveBotOutcome(ATgRepInfo_Game* GRI) {
	if (GRI == nullptr) return 0;

	bool anyDefenderDestroyed = false;
	bool hasAttacker          = false;
	bool allAttackerDestroyed = true;

	for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
		ATgMissionObjective* O = GRI->m_MissionObjectives.Data[i];
		if (O == nullptr) continue;
		if (!ObjectClassCache::ClassNameContains((UObject*)O, "TgMissionObjective_Bot")) continue;
		ATgMissionObjective_Bot* B = (ATgMissionObjective_Bot*)O;

		// Destroyed = engine status says captured/killed (8), OR the bound pawn
		// is present and at <=0 HP (covers the window where the pawn is dying but
		// ServerCalcStatus hasn't latched 8 yet / can't, because it skips a null
		// r_ObjectiveBot). A never-spawned bot (null pawn, status 7/0) is alive.
		const bool destroyed = (B->r_eStatus == 8)
			|| (B->r_ObjectiveBot != nullptr && B->r_ObjectiveBot->Health <= 0);
		if (B->nDefaultOwnerTaskForce == 2) {       // defended NPC (Bancroft)
			if (destroyed) anyDefenderDestroyed = true;
		} else {                                    // attacker boss
			hasAttacker = true;
			if (!destroyed) allAttackerDestroyed = false;
		}
	}

	if (anyDefenderDestroyed)                 return 2; // protected NPC destroyed → loss
	if (hasAttacker && allAttackerDestroyed)  return 1; // boss destroyed → win
	return 0;
}
