#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Engine/World/WelcomePlayer/World__WelcomePlayer.hpp"
#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/GameServer/Engine/FURL/Constructor/FURL__Constructor.hpp"
#include "src/GameServer/Engine/World/SpawnPlayActor/World__SpawnPlayActor.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.hpp"
#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"


void MarshalChannel__NotifyControlMessage::Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
	LogCallBegin();

	int param_1 = 0x4FF;
	wchar_t* local_410 = new wchar_t[512];
	int result = CMarshal__GetString::CallOriginal(InBunch, edx, param_1, local_410);
	
	char tmp[1024];
	wcstombs(tmp, local_410, sizeof(tmp));
	tmp[sizeof(tmp)-1] = '\0';

	// Logger::Log("wtf", "\n\n[MarshalChannel__NotifyControlMessage]: %s\n", tmp);

	Logger::Log(GetLogChannel(), "Message: %s\n", tmp);

	if (strncmp(tmp, "HELLO", 5) == 0) {
		

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
		// for (int i=0; i<UObject::GObjObjects()->Count; i++) {
		// 	if (UObject::GObjObjects()->Data[i]) {
		// 		UObject* obj = UObject::GObjObjects()->Data[i];
		// 		if (strcmp(obj->GetFullName(), "Function TgGame.TgPlayerController.ClientEnterStartState") == 0) {
		// 			UFunction* func = (UFunction*)obj;
		// 			func->FunctionFlags &= ~0x00000002;
		// 			// func->FunctionFlags |= 0x40 | 0x200000;
		//
		// 			// LogToFile("C:\\mylog.txt", "Function TgGame.TgPlayerController.ClientEnterStartState flags updated");
		// 			break;
		// 		}
		// 	}
		// }

		FString portal = FString(*(FString*)((char*)Connection + 0xF8));

		FString error;
		FString error2;
		std::wstring urlstr = Config::GetMapUrl();
		FString url = FString(urlstr.data());
		std::wstring params = Config::GetMapParams();

		FString options = FString(params.data());


		// LogToFile("C:\\mylog.txt", "[SpawnPlayActor] start");

		ULocalPlayer* connplayer = (ULocalPlayer*)Connection;

		// pOriginalLocalPlayerSpawnPlayActor(connection, edx_dummy, &url, &error);
		// void* newcontrollerptr = *(void**)((char*)connplayer + 0x40);

		FString connrequesturl = *(FString*)((char*)Connection + 0xF8);
		FURL requesturl;
		std::wstring params2 = Config::GetMapParams();
		FURL__Constructor::CallOriginal(&requesturl, nullptr, nullptr, params2.data(), 0);

		ATgPlayerController* newcontrollerptr = World__SpawnPlayActor::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, connplayer, 2, &requesturl, &error, 0);
		connplayer->Actor = newcontrollerptr;

		// LogFString(error);

		// LogToFile("C:\\mylog.txt", "[SpawnPlayActor] end");

		if (newcontrollerptr) {
			MarshalChannel__NotifyControlMessage::HandlePlayerConnected(Connection, newcontrollerptr);
		}
	}

	LogCallEnd();
}


void MarshalChannel__NotifyControlMessage::HandlePlayerConnected(UNetConnection* Connection, ATgPlayerController* Controller) {
	static bool bFirstPlayerSpawned = false;

	ULocalPlayer* connplayer = (ULocalPlayer*)Connection;
	UWorld* pWorld = (UWorld*)Globals::Get().GWorld;

	void* gameinfoptr = World__GetGameInfo::CallOriginal((UWorld*)pWorld);
	ATgGame* game = reinterpret_cast<ATgGame*>(gameinfoptr);
	ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(game->GameReplicationInfo);


	// for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
	// 	if (UObject::GObjObjects()->Data[i]) {
	// 		UObject* obj = UObject::GObjObjects()->Data[i];
	// 		if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgRandomSMActor") == 0) {
	// 			ATgRandomSMActor* RandomSMActor = reinterpret_cast<ATgRandomSMActor*>(obj);
	// 			Logger::Log("wtf", "%d hiddengame: %d\n", i, RandomSMActor->StaticMeshComponent->HiddenGame);
	// 		}
	// 	}
	// }

	if (gamerep->s_pRandomSMSettings.Dummy > 0) {
		CGameClient__SendMapRandomSMSettingsMarshal::Call(Connection, (void*)(gamerep->s_pRandomSMSettings.Dummy));
	}
	// Logger::DumpMemory("wtf", (void*)(gamerep->s_pRandomSMSettings.Dummy ), 0x1000, 0);

	ATgPlayerController* newcontroller = reinterpret_cast<ATgPlayerController*>(Controller);
	ATgPawn_Character* newpawn = NULL;

	newcontroller->PlayerReplicationInfo->bOnlySpectator = 0;

	newcontroller->PlayerReplicationInfo->Team = GTeamsData.Attackers;
	// newcontroller->PlayerReplicationInfo->Team = GTeamsData.Defenders;

	game->eventPostLogin(newcontroller);

	newcontroller->ResetForceViewTarget();

	// TcpEvent PlayerPawnSpawned;
	// PlayerPawnSpawned.Type = 1;
	// PlayerPawnSpawned.Pawn = (ATgPawn*)newcontroller->Pawn;
	// GTcpEvents.push_back(PlayerPawnSpawned);



	if (!bFirstPlayerSpawned) {
		bFirstPlayerSpawned = true;

		if (GTeamsData.Attackers->r_BeaconManager->r_Beacon == nullptr) {
			if (GTeamsData.BeaconAttackers) {
				GTeamsData.Attackers->r_BeaconManager->r_Beacon = GTeamsData.BeaconAttackers;
				GTeamsData.Attackers->r_BeaconManager->r_TaskForce = GTeamsData.Attackers;
				GTeamsData.Attackers->r_BeaconManager->bNetDirty = 1;
				GTeamsData.Attackers->r_BeaconManager->bForceNetUpdate = 1;
				GTeamsData.Attackers->r_BeaconManager->bNetInitial = 1;
				GTeamsData.Defenders->r_BeaconManager->bAlwaysRelevant = 1;
				GTeamsData.BeaconAttackers->r_DRI->r_TaskforceInfo = GTeamsData.Attackers;
				GTeamsData.Attackers->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconAttackers, 1);
				if (GTeamsData.BeaconEntranceAttackers) {
					GTeamsData.BeaconEntranceAttackers->r_DRI->r_TaskforceInfo = GTeamsData.Attackers;
					// ATgRepInfo_Beacon* AttackersBeaconRep = (ATgRepInfo_Beacon*)GTeamsData.BeaconEntranceAttackers->r_DRI;
					// AttackersBeaconRep->r_bDeployed = 1;
					// AttackersBeaconRep->r_vLoc = GTeamsData.BeaconAttackers->Location;

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
				GTeamsData.Defenders->r_BeaconManager->bAlwaysRelevant = 1;
				GTeamsData.BeaconDefenders->r_DRI->r_TaskforceInfo = GTeamsData.Defenders;
				GTeamsData.Defenders->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconDefenders, 1);
				if (GTeamsData.BeaconEntranceDefenders) {
					GTeamsData.BeaconEntranceDefenders->r_DRI->r_TaskforceInfo = GTeamsData.Defenders;
					// ATgRepInfo_Beacon* DefendersBeaconRep = (ATgRepInfo_Beacon*)GTeamsData.BeaconEntranceDefenders->r_DRI;
					// DefendersBeaconRep->r_bDeployed = 1;
					// DefendersBeaconRep->r_vLoc = GTeamsData.BeaconDefenders->Location;

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

		game->m_fGameMissionTime = 15 * 60;
		game->m_eTimerState = 0;
		game->StartGameTimer();


		TArray<USequenceObject*> Events;
		TArray<int> Indices;
		AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
		// AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;

		WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
			ClassPreloader::GetSeqEventLevelLoadedClass(),
			1,
			&Events
		);

		for (int i = 0; i < Events.Num(); i++) {
			USeqEvent_LevelLoaded* Event = (USeqEvent_LevelLoaded*)Events.Data[i];
			Event->CheckActivate(game, game, 0, 0, Indices);
		}

		TArray<USequenceObject*> Events2;
		TArray<int> Indices2;
		WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
			ClassPreloader::GetTgSeqEventLevelFadedInClass(),
			1,
			&Events2
		);

		for (int i = 0; i < Events2.Num(); i++) {
			UTgSeqEvent_LevelFadedIn* Event = (UTgSeqEvent_LevelFadedIn*)Events2.Data[i];
			Event->CheckActivate(game, game, 0, 0, Indices2);
		}
	}


	// LogToFile("C:\\mylog.txt", "Attackers beacon manager taskforce %d", GTeamsData.Attackers->r_BeaconManager->r_TaskForce->r_nTaskForce);
	// LogToFile("C:\\mylog.txt", "Attackers beacon taskforce %d", GTeamsData.Attackers->r_BeaconManager->r_Beacon->r_DRI->r_TaskforceInfo->r_nTaskForce);
	Logger::Log("debug", "MINE MarshalChannel__NotifyControlMessage END\n");
}

