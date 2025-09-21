#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Engine/World/WelcomePlayer/World__WelcomePlayer.hpp"
#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/GameServer/Engine/FURL/Constructor/FURL__Constructor.hpp"
#include "src/GameServer/Engine/World/SpawnPlayActor/World__SpawnPlayActor.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"

void MarshalChannel__NotifyControlMessage::Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
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
	// todo
}

