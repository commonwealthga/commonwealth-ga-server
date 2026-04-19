#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Misc/CMarshal/GetGuid/CMarshal__GetGuid.hpp"
#include "src/GameServer/Engine/World/WelcomePlayer/World__WelcomePlayer.hpp"
#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/GameServer/Engine/FURL/Constructor/FURL__Constructor.hpp"
#include "src/GameServer/Engine/World/SpawnPlayActor/World__SpawnPlayActor.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.hpp"
#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Constants/GameTypes.h"
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

		UUID* guid = new UUID();
		memset(guid, 0, sizeof(UUID));
		CMarshal__GetGuid::CallOriginal(InBunch, edx, 0x473, guid);

		Logger::DumpMemory("session_guid", (void*)guid, 0x10, 0);

		std::string session_guid = SessionGuidToHex(guid);

		GClientConnectionsData[(int)Connection].SessionGuid = session_guid;
		Logger::Log(GetLogChannel(), "[MarshalChannel__NotifyControlMessage] HELLO: session_guid=%s\n", session_guid.c_str());

		*(uint32_t*)((char*)Connection + 0xF4) = 4869; // set Connection->NegotiatedVersion

		void* PackageMap = *(void**)((char*)Connection + 0xBC);
		PackageMap__Compute::CallOriginal(PackageMap);

		// GUID fix for forced exports now runs inside the SendPackageMap hook
		// (NetConnection__SendPackageMap), where the list is already populated.

		World__WelcomePlayer::CallOriginal((UWorld*)Globals::Get().GWorld, edx, Connection, L"");


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

		// Parse ?gaSession= from the JOIN message to correlate with TCP session
	
		std::string session_guid = GClientConnectionsData[(int32_t)Connection].SessionGuid;
		std::string player_name_from_registry;

		auto info = PlayerRegistry::GetByGuid(session_guid);
		if (info) {
			player_name_from_registry = info->player_name;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.session_guid = session_guid;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.player_name = info->player_name;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.player_name_w = info->player_name_w;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.ip_address = info->ip_address;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.user_id = info->user_id;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.selected_profile_id  = info->selected_profile_id;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.selected_character_id = info->selected_character_id;
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.task_force = info->task_force;
			GClientConnectionsData[(int32_t)Connection].pPlayerInfo = PlayerRegistry::GetByGuidPtr(session_guid);
			Logger::Log(GetLogChannel(), "JOIN: identified player '%s' (guid=%s, profile=%u, connection=%d)\n",
				info->player_name.c_str(), session_guid.c_str(), info->selected_profile_id, (int32_t)Connection);
		} else {
			Logger::Log(GetLogChannel(), "JOIN: session guid %s not found in registry (direct connect? no PLAYER_REGISTER received)\n", session_guid.c_str());
		}


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

		// NOTE: Do NOT create local FString/FURL copies of UE3-owned data.
		// FString's destructor calls delete[] but UE3 allocates with its own
		// heap (appMalloc).  Mixing allocators corrupts the C++ heap.

		FString error;
		FString error2;

		std::wstring params2 = Config::GetMapParams();
		Logger::Log(GetLogChannel(), "[SpawnPlayActor] params: %s\n", params2.data());

		ULocalPlayer* connplayer = (ULocalPlayer*)Connection;

		FURL requesturl;
		FURL__Constructor::CallOriginal(&requesturl, nullptr, nullptr, params2.data(), 0);

		ATgPlayerController* newcontrollerptr = World__SpawnPlayActor::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, connplayer, 2, &requesturl, &error, 0);
		connplayer->Actor = newcontrollerptr;

		// Zero out FURL and error FStrings so their destructors don't delete[]
		// memory that UE3 allocated with appMalloc.
		memset(&requesturl, 0, sizeof(FURL));
		memset(&error, 0, sizeof(FString));
		memset(&error2, 0, sizeof(FString));

		// LogFString(error);

		// LogToFile("C:\\mylog.txt", "[SpawnPlayActor] end");

		if (newcontrollerptr) {
			MarshalChannel__NotifyControlMessage::HandlePlayerConnected(Connection, newcontrollerptr, session_guid, player_name_from_registry);
		}
	}

	LogCallEnd();
}


void MarshalChannel__NotifyControlMessage::HandlePlayerConnected(UNetConnection* Connection, ATgPlayerController* Controller,
	const std::string& session_guid, const std::string& player_name) {
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

	if (GClientConnectionsData[(int32_t)Connection].PlayerInfo.task_force == 1) {
		newcontroller->PlayerReplicationInfo->Team = GTeamsData.Attackers;
	} else {
		newcontroller->PlayerReplicationInfo->Team = GTeamsData.Defenders;
	}
	// newcontroller->PlayerReplicationInfo->Team = GTeamsData.Defenders;

	game->eventPostLogin(newcontroller);

	// Activate the game HUD's gameplay flags. TgPlayerController overrides
	// ClientSetCinematicMode: when bAffectsHUD is set and current bShowHUD
	// doesn't match the requested state, it calls TgHUD.ToggleDrawAllHUD()
	// which flips BOTH bShowHUD → true AND m_bDrawPawnHUD → true — which
	// are the two flags DrawActorOverlays' inner gate requires.
	//
	// NOTE: the SDK wrapper APlayerController::ClientSetCinematicMode uses
	// a C++ bitfield params struct (`unsigned long x : 1` at each 4-byte
	// offset). C++ default packing puts all four bools in the SAME dword,
	// but UC bytecode reads each bool as LOW BIT of a SEPARATE dword at
	// offsets 0x0/0x4/0x8/0xC. The SDK wrapper therefore sends garbage
	// params — bAffectsHUD arrives as 0 on the client and the inner if
	// is skipped. We call ProcessEvent directly with a manually-laid-out
	// params struct to work around this.
	{
		struct { uint32_t bInCinematicMode, bAffectsMovement, bAffectsTurning, bAffectsHUD; } parms = { 0, 0, 0, 1 };
		UFunction* pFn = (UFunction*)ObjectCache::Find("Function Engine.PlayerController.ClientSetCinematicMode");
		if (pFn) newcontroller->ProcessEvent(pFn, &parms, nullptr);
	}

	// Pre-spawned beacon deferral (team colors).
	//
	// Beacons placed on the map get their SpawnObject fired at server startup,
	// BEFORE any client exists.  Their DRI replicated-object refs (notably
	// r_TaskforceInfo) serialize by NetGUID; those NetGUIDs aren't in any
	// client's table yet, so on the first-connect initial replication they
	// resolve to null PERMANENTLY (re-dirtying the field post-connect didn't
	// cause the client's RecalculateMaterial to pick up the corrected state
	// — the material chose enemy on first render and stuck).
	//
	// The fix is to skip the SpawnObject body entirely when no client exists,
	// stash the factory pointer, and replay the spawns here on first connect.
	// Manual (runtime) beacon deploys already go through the correct flow
	// because clients are around when they fire; this alignment fixes the
	// pre-spawned case by putting them on the same post-connect path.
	if (!TgBeaconFactory__SpawnObject::s_firstPlayerConnected) {
		TgBeaconFactory__SpawnObject::FlushPendingSpawns();
	}

	//newcontroller->ResetForceViewTarget(); // fix for pawn relevancy - they otherwise don't show up until a player dies and respawns, because the game things view isn't moving

	// TgGame__SpawnPlayerCharacter::GiveJetpack((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, newcontroller, 999);
	// TgGame__SpawnPlayerCharacter::GiveAgonizer((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, newcontroller, 1000);

	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 5800, 221, 1, 1162, 0, 1, GA_G::TGDT_MELEE, 996); // lifestealer
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 2991, 198, 2, 1162, 0, 1, GA_G::TGDT_RANGED, 1000); // agonizer
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 2906, 200, 3, 1162, 0, 1, GA_G::TGDT_SPECIALTY, 995); // bfb
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 7032, 201, 5, 1162, 1, 0, GA_G::TGDT_TRAVEL, 999); // jetpack
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 2531, 203, 7, 1162, 1, 0, GA_G::TGDT_OFF_HAND, 997); // healnade
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 2773, 386, 10, 1162, 1, 0, GA_G::TGDT_MORALE, 994); // heal boost
	// TgGame__SpawnBotById::GiveDeviceById((ATgPawn_Character*)newcontroller->Pawn, (ATgRepInfo_Player*)newcontroller->PlayerReplicationInfo, 864, 502, 15, 1162, 1, 0, GA_G::TGDT_OFF_HAND, 998); // rest device

	GPawnSessions[(ATgPawn*)newcontroller->Pawn] = session_guid;
	GClientConnectionsData[(int32_t)Connection].Pawn = (ATgPawn_Character*)newcontroller->Pawn;

	// Now that GPawnSessions has the mapping, our ReapplyCharacterSkillTree
	// hook can resolve the pawn's PlayerInfo and materialize skill-based
	// effect groups. Running it earlier (at SpawnPlayerCharacter time) hits
	// the "no session mapping" path and no-ops.
	if (auto* tgpc = (ATgPlayerController*)newcontroller) {
		if (auto* pc = (ATgPawn_Character*)tgpc->Pawn) {
			pc->ReapplyCharacterSkillTree();
		}
	}

	{
		nlohmann::json ev;
		ev["type"]            = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]         = "spawn";
		ev["instance_id"]     = IpcClient::GetInstanceId();
		ev["session_guid"]    = session_guid;
		ev["pawn_id"]         = (int)((ATgPawn*)newcontroller->Pawn)->r_nPawnId;
		ev["item_profile_id"] = (int)((ATgPawn_Character*)newcontroller->Pawn)->r_nItemProfileId;
		IpcClient::Send(ev.dump());
	}

	// Also send PLAYER_JOINED to track this player in the instance
	{
		int tf = GClientConnectionsData[(int32_t)Connection].PlayerInfo.task_force;
		nlohmann::json joined;
		joined["type"]         = IpcProtocol::MSG_PLAYER_JOINED;
		joined["instance_id"]  = IpcClient::GetInstanceId();
		joined["session_guid"] = session_guid;
		joined["task_force"]   = tf;
		IpcClient::Send(joined.dump());
	}

	// First player: wire up BeaconManagers (RegisterBeacon sets r_BeaconStatus so entrance activates).
	if (!bFirstPlayerSpawned) {
		bFirstPlayerSpawned = true;

		// Re-assert the beacon exit's DRI team state after RegisterBeacon runs.
		// RegisterBeacon is FUNC_Native and internally calls TgRepInfo_Deployable::SetTaskForce,
		// which per reference_deployable_team_colors.md zeros r_InstigatorInfo — leaving the
		// DRI half-broken and making the exit render as enemy on every client (matches the
		// observed symptom of entrance=correct / exit=wrong, since only exits go through this).
		auto reassertBeaconDri = [](ATgDeploy_Beacon* beacon, ATgRepInfo_TaskForce* tf) {
			if (!beacon || !beacon->r_DRI || !tf) return;
			beacon->r_DRI->r_bOwnedByTaskforce = 1;
			beacon->r_DRI->r_TaskforceInfo     = tf;
			beacon->r_DRI->bNetDirty           = 1;
			beacon->r_DRI->bForceNetUpdate     = 1;
			Logger::Log("team_colors",
				"[PostRegister re-assert] beacon=0x%p dri=0x%p tf=0x%p\n",
				beacon, beacon->r_DRI, tf);
		};

		if (GTeamsData.BeaconAttackers && GTeamsData.Attackers->r_BeaconManager->r_Beacon == nullptr) {
			GTeamsData.Attackers->r_BeaconManager->r_Beacon    = GTeamsData.BeaconAttackers;
			GTeamsData.Attackers->r_BeaconManager->r_TaskForce = GTeamsData.Attackers;
			GTeamsData.Attackers->r_BeaconManager->bNetDirty      = 1;
			GTeamsData.Attackers->r_BeaconManager->bForceNetUpdate = 1;
			GTeamsData.Attackers->r_BeaconManager->bNetInitial     = 1;
			GTeamsData.Attackers->r_BeaconManager->bAlwaysRelevant = 1;
			GTeamsData.Attackers->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconAttackers, 1);
			reassertBeaconDri(GTeamsData.BeaconAttackers, GTeamsData.Attackers);
		}

		if (GTeamsData.BeaconDefenders && GTeamsData.Defenders->r_BeaconManager->r_Beacon == nullptr) {
			GTeamsData.Defenders->r_BeaconManager->r_Beacon    = GTeamsData.BeaconDefenders;
			GTeamsData.Defenders->r_BeaconManager->r_TaskForce = GTeamsData.Defenders;
			GTeamsData.Defenders->r_BeaconManager->bNetDirty      = 1;
			GTeamsData.Defenders->r_BeaconManager->bForceNetUpdate = 1;
			GTeamsData.Defenders->r_BeaconManager->bNetInitial     = 1;
			GTeamsData.Defenders->r_BeaconManager->bAlwaysRelevant = 1;
			GTeamsData.Defenders->r_BeaconManager->RegisterBeacon(GTeamsData.BeaconDefenders, 1);
			reassertBeaconDri(GTeamsData.BeaconDefenders, GTeamsData.Defenders);
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

	Logger::Log("debug", "MINE MarshalChannel__NotifyControlMessage END\n");
}

// ---------------------------------------------------------------------------
// Forced-export GUID fix
// ---------------------------------------------------------------------------

std::unordered_map<std::string, std::string> MarshalChannel__NotifyControlMessage::s_packageFileMap;
bool MarshalChannel__NotifyControlMessage::s_packageFileMapBuilt = false;

// Recursively scan a directory for .upk and .u files, building name -> path map.
// Uses POSIX dirent (Wine supports this).
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

static void ScanDirectory(const std::string& dir, std::unordered_map<std::string, std::string>& outMap) {
	DIR* d = opendir(dir.c_str());
	if (!d) return;

	struct dirent* entry;
	while ((entry = readdir(d)) != nullptr) {
		if (entry->d_name[0] == '.') continue; // skip . and ..

		std::string fullPath = dir + "/" + entry->d_name;

		struct stat st;
		if (stat(fullPath.c_str(), &st) != 0) continue;

		if (S_ISDIR(st.st_mode)) {
			ScanDirectory(fullPath, outMap);
		} else if (S_ISREG(st.st_mode)) {
			std::string name(entry->d_name);
			// Check for .upk or .u extension
			size_t dotPos = name.rfind('.');
			if (dotPos == std::string::npos) continue;
			std::string ext = name.substr(dotPos);
			if (ext != ".upk" && ext != ".u") continue;

			std::string baseName = name.substr(0, dotPos);
			// Only store first found (don't overwrite)
			if (outMap.find(baseName) == outMap.end()) {
				outMap[baseName] = fullPath;
			}
		}
	}
	closedir(d);
}

void MarshalChannel__NotifyControlMessage::BuildPackageFileMap() {
	if (s_packageFileMapBuilt) return;
	s_packageFileMapBuilt = true;

	// Derive CookedPC path from the game binary's location.
	// The game binary is at <GameRoot>/Binaries/GlobalAgenda.exe,
	// and CookedPC is at <GameRoot>/TgGame/CookedPC/ (case may vary on Linux).
	//
	// Prefer the -gamepath= command-line arg (a real POSIX path passed by the
	// control server spawner) over GetModuleFileNameA, which returns a Wine path
	// that requires drive-letter stripping and may not resolve on all setups.
	std::string path = Config::GetGamePath();
	if (path.empty()) {
		// Fallback: GetModuleFileNameA (Wine path, needs conversion)
		char exePath[512] = {};
		GetModuleFileNameA(nullptr, exePath, sizeof(exePath) - 1);
		path = std::string(exePath);
		for (char& c : path) { if (c == '\\') c = '/'; }
		if (path.size() >= 2 && (path[0] == 'Z' || path[0] == 'z') && path[1] == ':') {
			path = path.substr(2);
		}
		Logger::Log("packagemap", "[PackageMap] Using GetModuleFileNameA fallback: %s\n", path.c_str());
	} else {
		Logger::Log("packagemap", "[PackageMap] Using -gamepath: %s\n", path.c_str());
	}

	// Go up from Binaries/GlobalAgenda.exe to GameRoot, then into TgGame/CookedPC
	size_t binPos = path.rfind("/Binaries/");
	if (binPos == std::string::npos) {
		// Fallback: try going up two levels from the executable
		size_t lastSlash = path.rfind('/');
		if (lastSlash != std::string::npos) {
			binPos = path.rfind('/', lastSlash - 1);
		}
	}

	std::string cookedPCPath;
	if (binPos != std::string::npos) {
		std::string gameRoot = path.substr(0, binPos);
		// Try common casing variants for TgGame/tggame
		std::string candidates[] = {
			gameRoot + "/TgGame/CookedPC",
			gameRoot + "/tggame/CookedPC",
			gameRoot + "/TgGame/cookedpc",
			gameRoot + "/tggame/cookedpc",
		};
		for (const auto& candidate : candidates) {
			DIR* test = opendir(candidate.c_str());
			if (test) {
				closedir(test);
				cookedPCPath = candidate;
				break;
			}
		}
	}

	if (cookedPCPath.empty()) {
		Logger::Log("packagemap", "[PackageMap] Could not find CookedPC directory (path=%s)\n", path.c_str());
		return;
	}

	Logger::Log("packagemap", "[PackageMap] CookedPC path: %s\n", cookedPCPath.c_str());
	ScanDirectory(cookedPCPath, s_packageFileMap);
	Logger::Log("packagemap", "[PackageMap] Built file map: %zu packages indexed\n", s_packageFileMap.size());
}

bool MarshalChannel__NotifyControlMessage::ReadGuidFromPackageFile(const char* filePath, FGuid& outGuid) {
	FILE* f = fopen(filePath, "rb");
	if (!f) return false;

	// UE3 FPackageFileSummary serialization order (UnLinker.cpp):
	//   Tag(4), FileVersion(4), TotalHeaderSize(4),
	//   FString FolderName (INT length + length bytes),
	//   PackageFlags(4),
	//   NameCount(4), NameOffset(4), ExportCount(4), ExportOffset(4),
	//   ImportCount(4), ImportOffset(4), DependsOffset(4),
	//   [if version >= 623: ImportExportGuidsOffset(4), ImportGuidsCount(4), ExportGuidsCount(4)]
	//   [if version >= 584: ThumbnailTableOffset(4)]
	//   FGuid(16)

	// Read FileVersion
	fseek(f, 4, SEEK_SET); // skip Tag
	int fileVersion = 0;
	if (fread(&fileVersion, 4, 1, f) != 1) { fclose(f); return false; }
	int mainVer = fileVersion & 0xFFFF;

	// Skip to FolderName (offset 12: Tag + FileVersion + TotalHeaderSize)
	fseek(f, 12, SEEK_SET);

	// Read FString: INT length, then length bytes
	int folderNameLen = 0;
	if (fread(&folderNameLen, 4, 1, f) != 1) { fclose(f); return false; }
	if (folderNameLen < 0 || folderNameLen > 4096) {
		Logger::Log("packagemap", "[PackageMap] Bad FolderName length %d in %s\n", folderNameLen, filePath);
		fclose(f);
		return false;
	}
	fseek(f, folderNameLen, SEEK_CUR); // skip FolderName content

	// Skip PackageFlags(4) + NameCount(4) + NameOffset(4) + ExportCount(4) +
	//       ExportOffset(4) + ImportCount(4) + ImportOffset(4) + DependsOffset(4) = 32 bytes
	fseek(f, 32, SEEK_CUR);

	// Version-conditional fields
	if (mainVer >= 623) {
		fseek(f, 12, SEEK_CUR); // ImportExportGuidsOffset + ImportGuidsCount + ExportGuidsCount
	}
	if (mainVer >= 584) {
		fseek(f, 4, SEEK_CUR); // ThumbnailTableOffset
	}

	if (fread(&outGuid, sizeof(FGuid), 1, f) != 1) { fclose(f); return false; }
	fclose(f);
	return true;
}

bool MarshalChannel__NotifyControlMessage::ReadGuidAndGenerationsFromPackageFile(
		const char* filePath, FGuid& outGuid, std::vector<int>& outNetObjectCounts) {
	FILE* f = fopen(filePath, "rb");
	if (!f) return false;

	// Same header parsing as ReadGuidFromPackageFile
	fseek(f, 4, SEEK_SET);
	int fileVersion = 0;
	if (fread(&fileVersion, 4, 1, f) != 1) { fclose(f); return false; }
	int mainVer = fileVersion & 0xFFFF;

	fseek(f, 12, SEEK_SET);
	int folderNameLen = 0;
	if (fread(&folderNameLen, 4, 1, f) != 1) { fclose(f); return false; }
	if (folderNameLen < 0 || folderNameLen > 4096) { fclose(f); return false; }
	fseek(f, folderNameLen, SEEK_CUR);

	fseek(f, 32, SEEK_CUR); // PackageFlags + 7 INTs
	if (mainVer >= 623) fseek(f, 12, SEEK_CUR);
	if (mainVer >= 584) fseek(f, 4, SEEK_CUR);

	if (fread(&outGuid, sizeof(FGuid), 1, f) != 1) { fclose(f); return false; }

	// Read Generations array: TArray serialized as INT count, then count * FGenerationInfo
	// FGenerationInfo: ExportCount(4), NameCount(4), NetObjectCount(4) = 12 bytes each
	int genCount = 0;
	if (fread(&genCount, 4, 1, f) != 1) { fclose(f); return false; }
	if (genCount < 0 || genCount > 100) { fclose(f); return false; }

	outNetObjectCounts.clear();
	for (int g = 0; g < genCount; g++) {
		int exportCount, nameCount, netObjectCount;
		if (fread(&exportCount, 4, 1, f) != 1) { fclose(f); return false; }
		if (fread(&nameCount, 4, 1, f) != 1) { fclose(f); return false; }
		if (fread(&netObjectCount, 4, 1, f) != 1) { fclose(f); return false; }
		outNetObjectCounts.push_back(netObjectCount);
	}

	fclose(f);
	return true;
}

void MarshalChannel__NotifyControlMessage::FixForcedExportGuids(void* PackageMap) {
	BuildPackageFileMap();

	char* List = *(char**)((char*)PackageMap + 0x3C);
	int ListCount = *(int*)((char*)PackageMap + 0x40);

	// Dump first 3 entries raw to determine actual FPackageInfo layout
	Logger::Log("packagemap", "[PackageMap] List=%p Count=%d stride=0x44\n", List, ListCount);
	for (int i = 0; i < ListCount && i < 3; i++) {
		char* Entry = List + i * 0x44;
		// Dump raw hex of the 0x44-byte entry
		char hexBuf[0x44 * 3 + 1];
		for (int b = 0; b < 0x44; b++) {
			snprintf(hexBuf + b * 3, 4, "%02X ", (unsigned char)Entry[b]);
		}
		Logger::Log("packagemap", "[PackageMap] Entry[%d] raw: %s\n", i, hexBuf);

		// Try to interpret known fields
		FName* pkgName = (FName*)(Entry + 0x00);
		UObject* parent = *(UObject**)(Entry + 0x08);
		FGuid* guid = (FGuid*)(Entry + 0x0C);
		const char* name = parent ? parent->GetName() : "(null)";
		Logger::Log("packagemap", "[PackageMap] Entry[%d] FName.Index=%d Parent=%p Name='%s' GUID=%08X%08X%08X%08X\n",
			i, pkgName->Index, parent, name, guid->A, guid->B, guid->C, guid->D);

		// Log what's at various candidate offsets for ForcedExportBasePackageName
		FName* at34 = (FName*)(Entry + 0x34);
		FName* at38 = (FName*)(Entry + 0x38);
		FName* at3C = (FName*)(Entry + 0x3C);
		Logger::Log("packagemap", "[PackageMap] Entry[%d] FName@+0x34=%d FName@+0x38=%d FName@+0x3C=%d\n",
			i, at34->Index, at38->Index, at3C->Index);
	}

	// Check ForcedExportBasePackageName on the UPackage itself (at UPackage+0x8C),
	// NOT on FPackageInfo (struct layout differs from vanilla UE3 in this game).
	// For packages where the server's GUID (from forced export) differs from the
	// standalone .upk GUID, we fix the GUID AND update GenerationNetObjectCount
	// on the UPackage to match the standalone version.
	int fixed = 0;
	for (int i = 0; i < ListCount; i++) {
		char* Entry = List + i * 0x44;
		UObject* Parent = *(UObject**)(Entry + 0x08);
		if (!Parent) continue;

		// Check ForcedExportBasePackageName on UPackage (offset 0x90 in this build;
		// GenerationNetObjectCount TArray ends at +0x84+12=+0x90)
		FName* forcedBase = (FName*)((char*)Parent + 0x90);
		if (forcedBase->Index == 0) continue; // NAME_None — not a forced export

		const char* pkgName = Parent->GetName();
		if (!pkgName) continue;

		auto it = s_packageFileMap.find(std::string(pkgName));
		if (it == s_packageFileMap.end()) continue;

		FGuid diskGuid;
		std::vector<int> diskNetObjCounts;
		if (!ReadGuidAndGenerationsFromPackageFile(it->second.c_str(), diskGuid, diskNetObjCounts)) continue;

		FGuid* entryGuid = (FGuid*)(Entry + 0x0C);
		if (entryGuid->A != diskGuid.A || entryGuid->B != diskGuid.B ||
			entryGuid->C != diskGuid.C || entryGuid->D != diskGuid.D) {
			// Log server's GenerationNetObjectCount from UPackage
			// UPackage layout: +0x84 = TArray<INT> GenerationNetObjectCount (confirmed from Compute decompilation)
			TArray<int>* serverGenCounts = (TArray<int>*)((char*)Parent + 0x84);
			Logger::Log("packagemap", "[PackageMap] '%s' server GenNetObjCount: Num=%d [", pkgName, serverGenCounts->Num());
			for (int g = 0; g < serverGenCounts->Num(); g++) {
				Logger::Log("packagemap", "%d%s", serverGenCounts->Data[g], g < serverGenCounts->Num()-1 ? ", " : "");
			}
			Logger::Log("packagemap", "]\n");

			// Log disk file's GenerationNetObjectCount for comparison
			Logger::Log("packagemap", "[PackageMap] '%s' disk   GenNetObjCount: Num=%zu [", pkgName, diskNetObjCounts.size());
			for (size_t g = 0; g < diskNetObjCounts.size(); g++) {
				Logger::Log("packagemap", "%d%s", diskNetObjCounts[g], g < diskNetObjCounts.size()-1 ? ", " : "");
			}
			Logger::Log("packagemap", "]\n");

			Logger::Log("packagemap", "[PackageMap] Fixing GUID for forced export '%s': %08X%08X%08X%08X -> %08X%08X%08X%08X\n",
				pkgName,
				entryGuid->A, entryGuid->B, entryGuid->C, entryGuid->D,
				diskGuid.A, diskGuid.B, diskGuid.C, diskGuid.D);
			*entryGuid = diskGuid;

			// Also update the UPackage's own Guid (at UPackage+0x5C) so that
			// PackageMap::Compute uses consistent data.
			FGuid* pkgGuid = (FGuid*)((char*)Parent + 0x5C);
			*pkgGuid = diskGuid;

			// Clear ForcedExportBasePackageName so the client loads standalone version
			forcedBase->Index = 0;

			// Fix PackageFlags in FPackageInfo to match standalone (524297 = 0x80009)
			int32_t* flags = (int32_t*)(Entry + 0x2C);
			Logger::Log("packagemap", "[PackageMap] '%s' flags: 0x%X -> 0x80009\n", pkgName, *flags);
			*flags = 0x80009;

			// Fix GenerationNetObjectCount on the UPackage to match the standalone file.
			// Without this, Compute produces wrong object indices (off by the count difference).
			// TArray<INT> at UPackage+0x84: {INT* Data, INT Num, INT Max}
			if (!diskNetObjCounts.empty()) {
				int serverNum = serverGenCounts->Num();
				int diskNum = (int)diskNetObjCounts.size();
				if (serverNum == diskNum) {
					for (int g = 0; g < diskNum; g++) {
						if (serverGenCounts->Data[g] != diskNetObjCounts[g]) {
							Logger::Log("packagemap", "[PackageMap] '%s' GenNetObjCount[%d]: %d -> %d\n",
								pkgName, g, serverGenCounts->Data[g], diskNetObjCounts[g]);
							serverGenCounts->Data[g] = diskNetObjCounts[g];
						}
					}
				} else {
					Logger::Log("packagemap", "[PackageMap] '%s' GenNetObjCount size mismatch: server=%d disk=%d (cannot fix)\n",
						pkgName, serverNum, diskNum);
				}
			}

			fixed++;
		}
	}

	if (fixed > 0) {
		Logger::Log("packagemap", "[PackageMap] Fixed %d forced-export package(s)\n", fixed);
	}
}

