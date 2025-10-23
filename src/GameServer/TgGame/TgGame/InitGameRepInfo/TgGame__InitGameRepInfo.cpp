#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgGame__InitGameRepInfo::Call(ATgGame* Game, void* edx) {
	Logger::Log("debug", "MINE TgGame::InitGameRepInfo START\n");
	// LogToFile("C:\\mylog.txt", "MINE TgGame::InitGameRepInfo START");

	Globals::Get().GGameInfo = (void*)Game;

	ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (gamerep != nullptr) {

		gamerep->GameClass = Game->Class;
		gamerep->r_GameType = Game->m_GameType;

		gamerep->r_bIsRaid = 1;
		gamerep->r_bIsMission = 1;
		gamerep->r_bIsPVP = 0;
		gamerep->r_bIsTraining = 0;
		gamerep->r_bIsTutorialMap = 0;
		gamerep->r_bIsArena = 0;
		gamerep->r_bIsMatch = 0;
		gamerep->r_bIsTerritoryMap = 0;
		gamerep->r_bIsOpenWorld = 0;
		gamerep->r_bAllowBuildMorale = 1;
		gamerep->r_bActiveCombat = 1;
		gamerep->r_bAllowPlayerRelease = 1;
		gamerep->r_bDefenseAlarm = 1;
		gamerep->r_bInOverTime = 0;
		gamerep->r_nSecsToAutoReleaseAttackers = 5;// Game->m_nSecsToAutoReleaseAttackers;
		gamerep->r_nSecsToAutoReleaseDefenders = 5;//Game->m_nSecsToAutoReleaseDefenders;
		gamerep->r_nReleaseDelay = 3;
		gamerep->r_nPointsToWin = 3;
		gamerep->r_nRoundNumber = 1;
		gamerep->r_nMaxRoundNumber = 5;
		gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;

		// PreloadClasses();

		ATgRepInfo_TaskForce* attackers = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);
		ATgRepInfo_TaskForce* defenders = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);

		GTeamsData.Attackers = attackers;
		GTeamsData.Defenders = defenders;

		attackers->TeamIndex = 1;
		attackers->r_nTaskForce = 1;
		defenders->TeamIndex = 2;
		defenders->r_nTaskForce = 2;

		gamerep->SetTeam(1, attackers);
		gamerep->SetTeam(2, defenders);

		// gamerep->Teams.Data[0] = none;
		// gamerep->Teams.Data[1] = attackers;
		// gamerep->Teams.Data[2] = defenders;
		// gamerep->Teams.Count = 3;

		defenders->eventPostInit();
		attackers->eventPostInit();

		defenders->bNetInitial = 1;
		defenders->bNetDirty = 1;
		defenders->bForceNetUpdate = 1;
		defenders->bOnlyDirtyReplication = 0;
		defenders->bSkipActorPropertyReplication = 0;
		defenders->bReplicateMovement = 0;
		defenders->bReplicateRigidBodyLocation = 0;
		defenders->bReplicateInstigator = 0;

		attackers->bNetInitial = 1;
		attackers->bNetDirty = 1;
		attackers->bForceNetUpdate = 1;
		attackers->bOnlyDirtyReplication = 0;
		attackers->bSkipActorPropertyReplication = 0;
		attackers->bReplicateMovement = 0;
		attackers->bReplicateRigidBodyLocation = 0;
		attackers->bReplicateInstigator = 0;

		gamerep->bNetInitial = 1;
		gamerep->bNetDirty = 1;
		gamerep->bForceNetUpdate = 1;
		gamerep->bOnlyDirtyReplication = 0;
		gamerep->bSkipActorPropertyReplication = 0;
		gamerep->bReplicateMovement = 0;
		gamerep->bReplicateRigidBodyLocation = 0;

		// gamerep->r_nMissionTimerState
		// gamerep->bReplicateInstigator = 0;

	}
	Logger::Log("debug", "MINE TgGame::InitGameRepInfo END\n");
}

