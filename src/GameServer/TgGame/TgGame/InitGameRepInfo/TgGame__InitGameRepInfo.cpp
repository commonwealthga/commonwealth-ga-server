#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/DebugWindow/DebugWindow.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgGame__InitGameRepInfo::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	// LogToFile("C:\\mylog.txt", "MINE TgGame::InitGameRepInfo START");

	Globals::Get().GGameInfo = (void*)Game;

	ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);

	if (gamerep != nullptr) {


		// for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		// 	if (UObject::GObjObjects()->Data[i]) {
		// 		UObject* obj = UObject::GObjObjects()->Data[i];
		// 		if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0 && !strstr(obj->GetFullName(), "Default__")) {
		// 			ATgRandomSMManager* RandomSMManager = reinterpret_cast<ATgRandomSMManager*>(obj);
		//
		// 			DebugWindow::Instances["RandomSMManager"] = {
		// 				.address = (uintptr_t)RandomSMManager,
		// 				.type = "RandomSMManager"
		// 			};
		// 			break;
		// 		}
		// 	}
		// }

		DebugWindow::Instances["GameReplicationInfo"] = {
			.address = (uintptr_t)gamerep,
			.type = "GameReplicationInfo"
		};
		DebugWindow::Instances["Game"] = {
			.address = (uintptr_t)Game,
			.type = "Game"
		};

		DebugWindow::RefreshOptions();

		// Per-map overrides (v95/v97). When map_game_info has a row for the
		// current map: mission_time_secs replaces the legacy `15 * 60` baked
		// into every per-class block; overtime_secs + allow_overtime replace
		// the legacy 4-min/allowed default; is_pvp overrides the class-derived
		// r_bIsPVP AFTER all class blocks run. Maps absent from map_game_info
		// fall back to the historical hardcoded behavior.
		const std::string mapName = Config::GetMapNameChar();
		const auto mapRow = MapGameInfo::LookupByName(mapName);
		const int  missionTimeSecs = mapRow ? mapRow->mission_time_secs : 15 * 60;
		const int  overtimeSecs    = mapRow ? mapRow->overtime_secs     : 4 * 60;
		const bool allowOvertime   = mapRow ? mapRow->allow_overtime    : true;

		gamerep->GameClass = Game->Class;
		gamerep->r_GameType = Game->m_GameType;

		gamerep->r_bIsRaid = 0;
		gamerep->r_bIsMission = 1;
		gamerep->r_bIsPVP = 0;
		gamerep->r_bIsTraining = 0;
		gamerep->r_bIsTutorialMap = 0;
		gamerep->r_bIsArena = 0;
		// r_bIsMatch = 1 by default — gates the HUD team-panel left-side
		// teammate list (TgUIPrimaryHUD_TeamPanel TickPrimaryHUDElement walks
		// LocalPRI->r_TaskForce->m_TeamPlayers iff (GRI+0x264 >> 6) & 1 is set,
		// i.e. r_bIsMatch. Open-world / city modes below explicitly clear it.
		gamerep->r_bIsMatch = 1;
		gamerep->r_bIsTerritoryMap = 0;
		gamerep->r_bIsOpenWorld = 0;
		gamerep->r_bAllowBuildMorale = 1;
		gamerep->r_bActiveCombat = 1;
		gamerep->r_bAllowPlayerRelease = 1;
		gamerep->r_bDefenseAlarm = 1;
		gamerep->r_bInOverTime = 0;
		gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
		gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
		gamerep->r_nReleaseDelay = 1;
		gamerep->r_nPointsToWin = 3;
		gamerep->r_nRoundNumber = 1;
		gamerep->r_nMaxRoundNumber = 5;


		Game->m_fGameMissionTime  = static_cast<float>(missionTimeSecs);
		Game->m_fGameOvertimeTime = static_cast<float>(overtimeSecs);
		Game->m_bAllowOvertime    = allowOvertime ? 1 : 0;
		Game->m_eTimerState = 0;
		Game->TimeLimit = missionTimeSecs;
		gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;

		gamerep->TimeLimit = Game->TimeLimit;
		gamerep->RemainingTime = Game->TimeLimit;

		// If objectives weren't self-registered via AddToList/AddObjectivePointToList,
		// use cached actors to add them manually.
		if (gamerep->m_MissionObjectives.Count == 0) {

			Logger::Log(GetLogChannel(), "GRI->m_MissionObjectives is empty, populating manually\n");

			using AddObjectivePointToList_t = void(__fastcall*)(ATgRepInfo_Game*, void*, ATgMissionObjective*);
			auto AddObjectivePointToList = reinterpret_cast<AddObjectivePointToList_t>(0x109f0580);

			ActorCache::CacheMapActors();

			for (ATgMissionObjective* Objective : ActorCache::MissionObjectives) {
				AddObjectivePointToList(gamerep, nullptr, Objective);
				if (Logger::IsChannelEnabled(GetLogChannel())) {
					const std::string objectiveName = ((UObject*)Objective)->GetFullName();
					Logger::Log(GetLogChannel(), "Added objective %s to GRI->m_MissionObjectives\n", objectiveName.c_str());
				}
			}
		}


		const std::string GameClassName = Game->Class->GetFullName();
		if (Logger::IsChannelEnabled("gametimer")) {
			const std::string gameName = ((UObject*)Game)->GetFullName();
			const std::string stateName = Game->GetStateName().GetName();
			Logger::Log("gametimer",
				"[InitGameRepInfo:pre-class-config] game=%s class=%s state=%s wait=%d delayed=%d ended=%d "
				"timerState=%d mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f "
				"GRI{round=%d/%d mtState=%d mtChange=%d rem=%.2f remaining=%d limit=%d flags raid=%d mission=%d arena=%d match=%d}\n",
				gameName.c_str(),
				GameClassName.c_str(),
				stateName.c_str(),
				(int)Game->bWaitingToStartMatch,
				(int)Game->bDelayedStart,
				(int)Game->bGameEnded,
				(int)Game->m_eTimerState,
				Game->m_fMissionTime,
				Game->m_fGameMissionTime,
				Game->m_fGameOvertimeTime,
				Game->s_fMissionTimerStartedAt,
				gamerep->r_nRoundNumber,
				gamerep->r_nMaxRoundNumber,
				(int)gamerep->r_nMissionTimerState,
				gamerep->r_nMissionTimerStateChange,
				gamerep->r_fMissionRemainingTime,
				gamerep->RemainingTime,
				gamerep->TimeLimit,
				(int)gamerep->r_bIsRaid,
				(int)gamerep->r_bIsMission,
				(int)gamerep->r_bIsArena,
				(int)gamerep->r_bIsMatch);
		}

		if (GameClassName == "Class TgGame.TgGame_Defense") {
			gamerep->r_bIsRaid = 1;
			gamerep->r_bIsMission = 1;
			gamerep->r_bIsPVP = 0;
			gamerep->r_bIsTraining = 0;
			gamerep->r_bIsTutorialMap = 0;
			gamerep->r_bIsArena = 0;
			gamerep->r_bIsMatch = 1;
			gamerep->r_bIsTerritoryMap = 0;
			gamerep->r_bIsOpenWorld = 0;
			gamerep->r_bAllowBuildMorale = 1;
			gamerep->r_bActiveCombat = 1;
			gamerep->r_bAllowPlayerRelease = 1;
			gamerep->r_bDefenseAlarm = 0;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 3;
			gamerep->r_nRoundNumber = 0;
			gamerep->r_nMaxRoundNumber = 5;

			ATgGame_Defense* GameDef = (ATgGame_Defense*)Game;

			Game->m_fGameMissionTime = 0;
			Game->m_fMissionTime = 0;
			Game->m_fGameOvertimeTime = 0;
			Game->m_bAllowOvertime = 0;
			Game->m_bShouldWait = 0;

			// Game->TimeLimit = 210;


// function float GetSetupTime()
// {
//     // End:0x3B
//     if(TgRepInfo_Game(GameReplicationInfo).IsPvEMission())
//     {
//         // End:0x32
//         if(int(m_GameType) == int(11))
//         {
//             return 30.0000000;
//         }
//         return 15.0000000;        
//     }
//     else
//     {
//         // End:0x4E
//         if(IsTerritory())
//         {
//             return 120.0000000;
//         }
//     }
//     return 60.0000000;
//     //return ReturnValue;    
// }


			GameDef->s_nMaxRoundNumber = 5;
			GameDef->s_nRoundSetupTime = 0;
			GameDef->s_nBetweenRoundDelay = 30;
			GameDef->s_nRoundNumber = 0;
			GameDef->s_fRoundDuration = 210; // 3 minutes 30 seconds
			GameDef->m_fGameMissionTime = 0;

			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;

			if (Logger::IsChannelEnabled("gametimer")) {
				const std::string gameName = ((UObject*)Game)->GetFullName();
				const std::string stateName = Game->GetStateName().GetName();
				Logger::Log("gametimer",
					"[InitGameRepInfo:defense-config] game=%s class=%s state=%s wait=%d delayed=%d ended=%d "
					"timerState=%d mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f "
					"Arena{round=%d setup=%d between=%d objectiveUnlock=%d} Defense{maxRound=%d duration=%.2f} "
					"GRI{round=%d/%d mtState=%d mtChange=%d rem=%.2f remaining=%d limit=%d flags raid=%d mission=%d arena=%d match=%d defenseAlarm=%d}\n",
					gameName.c_str(),
					GameClassName.c_str(),
					stateName.c_str(),
					(int)Game->bWaitingToStartMatch,
					(int)Game->bDelayedStart,
					(int)Game->bGameEnded,
					(int)Game->m_eTimerState,
					Game->m_fMissionTime,
					Game->m_fGameMissionTime,
					Game->m_fGameOvertimeTime,
					Game->s_fMissionTimerStartedAt,
					GameDef->s_nRoundNumber,
					GameDef->s_nRoundSetupTime,
					GameDef->s_nBetweenRoundDelay,
					GameDef->s_nObjectiveUnlockDelay,
					GameDef->s_nMaxRoundNumber,
					GameDef->s_fRoundDuration,
					gamerep->r_nRoundNumber,
					gamerep->r_nMaxRoundNumber,
					(int)gamerep->r_nMissionTimerState,
					gamerep->r_nMissionTimerStateChange,
					gamerep->r_fMissionRemainingTime,
					gamerep->RemainingTime,
					gamerep->TimeLimit,
					(int)gamerep->r_bIsRaid,
					(int)gamerep->r_bIsMission,
					(int)gamerep->r_bIsArena,
					(int)gamerep->r_bIsMatch,
					(int)gamerep->r_bDefenseAlarm);
			}
		}

		if (GameClassName == "Class TgGame.TgGame_PointRotation") {
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
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 3;
			gamerep->r_nRoundNumber = 1;
			gamerep->r_nMaxRoundNumber = 5;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;
			Game->TimeLimit = missionTimeSecs;

			// PointRotation UC default is 30s between rounds; we shorten to 20s
			// to match the original feel. Pairs with ROTATION_BANNER_LEAD_SECS=17
			// in MissionAlerts.cpp so "Point Changing" fires at t=3, then
			// 15/10/5 countdowns at t=5/10/15, activation at t=20. Must be set
			// before TgGame_Arena.RoundInProgress::BeginState runs (which is
			// where `SetTimer(s_nObjectiveUnlockDelay, false, 'ObjectiveUnlock')`
			// fires). InitGameRepInfo runs before PostBeginPlay, so we're early.
			((ATgGame_Arena*)Game)->s_nObjectiveUnlockDelay = 20;
		}

		if (GameClassName == "Class TgGame.TgGame_Mission") {
			gamerep->r_bIsRaid = 0;
			gamerep->r_bIsMission = 1;
			gamerep->r_bIsPVP = 0;
			gamerep->r_bIsTraining = 0;
			gamerep->r_bIsTutorialMap = 0;
			gamerep->r_bIsArena = 0;
			gamerep->r_bIsMatch = 1;  // enables HUD team-panel teammate list
			gamerep->r_bIsTerritoryMap = 0;
			gamerep->r_bIsOpenWorld = 0;
			gamerep->r_bAllowBuildMorale = 1;
			gamerep->r_bActiveCombat = 1;
			gamerep->r_bAllowPlayerRelease = 1;
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 3;
			gamerep->r_nRoundNumber = 1;
			// gamerep->r_nMaxRoundNumber = 5;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;
			Game->TimeLimit = missionTimeSecs;

			if (mapName == "Inception_ALL" || mapName == "Inception_3_TEMP" || mapName == "Adrenaline_P" || mapName == "Skylark_P" || mapName == "AgencyZero_P") {
				gamerep->r_bIsTutorialMap = 1;
				gamerep->r_bIsPVP = 0;
			}
		}

		if (GameClassName == "Class TgGame.TgGame_City") {
			gamerep->r_bIsRaid = 0;
			gamerep->r_bIsMission = 0;
			gamerep->r_bIsPVP = 0;
			gamerep->r_bIsTraining = 0;
			gamerep->r_bIsTutorialMap = 0;
			gamerep->r_bIsArena = 0;
			gamerep->r_bIsMatch = 0;
			gamerep->r_bIsTerritoryMap = 0;
			gamerep->r_bIsOpenWorld = 1;
			gamerep->r_bAllowBuildMorale = 0;
			gamerep->r_bActiveCombat = 0;
			gamerep->r_bAllowPlayerRelease = 1;
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 0;
			gamerep->r_nRoundNumber = 0;
			gamerep->r_nMaxRoundNumber = 0;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
		}

		if (GameClassName == "Class TgGame.TgGame_OpenWorldPVE") {
			gamerep->r_bIsRaid = 0;
			gamerep->r_bIsMission = 0;
			gamerep->r_bIsPVP = 0;
			gamerep->r_bIsTraining = 0;
			gamerep->r_bIsTutorialMap = 0;
			gamerep->r_bIsArena = 0;
			gamerep->r_bIsMatch = 0;
			gamerep->r_bIsTerritoryMap = 0;
			gamerep->r_bIsOpenWorld = 1;
			gamerep->r_bAllowBuildMorale = 1;
			gamerep->r_bActiveCombat = 1;
			gamerep->r_bAllowPlayerRelease = 1;
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 0;
			gamerep->r_nRoundNumber = 0;
			gamerep->r_nMaxRoundNumber = 0;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
		}

		if (GameClassName == "Class TgGame.TgGame_Escort") {
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
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 3;
			gamerep->r_nRoundNumber = 1;
			gamerep->r_nMaxRoundNumber = 3;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;
			Game->TimeLimit = missionTimeSecs;
		}

		if (GameClassName == "Class TgGame.TgGame_Ticket") {
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
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 800;  // also set by TgGame_Ticket::LoadGameConfig
			gamerep->r_nRoundNumber = 1;
			gamerep->r_nMaxRoundNumber = 3;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;
			Game->TimeLimit = missionTimeSecs;
		}

		if (GameClassName == "Class TgGame.TgGame_DualCTF") {
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
			gamerep->r_bDefenseAlarm = 1;
			gamerep->r_bInOverTime = 0;
			gamerep->r_nSecsToAutoReleaseAttackers = 1;// Game->m_nSecsToAutoReleaseAttackers;
			gamerep->r_nSecsToAutoReleaseDefenders = 1;//Game->m_nSecsToAutoReleaseDefenders;
			gamerep->r_nReleaseDelay = 1;
			gamerep->r_nPointsToWin = 3;
			gamerep->r_nRoundNumber = 1;
			gamerep->r_nMaxRoundNumber = 3;
			gamerep->r_fMissionRemainingTime = Game->m_fMissionTime;
			gamerep->TimeLimit = Game->m_fMissionTime;
			gamerep->RemainingTime = Game->m_fMissionTime;
			Game->TimeLimit = missionTimeSecs;
		}

		// Per-map PvP override (v95). map_game_info.is_pvp wins over the
		// class-derived default set in the per-class blocks above. The tutorial
		// special-case inside TgGame_Mission already forced r_bIsPVP=0 for
		// Inception / Adrenaline / Skylark / AgencyZero — preserve that by
		// only applying the DB override when the map isn't a tutorial.
		if (mapRow && !gamerep->r_bIsTutorialMap) {
			gamerep->r_bIsPVP = mapRow->is_pvp ? 1 : 0;
		}

		// PreloadClasses();

		ATgRepInfo_TaskForce* attackers = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);
		ATgRepInfo_TaskForce* defenders = (ATgRepInfo_TaskForce*)gamerep->Spawn(ClassPreloader::GetTgRepInfoTaskForceClass(), gamerep, FName(), FVector(0, 0, 0), FRotator(0, 0, 0), nullptr, 1);
		if (!attackers || !defenders) {
			Logger::Log("debug", "InitGameRepInfo: failed to Spawn task forces (attackers=%p defenders=%p) — class not preloaded?\n", attackers, defenders);
			return;
		}

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

		// defenders->NetPriority = 1;
		// defenders->NetUpdateFrequency = 0.5;
		// defenders->bNetInitial = 1;
		// defenders->bNetDirty = 1;
		// defenders->bForceNetUpdate = 1;
		// defenders->bOnlyDirtyReplication = 0;
		// defenders->bSkipActorPropertyReplication = 0;
		// defenders->bReplicateMovement = 0;
		// defenders->bReplicateRigidBodyLocation = 0;
		// defenders->bReplicateInstigator = 0;
		//
		// attackers->NetPriority = 1;
		// attackers->NetUpdateFrequency = 0.5;
		// attackers->bNetInitial = 1;
		// // attackers->bNetDirty = 1;
		// // attackers->bForceNetUpdate = 1;
		// attackers->bOnlyDirtyReplication = 0;
		// attackers->bSkipActorPropertyReplication = 0;
		// attackers->bReplicateMovement = 0;
		// attackers->bReplicateRigidBodyLocation = 0;
		// attackers->bReplicateInstigator = 0;

		// gamerep->bNetInitial = 1;
		// gamerep->bNetDirty = 1;
		// gamerep->bForceNetUpdate = 1;
		// gamerep->bOnlyDirtyReplication = 0;
		// gamerep->bSkipActorPropertyReplication = 0;
		// gamerep->bReplicateMovement = 0;
		// gamerep->bReplicateRigidBodyLocation = 0;
		// gamerep->NetPriority = 5;
		// gamerep->NetUpdateFrequency = 5;

		// gamerep->r_nMissionTimerState
		// gamerep->bReplicateInstigator = 0;

	}

	LogCallEnd();
}
