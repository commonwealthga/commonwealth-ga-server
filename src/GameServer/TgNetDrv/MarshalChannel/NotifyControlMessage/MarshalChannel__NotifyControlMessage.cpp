#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Engine/World/WelcomePlayer/World__WelcomePlayer.hpp"
#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/GameServer/Engine/FURL/Constructor/FURL__Constructor.hpp"
#include "src/GameServer/Engine/World/SpawnPlayActor/World__SpawnPlayActor.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Constants/GameTypes.h"
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

	ATgPawn_Character* newpawnchar = (ATgPawn_Character*)newcontroller->Pawn;

	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(newcontroller->PlayerReplicationInfo);
	// newrepplayer->Team = defenders;
	newrepplayer->bAdmin = 1;
	newrepplayer->r_CustomCharacterAssembly.SuitMeshId = 1225;
	newrepplayer->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	newrepplayer->r_CustomCharacterAssembly.HairMeshId = 1974;
	newrepplayer->r_CustomCharacterAssembly.HelmetMeshId = -1;
	newrepplayer->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.bBald = false;
	newrepplayer->r_CustomCharacterAssembly.bHideHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	newrepplayer->r_CustomCharacterAssembly.bHalfHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	newrepplayer->r_CustomCharacterAssembly.HeadFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.SuitFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	newrepplayer->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_nProfileId = 567; // medic
	newrepplayer->r_nHealthCurrent = 1300;
	newrepplayer->r_nHealthMaximum = 1300;
	newrepplayer->r_nCharacterId = newpawnchar->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawnchar;
	newrepplayer->r_ApproxLocation = newpawnchar->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");

	newrepplayer->eventSetPlayerName(FString(L"Zaxik"));
	newrepplayer->r_TaskForce = GTeamsData.Attackers;
	newrepplayer->SetTeam(GTeamsData.Attackers);
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

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

