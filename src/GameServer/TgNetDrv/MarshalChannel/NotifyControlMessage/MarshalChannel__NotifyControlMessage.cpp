#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Engine/World/WelcomePlayer/World__WelcomePlayer.hpp"
#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/GameServer/Engine/FURL/Constructor/FURL__Constructor.hpp"
#include "src/GameServer/Engine/World/SpawnPlayActor/World__SpawnPlayActor.hpp"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"


void MarshalChannel__NotifyControlMessage::Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
	Logger::Log("debug", "MINE MarshalChannel__NotifyControlMessage START\n");
	int param_1 = 0x4FF;
	wchar_t local_410[512];
	int result;
	void* func = (void*)CMarshal__GetString::m_original;
	__asm__ __volatile__ (
		"push %[p2]\n\t"         // push param_2 (wchar_t*)
		"push %[p1]\n\t"         // push param_1 (int)
		"mov %[self], %%ecx\n\t" // move 'this' into ecx
		"call *%[fn]\n\t"
		"add $8, %%esp\n\t"      // clean stack (2 * 4 bytes)
		: "=a" (result)
		: [self] "r" (InBunch),
		[fn]   "r" (func),
		[p1]   "r" (param_1),
		[p2]   "r" (local_410)
		: "ecx", "memory"
	);
	
	char tmp[1024];
	wcstombs(tmp, local_410, sizeof(tmp));
	tmp[sizeof(tmp)-1] = '\0';

	// LogToFile("C:\\mylog.txt", "CMarshal_get_string:      result   =  %u      %s", result, tmp);

	if (strncmp(tmp, "HELLO", 5) == 0) {
		if (!Globals::Get().GWorld) {

			void* NetDriver = *(void**)((char*)Connection + 0x70);
			void* Notify = *(void**)((char*)NetDriver + 0x54);
			Globals::Get().GWorld = reinterpret_cast<UWorld*>((char*)Notify - 0x3C);
		}
		if (!Globals::Get().GWorldInfo) {
			bool bFirstSkipped = false;
			for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
				if (UObject::GObjObjects()->Data[i]) {
					if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
						if (!bFirstSkipped) {
							bFirstSkipped = true;
							continue;
						}

						Globals::Get().GWorldInfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
						break;
					}
				}
			}
		}

		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			if (UObject::GObjObjects()->Data[i]) {
				UObject* obj = UObject::GObjObjects()->Data[i];
				if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgBotFactory") == 0) {
					ATgBotFactory* botfactory = reinterpret_cast<ATgBotFactory*>(obj);
					if (botfactory->WorldInfo == nullptr) {
						botfactory->WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;
					}

					botfactory->SpawnBot();
					// botfactory->SpawnBot();
					// botfactory->SpawnBot();
					// botfactory->SpawnBot();
					// botfactory->SpawnBot();
					// botfactory->SpawnBot();
					// botfactory->SpawnBot();
				}
			}
		}

		// random sm manager start
		// AWorldInfo* worldinfo = nullptr;
		// for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		// 	if (UObject::GObjObjects()->Data[i]) {
		// 		UObject* obj = UObject::GObjObjects()->Data[i];
		// 		if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0) {
		// 			ATgRandomSMManager* randomactor = reinterpret_cast<ATgRandomSMManager*>(obj);
		// 			if (randomactor->WorldInfo == nullptr) {
		// 				randomactor->WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;
		// 			}
		//
		// 			randomactor->ManageRandomSMActors();
		// 			Logger::Log("debug", "ManageRandomSMActors() called\n");
		//
		// 			break;
		// 		}
		// 	}
		// }
		// AGameReplicationInfo* gamerep = worldinfo->GRI;
		// void* randomsmsettings = *(void**)((char*)gamerep + 0x438);
		// void* randomsmsettingsarr = *(void**)((char*)randomsmsettings + 0x10);
		// void* randomsmsettingsarr4 = *(void**)((char*)randomsmsettingsarr + 0x4);
		// Logger::DumpMemory("randomsmsettings", randomsmsettingsarr4, 0x300, 0);
		// random sm manager end

		// bPlayerConnected = true;

		*(uint32_t*)((char*)Connection + 0xF4) = 4869; // set Connection->NegotiatedVersion

		void* PackageMap = *(void**)((char*)Connection + 0xBC);
		PackageMap__Compute::CallOriginal(PackageMap);

		World__WelcomePlayer::CallOriginal((UWorld*)Globals::Get().GWorld, edx, Connection);
	} else if (strncmp(tmp, "HAVE", 4) == 0) {
		// example message: "HAVE GUID=AA108FEF49E725083C1214A5E56117F7 GEN=2"
		std::string msg(tmp); // copy

		std::string guidVal = msg.substr(10, 32);
		std::string genStr  = msg.substr(47, 1);

		int genVal = std::stoi(genStr);

		// LogToFile("C:\\mylog.txt", "Parsed guidVal='%s' genVal=%d", guidVal.c_str(), genVal);

		uint8_t bytes[16];
		for (size_t i = 0; i < 16; i++) {
			std::string byteStr = guidVal.substr(i * 2, 2);
			bytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
		}

		FGuid guid;
		// interpret as little-endian 32-bit ints (UE3 on x86 was LE)
		guid.A = *reinterpret_cast<int*>(&bytes[0]);
		guid.B = *reinterpret_cast<int*>(&bytes[4]);
		guid.C = *reinterpret_cast<int*>(&bytes[8]);
		guid.D = *reinterpret_cast<int*>(&bytes[12]);

		// LogToFile("C:\\mylog.txt", "Looking for guid: %x %x %x %x or %s", guid.A, guid.B, guid.C, guid.D, guidVal);

		void* PackageMap = *(void**)((char*)Connection + 0xBC);
		// DumpUnknownObjectMemory(PackageMap, "C:\\PackageMap.txt", 1000);

		void* List = *(void**)((char*)PackageMap + 0x3C);
		int ListCount = *(int*)((char*)PackageMap + 0x40);
		for (int i = 0; i < ListCount; i++) {
			char* Base = (char*)List + i * 0x44;

			FGuid* itemguid = (FGuid*)(Base + 0x0C);

			std::string itemguidStr = MarshalChannel__NotifyControlMessage::GuidToHex(*itemguid);

			// LogToFile("C:\\mylog.txt", "Comparing with guid: %x %x %x %x", itemguid->A, itemguid->B, itemguid->C, itemguid->D);
			// LogToFile("C:\\mylog.txt", "Comparing with guid: %s", itemguidStr.c_str());

			if (itemguidStr == guidVal) {
				// LogToFile("C:\\mylog.txt", "[PackageMap] Found matching guid: %s", guidVal.c_str());

				int32_t* RemoteGeneration = (int32_t*)(Base + 0x28);
				*RemoteGeneration = genVal;

				PackageMap__Compute::CallOriginal(PackageMap);
				break;
			}
		}
	} else if (strncmp(tmp, "JOIN", 4) == 0) {

		// todo: check if this is even needed
		for (int i=0; i<UObject::GObjObjects()->Count; i++) {
			if (UObject::GObjObjects()->Data[i]) {
				UObject* obj = UObject::GObjObjects()->Data[i];
				if (strcmp(obj->GetFullName(), "Function TgGame.TgPlayerController.ClientEnterStartState") == 0) {
					UFunction* func = (UFunction*)obj;
					func->FunctionFlags &= ~0x00000002;
					// func->FunctionFlags |= 0x40 | 0x200000;

					// LogToFile("C:\\mylog.txt", "Function TgGame.TgPlayerController.ClientEnterStartState flags updated");
					break;
				}
			}
		}

		FString portal = FString(*(FString*)((char*)Connection + 0xF8));

		FString error;
		FString error2;
		FString url = FString(Config::GetMapUrl());
		FString options = FString(Config::GetMapParams());


		// LogToFile("C:\\mylog.txt", "[SpawnPlayActor] start");

		ULocalPlayer* connplayer = (ULocalPlayer*)Connection;

		// pOriginalLocalPlayerSpawnPlayActor(connection, edx_dummy, &url, &error);
		// void* newcontrollerptr = *(void**)((char*)connplayer + 0x40);

		FString connrequesturl = *(FString*)((char*)Connection + 0xF8);
		FURL requesturl;
		FURL__Constructor::CallOriginal(&requesturl, nullptr, nullptr, Config::GetMapParams(), 0);

		ATgPlayerController* newcontrollerptr = World__SpawnPlayActor::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, connplayer, 2, &requesturl, &error, 0);

		// LogFString(error);

		// LogToFile("C:\\mylog.txt", "[SpawnPlayActor] end");

		if (newcontrollerptr) {
			MarshalChannel__NotifyControlMessage::HandlePlayerConnected(Connection, newcontrollerptr);


			// AWorldInfo* worldinfo = nullptr;
			// for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			// 	if (UObject::GObjObjects()->Data[i]) {
			// 		UObject* obj = UObject::GObjObjects()->Data[i];
			// 		if (
			// 			FALSE
			// 			// strcmp(obj->Class->GetFullName(), "Class Engine.Actor") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class Engine.StaticMeshActor") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgStaticMeshActor") == 0
			// 			|| strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMActor") == 0
			// 			|| strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgDynamicSMActor") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgDynamicDestructible") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgDummyActor") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgInterpActor") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgKActorSpawnable") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgKAssetSpawnable") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgMeshAssembly") == 0
			// 			// || strcmp(obj->Class->GetFullName(), "Class TgGame.TgDoor") == 0
			// 		) {
			// 			AActor* actor = reinterpret_cast<AActor*>(obj);
			// 			// LogToFile("C:\\serveractors.txt", "%s %d", actor->GetFullName(), actor->NetIndex);
			// 			actor->bAlwaysRelevant = 1;
			// 			actor->bNetDirty = 1;
			// 			actor->bNetInitial = 1;
			// 			actor->bForceNetUpdate = 1;
			// 			actor->RemoteRole = 1;
			// 			actor->Role = 3;
			// 			actor->NetUpdateFrequency = 0.1f;
			// 			actor->bSkipActorPropertyReplication = 0;
			// 			if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMActor") == 0) {
			// 				ATgRandomSMActor* randomactor = reinterpret_cast<ATgRandomSMActor*>(obj);
			// 				if (randomactor->WorldInfo == nullptr && worldinfo == nullptr) {
			//
			// 					bool bFirstSkipped = false;
			// 					for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			// 						if (UObject::GObjObjects()->Data[i]) {
			// 							if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
			// 								if (!bFirstSkipped) {
			// 									// LogToFile("C:\\tcplog.txt", "Skipping first WorldInfo");
			// 									bFirstSkipped = true;
			// 									continue;
			// 								}
			//
			// 								worldinfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
			//
			//
			// 								// LogToFile("C:\\tcplog.txt", "WI found: %s", WI->GetFullName());
			//
			// 								// UClass* GameClass = WI->GetGameClass( );
			// 								// if (GameClass == nullptr) {
			// 								// 	// LogToFile("C:\\tcplog.txt", "GameClass is nullptr");
			// 								// 	continue;
			// 								// }
			//
			// 								// LogToFile("C:\\tcplog.txt", "GameClass: %s", GameClass->GetFullName());
			// 							}
			// 						}
			// 					}
			// 				}
			// 				randomactor->WorldInfo = worldinfo;
			// 				randomactor->PostBeginPlay();
			// 			}
			// 			if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMManager") == 0) {
			// 				ATgRandomSMManager* randomactor = reinterpret_cast<ATgRandomSMManager*>(obj);
			// 				if (randomactor->WorldInfo == nullptr && worldinfo == nullptr) {
			//
			// 					bool bFirstSkipped = false;
			// 					for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			// 						if (UObject::GObjObjects()->Data[i]) {
			// 							if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
			// 								if (!bFirstSkipped) {
			// 									// LogToFile("C:\\tcplog.txt", "Skipping first WorldInfo");
			// 									bFirstSkipped = true;
			// 									continue;
			// 								}
			//
			// 								worldinfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
			//
			//
			// 								// LogToFile("C:\\tcplog.txt", "WI found: %s", WI->GetFullName());
			//
			// 								// UClass* GameClass = WI->GetGameClass( );
			// 								// if (GameClass == nullptr) {
			// 								// 	// LogToFile("C:\\tcplog.txt", "GameClass is nullptr");
			// 								// 	continue;
			// 								// }
			//
			// 								// LogToFile("C:\\tcplog.txt", "GameClass: %s", GameClass->GetFullName());
			// 							}
			// 						}
			// 					}
			// 				}
			//
			// 				randomactor->WorldInfo = worldinfo;
			// 				randomactor->ManageRandomSMActors();
			// 				Logger::Log("debug", "ManageRandomSMActors() called\n");
			// 			}
			//
			//
			// 			continue;
			// 		}
			// 	}
			// }
		}
	}
}


void MarshalChannel__NotifyControlMessage::HandlePlayerConnected(UNetConnection* Connection, ATgPlayerController* Controller) {

	ULocalPlayer* connplayer = (ULocalPlayer*)Connection;
	UWorld* pWorld = (UWorld*)Globals::Get().GWorld;

	void* gameinfoptr = World__GetGameInfo::CallOriginal((UWorld*)pWorld);
	ATgGame* game = reinterpret_cast<ATgGame*>(gameinfoptr);
	ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(game->GameReplicationInfo);

	ATgPlayerController* newcontroller = reinterpret_cast<ATgPlayerController*>(Controller);
	ATgPawn_Character* newpawn = NULL;

	newcontroller->PlayerReplicationInfo->bOnlySpectator = 0;

	game->eventPostLogin(newcontroller);

	TcpEvent PlayerPawnSpawned;
	PlayerPawnSpawned.Type = 1;
	PlayerPawnSpawned.Pawn = (ATgPawn*)newcontroller->Pawn;
	GTcpEvents.push_back(PlayerPawnSpawned);

	if (GTeamsData.Attackers->r_BeaconManager->r_Beacon == nullptr) {
		if (GTeamsData.BeaconAttackers) {
			GTeamsData.Attackers->r_BeaconManager->r_Beacon = GTeamsData.BeaconAttackers;
			GTeamsData.Attackers->r_BeaconManager->r_TaskForce = GTeamsData.Attackers;
			GTeamsData.Attackers->r_BeaconManager->bNetDirty = 1;
			GTeamsData.Attackers->r_BeaconManager->bForceNetUpdate = 1;
			GTeamsData.Attackers->r_BeaconManager->bNetInitial = 1;
			GTeamsData.BeaconAttackers->r_DRI->r_TaskforceInfo = GTeamsData.Attackers;
			GTeamsData.Attackers->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconAttackers, 1);
			if (GTeamsData.BeaconEntranceAttackers) {
				GTeamsData.BeaconEntranceAttackers->r_DRI->r_TaskforceInfo = GTeamsData.Attackers;
				// LogToFile("C:\\mylog.txt", "Attackers beacon entrance taskforce set");
			}
			// LogToFile("C:\\mylog.txt", "Attackers beacon set");
		} else {
			// LogToFile("C:\\mylog.txt", "Attackers beacon global is null");
		}
	} else {
		// LogToFile("C:\\mylog.txt", "Attackers beacon is not null");
	}
	if (GTeamsData.Defenders->r_BeaconManager->r_Beacon == nullptr) {
		if (GTeamsData.BeaconDefenders) {
			GTeamsData.Defenders->r_BeaconManager->r_TaskForce = GTeamsData.Defenders;
			GTeamsData.Defenders->r_BeaconManager->r_Beacon = GTeamsData.BeaconDefenders;
			GTeamsData.Defenders->r_BeaconManager->bNetDirty = 1;
			GTeamsData.Defenders->r_BeaconManager->bForceNetUpdate = 1;
			GTeamsData.Defenders->r_BeaconManager->bNetInitial = 1;
			GTeamsData.BeaconDefenders->r_DRI->r_TaskforceInfo = GTeamsData.Defenders;
			GTeamsData.Defenders->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconDefenders, 1);
			if (GTeamsData.BeaconEntranceDefenders) {
				GTeamsData.BeaconEntranceDefenders->r_DRI->r_TaskforceInfo = GTeamsData.Defenders;
				// LogToFile("C:\\mylog.txt", "Defenders beacon entrance taskforce set");
			}
			// LogToFile("C:\\mylog.txt", "Defenders beacon set");
		} else {
			// LogToFile("C:\\mylog.txt", "Defenders beacon global is null");
		}
	} else {
		// LogToFile("C:\\mylog.txt", "Defenders beacon is not null");
	}

	// LogToFile("C:\\mylog.txt", "Attackers taskforce %d", GTeamsData.Attackers->r_nTaskForce);

	if (GTeamsData.Attackers->r_BeaconManager != nullptr) {
		Logger::Log("debug", "Attackers have beacon manager\n");
		if (GTeamsData.Attackers->r_BeaconManager->r_TaskForce != nullptr) {
			Logger::Log("debug", "Attackers beacon manager taskforce %d\n", GTeamsData.Attackers->r_BeaconManager->r_TaskForce->r_nTaskForce);
		}
		if (GTeamsData.Attackers->r_BeaconManager->r_Beacon != nullptr) {
			Logger::Log("debug", "Attackers have beacon\n");
			if (GTeamsData.Attackers->r_BeaconManager->r_Beacon->r_DRI != nullptr) {
				Logger::Log("debug", "Attackers beacon has replication info\n");
				if (GTeamsData.Attackers->r_BeaconManager->r_Beacon->r_DRI->r_TaskforceInfo != nullptr) {
					Logger::Log("debug", "Attackers beacon taskforce %d\n", GTeamsData.Attackers->r_BeaconManager->r_Beacon->r_DRI->r_TaskforceInfo->r_nTaskForce);
				} else {
					Logger::Log("debug", "Attackers beacon taskforce is null\n");
				}
			} else {
				Logger::Log("debug", "Attackers beacon replication info is null\n");
			}
		} else {
			Logger::Log("debug", "Attackers beacon is null\n");
		}
	} else {
		Logger::Log("debug", "Attackers do not have beacon manager\n");
	}

	// LogToFile("C:\\mylog.txt", "Attackers beacon manager taskforce %d", GTeamsData.Attackers->r_BeaconManager->r_TaskForce->r_nTaskForce);
	// LogToFile("C:\\mylog.txt", "Attackers beacon taskforce %d", GTeamsData.Attackers->r_BeaconManager->r_Beacon->r_DRI->r_TaskforceInfo->r_nTaskForce);
	Logger::Log("debug", "MINE MarshalChannel__NotifyControlMessage END\n");
}

