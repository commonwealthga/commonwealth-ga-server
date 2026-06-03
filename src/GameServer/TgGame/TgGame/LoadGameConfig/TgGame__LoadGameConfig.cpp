#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool TgGame__LoadGameConfig::bRandomSMSettingsLoaded = false;
std::vector<std::string> TgGame__LoadGameConfig::m_randomSMSettings;

/**

Breach = TgGame_Mission
Payload = TgGame_Escort
Scramble = TgGame_PointRotation
Control = TgGame_Ticket
Dome = TgGame_City
Desert = TgGame_OpenWorldPVE
Raid = TgGame_Defense
VR = TgGame_Mission
Double agent = TgGame_Mission
Umax = TgGame_Mission
Acquisition = TgGame_DualCTF
AvA theft = TgGame_Escort

*/

void __fastcall TgGame__LoadGameConfig::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	if (Logger::IsChannelEnabled("gametimer")) {
		const std::string gameName = ((UObject*)Game)->GetFullName();
		const std::string gameClass = Game->Class ? Game->Class->GetFullName() : "<null-class>";
		const std::string stateName = Game->GetStateName().GetName();
		Logger::Log("gametimer",
			"[LoadGameConfig:before] game=%s class=%s state=%s wait=%d delayed=%d ended=%d "
			"timerState=%d mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f timeLimit=%d\n",
			gameName.c_str(),
			gameClass.c_str(),
			stateName.c_str(),
			(int)Game->bWaitingToStartMatch,
			(int)Game->bDelayedStart,
			(int)Game->bGameEnded,
			(int)Game->m_eTimerState,
			Game->m_fMissionTime,
			Game->m_fGameMissionTime,
			Game->m_fGameOvertimeTime,
			Game->s_fMissionTimerStartedAt,
			Game->TimeLimit);
	}
	// Keep the stock GameInfo.PostLogin path alive:
	// bWaitingToStartMatch && !bDelayedStart makes super.PostLogin call
	// StartMatch(), and TgGame.StartMatch enters GameRunning before
	// TgGame.PostLogin calls StartGameTimer().
	Game->bWaitingToStartMatch = 1;

	Game->s_UseCustomReviveTimer = 0;
	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;
	Game->m_bIsTutorialMap = 0;

	// Mission time + overtime are per-map via map_game_info (v95/v97). Falls
	// back to the historical 15-min mission / 4-min overtime / allowed default
	// if no row exists for the current map.
	std::string map_name = Config::GetMapNameChar();
	int  missionTimeSecs = 15 * 60;
	int  overtimeSecs    = 4 * 60;
	bool allowOvertime   = true;
	if (auto row = MapGameInfo::LookupByName(map_name)) {
		missionTimeSecs = row->mission_time_secs;
		overtimeSecs    = row->overtime_secs;
		allowOvertime   = row->allow_overtime;
	}
	Game->m_fGameMissionTime  = static_cast<float>(missionTimeSecs);
	Game->m_fGameOvertimeTime = static_cast<float>(overtimeSecs);
	Game->m_bAllowOvertime    = allowOvertime ? 1 : 0;
	Game->m_eTimerState = 0;

	Game->TimeLimit = missionTimeSecs;

	// Engine-side GameInfo.GameDifficulty (float, UE3 default 1.0). Read by
	// UC at TgPawn_Character.uc:754 (wall-jump noise gate, fires only when
	// > 2.0) and TgInventoryManager.uc:125 (weapon-switch noise volume scales
	// linearly). Map the active difficulty_value_id to the 1.0–3.0 range so
	// AI hearing actually responds to the Ultra-Max tier.
	Game->GameDifficulty = Config::GetDifficultyScalar();

	if (map_name == "Dome3_VR_Arena_P") {
		// Auto-resetting "infinite" mission timer. PollMissionTimer re-arms
		// 'MissionTimer' via eventMissionTimerStart() right after the 60s
		// alert fires. Disable overtime so the state machine never tries the
		// 2->3 transition if our reset is a tick late. Cycle length comes from
		// map_game_info.mission_time_secs above.
		Game->m_bAllowOvertime    = 0;
		Game->m_fGameOvertimeTime = 0.0f;
	}

	if (map_name == "Push_Dust_P") {
		// UObject__CollectGarbage::bDisableGarbageCollection = true;
	}

	LoadCommonGameConfig(Game);

	// Hard-code PlayerCountTarget=1 on every TgPlayerCountVolume in the map so
	// the threshold fires on the first qualifying touch. Placeholder until a
	// dynamic resolver (e.g. scale with online player count) lands. Volumes
	// are cached lazily by ActorCache on first call — calling here ensures the
	// world is scanned before any volume Touch can fire its native Update hook.
	ActorCache::CacheMapActors();
	for (ATgPlayerCountVolume* v : ActorCache::PlayerCountVolumes) {
		if (!v) continue;
		v->PlayerCountTarget = 1;
	}
	Logger::Log("playercount",
		"LoadGameConfig: forced PlayerCountTarget=1 on %d TgPlayerCountVolume(s)\n",
		(int)ActorCache::PlayerCountVolumes.size());

	if (Logger::IsChannelEnabled("gametimer")) {
		const std::string gameName = ((UObject*)Game)->GetFullName();
		const std::string gameClass = Game->Class ? Game->Class->GetFullName() : "<null-class>";
		const std::string stateName = Game->GetStateName().GetName();
		Logger::Log("gametimer",
			"[LoadGameConfig:after] game=%s class=%s state=%s wait=%d delayed=%d ended=%d "
			"timerState=%d mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f timeLimit=%d map=%s\n",
			gameName.c_str(),
			gameClass.c_str(),
			stateName.c_str(),
			(int)Game->bWaitingToStartMatch,
			(int)Game->bDelayedStart,
			(int)Game->bGameEnded,
			(int)Game->m_eTimerState,
			Game->m_fMissionTime,
			Game->m_fGameMissionTime,
			Game->m_fGameOvertimeTime,
			Game->s_fMissionTimerStartedAt,
			Game->TimeLimit,
			map_name.c_str());
	}

	LogCallEnd();
}

void TgGame__LoadGameConfig::LoadCommonGameConfig(ATgGame* Game) {

	return;

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

	int nObjectivesCounter = 0;
	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		if (UObject::GObjObjects()->Data[i]) {
			UObject* obj = UObject::GObjObjects()->Data[i];

			char* classname = obj->Class->GetFullName();

			if (strstr(obj->GetFullName(), "Default__")) {
				continue;
			}

			if (strcmp(classname, "Class TgGame.TgObjectiveAttachActor") == 0) {
				continue;
				ATgObjectiveAttachActor* ObjectiveAttachActor = reinterpret_cast<ATgObjectiveAttachActor*>(obj);
				ObjectiveAttachActor->ReplicatedMeshScale3D.X = 1;
				ObjectiveAttachActor->ReplicatedMeshScale3D.Y = 1;
				ObjectiveAttachActor->ReplicatedMeshScale3D.Z = 1;
				ObjectiveAttachActor->ReplicatedMeshRotation = ObjectiveAttachActor->StaticMeshComponent->Rotation;
				ObjectiveAttachActor->ReplicatedMeshTranslation = ObjectiveAttachActor->StaticMeshComponent->Translation;
				ObjectiveAttachActor->ReplicatedMesh = ObjectiveAttachActor->StaticMeshComponent->StaticMesh;
				ObjectiveAttachActor->DrawScale = 1;
				ObjectiveAttachActor->DrawScale3D = 1;
				// ObjectiveAttachActor->bNetInitial = 1;
				// ObjectiveAttachActor->bNetDirty = 1;
				// ObjectiveAttachActor->bForceNetUpdate = 1;
				// ObjectiveAttachActor->bSkipActorPropertyReplication = 0;
				ObjectiveAttachActor->Role = 3;
				ObjectiveAttachActor->RemoteRole = 1;
				ObjectiveAttachActor->SetOwner(WorldInfo);

				// ObjectiveAttachActor->r_EffectManager->r_Owner = ObjectiveAttachActor;
				// ObjectiveAttachActor->r_EffectManager->SetOwner(ObjectiveAttachActor);
				// ObjectiveAttachActor->r_EffectManager->Base = ObjectiveAttachActor;
				// ObjectiveAttachActor->r_EffectManager->Role = 3;
				// ObjectiveAttachActor->r_EffectManager->RemoteRole = 1;
				// ObjectiveAttachActor->r_EffectManager->bAlwaysRelevant = 1;
				// ObjectiveAttachActor->r_EffectManager->bNetInitial = 1;
				// ObjectiveAttachActor->r_EffectManager->bNetDirty = 1;
			}

			if (strcmp(classname, "Class TgGame.TgMissionObjective_Proximity") == 0) {
				ATgMissionObjective_Proximity* Objective = reinterpret_cast<ATgMissionObjective_Proximity*>(obj);
				Objective->bNetInitial = 1;
				Objective->bNetDirty = 1;
				Objective->bOnlyDirtyReplication = 0;
				Objective->bForceNetUpdate = 1;
				Objective->bSkipActorPropertyReplication = 0;
				Objective->Role = 3;
				Objective->RemoteRole = 1;
				Objective->SetOwner(WorldInfo);
				Objective->bAlwaysRelevant = 1;

				Objective->r_eStatus = 3;
				Objective->r_nOwnerTaskForce = 2;
				Objective->r_bIsLocked = 0;
				Objective->r_bIsActive = 1;

				// ATgRepInfo_Game* GameRep = (ATgRepInfo_Game*)Game->GameReplicationInfo;
				//
				// GameRep->r_Objectives[nObjectivesCounter] = Objective;
				// nObjectivesCounter++;
			}
		}
	}

	return;
	if (!bRandomSMSettingsLoaded) {
		bRandomSMSettingsLoaded = true;
		AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

		std::vector<std::string> settings;
		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			if (UObject::GObjObjects()->Data[i]) {
				// UObject* obj = UObject::GObjObjects()->Data[i];
				// if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMActor") == 0) {
				// 	ATgRandomSMActor* RandomSMActor = reinterpret_cast<ATgRandomSMActor*>(obj);
				// 	Logger::Log("wtf", "%d hiddengame: %d\n", i, RandomSMActor->StaticMeshComponent->HiddenGame);
				// }
				// if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0 && !strstr(obj->GetFullName(), "Default__")) {
				// 	ATgRandomSMManager* RandomSMManager = reinterpret_cast<ATgRandomSMManager*>(obj);
				// 	RandomSMManager->WorldInfo = WorldInfo;
				// 	RandomSMManager->ManageRandomSMActors();
				// 	break;
				// }
				// if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgStaticMeshActor") == 0) {
				// 	ATgStaticMeshActor* StaticMeshActor = reinterpret_cast<ATgStaticMeshActor*>(obj);
				// 	StaticMeshActor->bNetInitial = 1;
				// 	StaticMeshActor->bNetDirty = 1;
				// 	StaticMeshActor->bOnlyDirtyReplication = 1;
				// 	StaticMeshActor->bForceNetUpdate = 1;
				// 	StaticMeshActor->bSkipActorPropertyReplication = 0;
				// 	StaticMeshActor->bAlwaysRelevant = 0;
				// 	StaticMeshActor->WorldInfo = WorldInfo;
				// 	StaticMeshActor->bStatic = 0;
				// 	StaticMeshActor->Role = 3;
				// 	StaticMeshActor->RemoteRole = 1;
				// 	StaticMeshActor->SetOwner(WorldInfo);
				// } else if (strcmp(obj->Class->GetFullName(), "Class Engine.StaticMeshActor") == 0) {
				// 	AStaticMeshActor* StaticMeshActor = reinterpret_cast<AStaticMeshActor*>(obj);
				// 	StaticMeshActor->bNetInitial = 1;
				// 	StaticMeshActor->bNetDirty = 1;
				// 	StaticMeshActor->bOnlyDirtyReplication = 1;
				// 	StaticMeshActor->bForceNetUpdate = 1;
				// 	StaticMeshActor->bSkipActorPropertyReplication = 0;
				// 	StaticMeshActor->bAlwaysRelevant = 0;
				// 	StaticMeshActor->WorldInfo = WorldInfo;
				// 	StaticMeshActor->bStatic = 0;
				// 	StaticMeshActor->Role = 3;
				// 	StaticMeshActor->RemoteRole = 1;
				// 	StaticMeshActor->SetOwner(WorldInfo);
				// }
				//
					// ATgRandomSMActor* RandomSMActor = reinterpret_cast<ATgRandomSMActor*>(obj);
					// RandomSMActor->bStatic = 0;
					// RandomSMActor->StaticMeshComponent->CollideActors = 0;
					// RandomSMActor->StaticMeshComponent->BlockActors = 0;
					// RandomSMActor->StaticMeshComponent->AlwaysCheckCollision = 0;
					// RandomSMActor->bHidden = 1;
					// RandomSMActor->CollisionComponent->CollideActors = 0;
					// RandomSMActor->CollisionComponent->BlockActors = 0;
					// RandomSMActor->CollisionComponent->AlwaysCheckCollision = 0;
					//
					// RandomSMActor->bNetInitial = 1;
					// RandomSMActor->bNetDirty = 1;
					// RandomSMActor->bOnlyDirtyReplication = 1;
					// RandomSMActor->bForceNetUpdate = 1;
					// RandomSMActor->bSkipActorPropertyReplication = 0;
					// RandomSMActor->ForceUpdateComponents(1, 0);
					// RandomSMActor->bAlwaysRelevant = 1;
					// RandomSMActor->WorldInfo = WorldInfo;
					// RandomSMActor->bStatic = 0;
					// RandomSMActor->Role = 3;
					// RandomSMActor->RemoteRole = 1;
					// RandomSMActor->SetOwner(WorldInfo);
					// RandomSMActor->PostBeginPlay();

				// }
			}
		}

		m_randomSMSettings = settings;
	}
}
