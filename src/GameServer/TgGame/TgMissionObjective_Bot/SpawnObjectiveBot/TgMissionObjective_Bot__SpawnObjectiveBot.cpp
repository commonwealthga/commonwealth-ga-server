#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

ATgPawn* SpawnBotAtLocation(ATgGame* Game, int botId, FVector loc, FRotator rot) {
	float radius = 0.0f, halfHeight = 0.0f;
	TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
	if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
	// Bosses / escort targets spawn without a TgBotFactory but still want
	// the same per-bot BBM × per-difficulty HP+damage scaling as the rest of
	// the enemy roster. Raise the flag explicitly here; SpawnBotById would
	// not raise it for a null factory pointer.
	TgPawn__InitializeDefaultProps::bPendingEnemyScaling = true;
	return (ATgPawn*)Game->SpawnBotById(
		botId, loc, rot,
		/*bKillController=*/   false,
		/*pFactory=*/          nullptr,
		/*bIgnoreCollision=*/  true,
		/*pOwnerPawn=*/        nullptr,
		/*bIsDecoy=*/          false,
		/*deviceFire=*/        nullptr,
		/*fDeployAnimLength=*/ 0.0f);
}

void ApplyTaskForce(ATgPawn* Bot, int taskForce) {
	if (!Bot) return;
	ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	if (!BotRep) return;
	ATgRepInfo_TaskForce* team = (taskForce == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
	BotRep->r_TaskForce = team;
	BotRep->Team        = team;
	BotRep->SetTeam(team);
	Bot->NotifyTeamChanged();
}

void StampObjective(ATgMissionObjective_Bot* Obj, ATgPawn* Bot, int taskForce) {
	if (!Bot) return;
	Obj->Role               = 3;
	Obj->r_ObjectiveBot     = Bot;
	Obj->r_ObjectiveBotInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	// Activation (bEnabled + r_bIsActive) is handled canonically by
	// TgGame::PostBeginPlay → UnlockObjective(1) → eventUnlockObjective(1)
	// → SetObjectiveActive(true), which now resolves to our
	// TgMissionObjective_Bot__SetObjectiveActive hook (the subclass native
	// at 0x10a797c0 is a stub that would otherwise no-op the canonical path
	// and leave bEnabled=0 → UpdateObjectiveStatus gate blocks every status
	// flip → boss death never propagates to BeginEndMission). Order is
	// safe: actor PostBeginPlay runs first (boss spawned here), then game
	// PostBeginPlay fires UnlockObjective(1) — the bot is already stamped
	// by the time activation flips the flags.
	// A freshly-spawned bot is alive and intact → status 7. (ServerCalcStatus
	// re-derives this from health each tick once active.) Do NOT seed the
	// attacker boss at 8: r_eStatus==8 is the engine's "destroyed" status, and
	// the win-checks read it directly — a seeded 8 reads as "boss already dead"
	// the instant it spawns and would hand the defenders an instant round-5 win.
	Obj->r_eStatus = 7;
	if (taskForce == 2)
		GTeamsData.Attackers->r_CurrActiveObjective = Obj;  // attackers' goal: kill Bancroft
	else
		GTeamsData.Defenders->r_CurrActiveObjective = Obj;  // defenders' goal: kill the boss
}

}  // namespace

void __fastcall TgMissionObjective_Bot__SpawnObjectiveBot::Call(ATgMissionObjective_Bot* Obj, void* edx) {
	if (Obj == nullptr) return;
	const int mid = Obj->m_nMapObjectId;

	Logger::Log("tgmissionobjective_bot",
		"[%s] %s SpawnObjectiveBot mapObjectId=%d s_nBotId=%d s_nSpawnTableId=%d "
		"defaultOwnerTaskForce=%d\n",
		Logger::GetTime(), Obj->GetName(), mid,
		Obj->s_nBotId, Obj->s_nSpawnTableId, Obj->nDefaultOwnerTaskForce);

	int botId = 0;

	// Resolution priority (all fields already populated on the actor by
	// TgMissionObjective_Bot__LoadObjectConfig):
	//   1. s_nBotId       > 0 → spawn that exact bot.
	//   2. s_nSpawnTableId > 0 → use the shared helper to build a queue, pick
	//      the first entry, resolve the bot for that (table, group) via
	//      PickBotFromSpawnTableGroup. Boss tables conventionally have one
	//      group, so "first entry" is unambiguous; multi-group tables fall
	//      back to whatever the std::map iteration order yields, which is
	//      lowest group number.
	if (Obj->s_nBotId > 0) {
		botId = Obj->s_nBotId;
	} else if (Obj->s_nSpawnTableId > 0) {
		// Boss tables conventionally have one group — pick from the first.
		const int firstGroup = TgBotFactory__LoadObjectConfig::GetGroupNumberByIndex(
			Obj->s_nSpawnTableId, 0);
		if (firstGroup >= 0) {
			botId = TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(
				Obj->s_nSpawnTableId, firstGroup);
		}
		if (botId == 0) {
			Logger::Log("tgmissionobjective_bot",
				"  s_nSpawnTableId=%d yielded no bot at current difficulty\n",
				Obj->s_nSpawnTableId);
		}
	} else {
		Logger::Log("tgmissionobjective_bot",
			"  no resolution: s_nBotId=0 AND s_nSpawnTableId=0\n");
	}

	if (botId <= 0) return;

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	ATgPawn* Bot = SpawnBotAtLocation(Game, botId, Obj->Location, Obj->Rotation);
	if (Bot == nullptr) {
		Logger::Log("tgmissionobjective_bot",
			"  SpawnBotById returned null for bot=%d\n", botId);
		return;
	}

	// Team for the spawned bot comes from parent's existing UC field
	// (TgMissionObjective.nDefaultOwnerTaskForce, default 2). No synthetic
	// column, no side map.
	ApplyTaskForce(Bot, Obj->nDefaultOwnerTaskForce);
	StampObjective(Obj, Bot, Obj->nDefaultOwnerTaskForce);

	// Designer-set retreat nav point — same copy retail's native spawn path
	// did for factory bots; gates the PANIC retreat actions via test 253.
	if (Bot->Controller != nullptr) {
		ATgAIController* AIC = (ATgAIController*)Bot->Controller;
		if (Obj->SafetyLocation != nullptr) {
			AIC->m_pSafetyLocation = Obj->SafetyLocation;
			Logger::Log("panic",
				"SpawnObjectiveBot: bot=%d safety location set from objective %d\n",
				botId, mid);
		}
		// Alarm group id for RadioAlarm (action 620) — bosses summoning adds.
		AIC->m_nGlobalAlarmId = Obj->nGlobalAlarmId;
	}

	// TEMP TEST CHEAT — REMOVE BEFORE SHIPPING. Defended NPCs are unkillable
	// so the 10-player missions can be soloed: Bancroft (dome, mapObjectId
	// 13623), the Backup Generator (Canyon_Defense00, 13282), the Mobile
	// Power Unit (Moving_Target00, 13332) and the Backup Generator / Cage of
	// Souls (Oasis_Checkpoint / Raid_Halloween_Oasis_P, both 12599).
	// Mirrors the player godmode in TgGame::SpawnPlayerCharacter.
	if ((mid == 13623 || mid == 13282 || mid == 13332 || mid == 12599) && Bot->Controller != nullptr) {
		// Bot->Controller->bGodMode = 1;
		Logger::Log("tgmissionobjective_bot", "  TEMP CHEAT: godmode enabled on defended NPC (mid=%d)\n", mid);
	}

	Logger::Log("tgmissionobjective_bot",
		"  spawned objective bot %d successfully (team from nDefaultOwnerTaskForce=%d)\n",
		botId, Obj->nDefaultOwnerTaskForce);
}
