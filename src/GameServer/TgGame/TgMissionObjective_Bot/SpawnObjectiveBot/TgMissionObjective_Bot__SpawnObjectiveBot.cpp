#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

ATgPawn* SpawnBotAtLocation(ATgGame* Game, int botId, FVector loc, FRotator rot) {
	float radius = 0.0f, halfHeight = 0.0f;
	TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
	if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
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
	Obj->r_bIsActive        = 1;
	if (taskForce == 2){
		Obj->r_eStatus = 7;
		GTeamsData.Attackers->r_CurrActiveObjective = Obj;
	} else {
		Obj->r_eStatus = 8;
		GTeamsData.Defenders->r_CurrActiveObjective = Obj;
	}
	// Obj->r_eStatus          = 7; //3;
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
		const auto queue = TgBotFactory__LoadObjectConfig::CreateRandomSpawnQueue(Obj->s_nSpawnTableId);
		if (!queue.empty()) {
			const FSpawnQueueEntry& first = queue.front();
			botId = TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(
				first.nSpawnTableId, first.nGroupNumber);
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

	Logger::Log("tgmissionobjective_bot",
		"  spawned objective bot %d successfully (team from nDefaultOwnerTaskForce=%d)\n",
		botId, Obj->nDefaultOwnerTaskForce);
}
