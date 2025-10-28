#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool TgGame__LoadGameConfig::bRandomSMSettingsLoaded = false;
std::vector<std::string> TgGame__LoadGameConfig::m_randomSMSettings;

void __fastcall* TgGame__LoadGameConfig::Call(ATgGame* Game, void* edx) {
	Logger::Log("wtf", "MINE TgGame__LoadGameConfig START\n");
	Game->bWaitingToStartMatch = 0;

	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;
	Game->m_bIsTutorialMap = 0;

	LoadCommonGameConfig(Game);

	Logger::Log("wtf", "MINE TgGame__LoadGameConfig END\n");
}

void TgGame__LoadGameConfig::LoadCommonGameConfig(ATgGame* Game) {
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

