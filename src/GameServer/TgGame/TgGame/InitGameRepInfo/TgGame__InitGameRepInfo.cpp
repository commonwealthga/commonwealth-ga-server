#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgGame__InitGameRepInfo::Call(ATgGame* Game, void* edx) {
	Logger::Log("debug", "MINE TgGame::InitGameRepInfo START\n");
	// LogToFile("C:\\mylog.txt", "MINE TgGame::InitGameRepInfo START");

	ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (gamerep != nullptr) {

		gamerep->GameClass = Game->Class;
		gamerep->r_GameType = Game->m_GameType;

		gamerep->r_bIsRaid = 0;
		gamerep->r_bIsMission = 1;
		gamerep->r_bIsPVP = 1;
		gamerep->r_bIsTraining = 0;
		gamerep->r_bIsTutorialMap = 0;
		gamerep->r_bIsArena = 1;
		gamerep->r_bIsMatch = 1;
		gamerep->r_bIsTerritoryMap = 0;
		gamerep->r_bIsOpenWorld = 0;
		gamerep->r_bAllowBuildMorale = 1;
		gamerep->r_bActiveCombat = 1;
		gamerep->r_bAllowPlayerRelease = 1;
		gamerep->r_bDefenseAlarm = 0;
		gamerep->r_bInOverTime = 0;
		gamerep->r_nSecsToAutoReleaseAttackers = Game->m_nSecsToAutoReleaseAttackers;
		gamerep->r_nSecsToAutoReleaseDefenders = Game->m_nSecsToAutoReleaseDefenders;
		gamerep->r_nReleaseDelay = 15;
		gamerep->r_nPointsToWin = 3;
		gamerep->r_nRoundNumber = 1;
		gamerep->r_nMaxRoundNumber = 3;

		// PreloadClasses();

		ATgRepInfo_TaskForce* attackers = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);
		ATgRepInfo_TaskForce* defenders = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);

		GTeamsData.Attackers = attackers;
		GTeamsData.Defenders = defenders;

		attackers->r_nTaskForce = 1;
		defenders->r_nTaskForce = 2;

		gamerep->Teams.Data[0] = attackers;
		gamerep->Teams.Data[1] = defenders;
		gamerep->Teams.Count = 2;

		attackers->eventPostInit();
		defenders->eventPostInit();

		gamerep->InitMissionTime();
	}
	Logger::Log("debug", "MINE TgGame::InitGameRepInfo END\n");
}

