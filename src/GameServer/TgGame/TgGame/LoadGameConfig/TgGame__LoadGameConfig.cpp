#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
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

void __fastcall* TgGame__LoadGameConfig::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	Game->bWaitingToStartMatch = 0;

	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;
	Game->m_bIsTutorialMap = 0;

	LoadCommonGameConfig(Game);

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

