#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CMarshal/GetString/CMarshal__GetString.hpp"
#include "src/GameServer/Misc/CMarshal/GetGuid/CMarshal__GetGuid.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
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
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/Utils/Logger/Logger.hpp"
#include <algorithm>


// Build a marshal-channel control message and send it. Mirrors the wire format
// that triggers our own NotifyControlMessage on the receive side: opcode 0x019B
// + wchar_t string in field 0x4FF. The dispatcher at 0x109f9970 routes
// opcode=0x019B marshals to NotifyControlMessage on both ends, so the same
// shape we receive is the shape we send.
void MarshalChannel__NotifyControlMessage::SendControlMessage(UNetConnection* Connection, const wchar_t* text) {
	if (!Connection || !text) return;
	if ((uintptr_t)Connection < 0x10000) return;

	uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;
	*(uint16_t*)((uint8_t*)Marshal + 0x26) = GA_U::MARSHAL_CHANNEL;  // 0x019B

	CMarshal__SetWcharT::CallOriginal(Marshal, nullptr, GA_T::TEXT_VALUE, (wchar_t*)text);

	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);
	NetConnection__FlushNet::CallOriginal(Connection);
	CMarshal__Destroy::CallOriginal(Marshal);
}

void MarshalChannel__NotifyControlMessage::Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
	LogCallBegin();

	wchar_t local_410[512] = {0};
	CMarshal__GetString::CallOriginal(InBunch, edx, GA_T::TEXT_VALUE, local_410);

	char tmp[1024];
	wcstombs(tmp, local_410, sizeof(tmp));
	tmp[sizeof(tmp)-1] = '\0';

	// TEMPORARY: log every incoming control message in full while we lock down
	// the handshake. Remove (or quiet down) once HELLO/CHALLENGE/NETSPEED/LOGIN
	// parsing has settled.
	Logger::Log("handshake", "[handshake] in: '%s' (conn=%p)\n", tmp, (void*)Connection);

	if (strncmp(tmp, "HELLO", 5) == 0) {
		UUID* guid = new UUID();
		memset(guid, 0, sizeof(UUID));
		CMarshal__GetGuid::CallOriginal(InBunch, edx, 0x473, guid);

		Logger::DumpMemory("session_guid", (void*)guid, 0x10, 0);

		std::string session_guid = SessionGuidToHex(guid);
		GClientConnectionsData[(int)Connection].SessionGuid = session_guid;
		Logger::Log(GetLogChannel(), "[handshake] HELLO: session_guid=%s\n", session_guid.c_str());

		*(uint32_t*)((char*)Connection + 0xF4) = 4869; // Connection->NegotiatedVersion

		// Respond with CHALLENGE. The challenge value isn't validated server-side
		// (vanilla UE3 has gamespy hash logic gated on WITH_GAMESPY; ours doesn't),
		// so any value will do — the important thing is that the client receives
		// a CHALLENGE and then responds with NETSPEED + LOGIN. UE3 uses appCycles()
		// formatted as 8 hex digits.
		uint32_t challenge_val = (uint32_t)GetTickCount() ^ (uint32_t)(uintptr_t)Connection;
		wchar_t challenge_msg[64];
		swprintf(challenge_msg, 64, L"CHALLENGE VER=4869 CHALLENGE=%08X", challenge_val);
		SendControlMessage(Connection, challenge_msg);

		// PackageMap__Compute + WelcomePlayer moved to LOGIN — that's the UE3
		// order (UnWorld.cpp:4961 calls WelcomePlayer only after NMT_Login
		// validates).

	} else if (strncmp(tmp, "NETSPEED", 8) == 0) {
		// Wire format: "NETSPEED <int>" — bare trailing integer, no field tag.
		// Clamp into UE3's [1800, MaxClientRate] range (UnController.cpp:440,
		// UnWorld.cpp:4836).
		int rate = 0;
		const wchar_t* q = local_410 + 8; // past "NETSPEED"
		while (*q && (*q == L' ' || *q == L'\t')) q++;
		if (*q) rate = wcstol(q, nullptr, 10);

		UNetDriver* Driver = *(UNetDriver**)((char*)Connection + 0x70);
		// int MaxRate = Driver ? Driver->MaxClientRate : 50000;
		int MaxRate = 200000;
		int clamped = std::max(1800, std::min(rate > 0 ? rate : MaxRate, MaxRate));
		Connection->CurrentNetSpeed = clamped;
		Logger::Log(GetLogChannel(), "[handshake] NETSPEED: requested=%d clamped=%d (MaxClientRate=%d)\n",
			rate, clamped, MaxRate);

	} else if (strncmp(tmp, "LOGIN", 5) == 0) {
		// Equivalent of UnWorld.cpp:4917 NMT_Login handling: server now welcomes
		// the client into the level. The forced-export GUID fix runs inside
		// NetConnection__SendPackageMap which fires from WelcomePlayer.
		Logger::Log(GetLogChannel(), "[handshake] LOGIN: payload='%s'\n", tmp);

		void* PackageMap = *(void**)((char*)Connection + 0xBC);
		PackageMap__Compute::CallOriginal(PackageMap);
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
			GClientConnectionsData[(int32_t)Connection].PlayerInfo.control_register_token = info->control_register_token;
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
	} else {
		// TEMPORARY: any keyword not recognized above gets a loud log so we can
		// discover the actual wire format the client uses for NETSPEED / LOGIN
		// (current parsers above are best-guesses based on UE3 conventions).
		Logger::Log("handshake", "[handshake] UNKNOWN keyword: '%s'\n", tmp);
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

	if (Logger::IsChannelEnabled("gametimer")) {
		const std::string gameName = ((UObject*)game)->GetFullName();
		const std::string gameClass = game->Class ? game->Class->GetFullName() : "<null-class>";
		const std::string stateName = game->GetStateName().GetName();
		Logger::Log("gametimer",
			"[NotifyControlMessage:HandlePlayerConnected] firstPlayerSpawned=%d game=%s class=%s state=%s "
			"wait=%d delayed=%d ended=%d timerState=%d mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f "
			"GRI{ptr=%p round=%d/%d mtState=%d mtChange=%d rem=%.2f remaining=%d limit=%d matchOver=%d}\n",
			(int)bFirstPlayerSpawned,
			gameName.c_str(),
			gameClass.c_str(),
			stateName.c_str(),
			(int)game->bWaitingToStartMatch,
			(int)game->bDelayedStart,
			(int)game->bGameEnded,
			(int)game->m_eTimerState,
			game->m_fMissionTime,
			game->m_fGameMissionTime,
			game->m_fGameOvertimeTime,
			game->s_fMissionTimerStartedAt,
			gamerep,
			gamerep ? gamerep->r_nRoundNumber : -1,
			gamerep ? gamerep->r_nMaxRoundNumber : -1,
			gamerep ? (int)gamerep->r_nMissionTimerState : -1,
			gamerep ? gamerep->r_nMissionTimerStateChange : -1,
			gamerep ? gamerep->r_fMissionRemainingTime : -1.0f,
			gamerep ? gamerep->RemainingTime : -1,
			gamerep ? gamerep->TimeLimit : -1,
			gamerep ? (int)gamerep->bMatchIsOver : -1);
	}

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

	// bOnlySpectator/bIsSpectator/bOutOfLives are now cleared inside our
	// `Function TgGame.TgGame.Login` ProcessEvent hook, BEFORE SpawnPlayActor's
	// internal eventPostLogin runs. That first PostLogin now spawns the pawn
	// naturally, so we no longer need:
	//   1. the manual `bOnlySpectator = 0` flip here, and
	//   2. the redundant second `game->eventPostLogin(...)` call below.
	// Removing both eliminates the UnPossess+re-Possess transient ViewTarget
	// flip caused by the second Possess and the SpectatingMatch-state race
	// (client's replicated bOnlySpectator landing as TRUE when ClientSetReadyState
	// fires, causing client to GotoState('SpectatingMatch') → BeginState fires
	// ServerSetViewTarget(Pawn=null on client) → server SetViewTarget(none) →
	// null→self conversion in public SetViewTarget → ViewTarget = PC permanently).

	if (GClientConnectionsData[(int32_t)Connection].PlayerInfo.task_force == 1) {
		newcontroller->PlayerReplicationInfo->Team = GTeamsData.Attackers;
	} else {
		newcontroller->PlayerReplicationInfo->Team = GTeamsData.Defenders;
	}

	// game->eventPostLogin(newcontroller);  // removed: now runs inside SpawnPlayActor naturally

	Logger::Log("aim-trace", "RESETFORCEVIEWTARGET pawn = 0x%p\n", newcontroller->Pawn);
	newcontroller->ResetForceViewTarget();
	Logger::Log("aim-trace", "NEW VIEWTARGET = 0x%p\n", newcontroller->ViewTarget);

	// Clear stale SpectatingMatch/cinematic visual state after real TgHUD_Game
	// exists. TgPlayerController's overrides update HUD fade and m_bDrawPawnHUD.
	{
		struct ClearFadeParms {
			uint32_t bEnableFading;
			uint8_t fadeColor[4];
			float fadeAlpha[2];
			float fadeTime;
		} clearFade = {};
		struct CinematicParms {
			uint32_t bInCinematicMode;
			uint32_t bAffectsMovement;
			uint32_t bAffectsTurning;
			uint32_t bAffectsHUD;
		} cinematic = { 0, 0, 0, 1 };

		UFunction* fadeFn = (UFunction*)ObjectCache::Find("Function TgGame.TgPlayerController.ClientSetCameraFade");
		UFunction* cinematicFn = (UFunction*)ObjectCache::Find("Function TgGame.TgPlayerController.ClientSetCinematicMode");
		if (fadeFn) {
			newcontroller->ProcessEvent(fadeFn, &clearFade, nullptr);
		}
		if (cinematicFn) {
			newcontroller->ProcessEvent(cinematicFn, &cinematic, nullptr);
		}
		Logger::Log("spawn",
			"HandlePlayerConnected: cleared client fade/cinematic hud fadeFn=%p cinematicFn=%p pawn=%p viewTarget=%p\n",
			fadeFn, cinematicFn, newcontroller->Pawn, newcontroller->ViewTarget);
	}

	// (Removed: pre-spawned beacon deferral. The new SpawnObject + GetTaskForce
	// reimplementation lets the UC state machine register beacons through their
	// natural lifecycle once the TgRepInfo_TaskForce / BeaconManager chain is
	// up — no NetGUID race because the exit beacon is now spawned on demand by
	// CheckBeacon -> SpawnNewBeaconForTeam, not pre-spawned at PostBeginPlay.)

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

		AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

		if (Logger::IsChannelEnabled("gametimer")) {
			const std::string gameName = ((UObject*)game)->GetFullName();
			const std::string stateName = game->GetStateName().GetName();
			Logger::Log("gametimer",
				"[NotifyControlMessage:first-player-events:before] game=%s state=%s timerState=%d "
				"mission=%.2f gameMission=%.2f round=%d/%d mtState=%d\n",
				gameName.c_str(),
				stateName.c_str(),
				(int)game->m_eTimerState,
				game->m_fMissionTime,
				game->m_fGameMissionTime,
				gamerep ? gamerep->r_nRoundNumber : -1,
				gamerep ? gamerep->r_nMaxRoundNumber : -1,
				gamerep ? (int)gamerep->r_nMissionTimerState : -1);
		}

		TArray<USequenceObject*> Events;
		TArray<int> Indices;
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

		if (Logger::IsChannelEnabled("gametimer")) {
			const std::string gameName = ((UObject*)game)->GetFullName();
			const std::string stateName = game->GetStateName().GetName();
			Logger::Log("gametimer",
				"[NotifyControlMessage:first-player-events:after] game=%s state=%s timerState=%d "
				"mission=%.2f gameMission=%.2f round=%d/%d mtState=%d levelLoadedEvents=%d levelFadedEvents=%d\n",
				gameName.c_str(),
				stateName.c_str(),
				(int)game->m_eTimerState,
				game->m_fMissionTime,
				game->m_fGameMissionTime,
				gamerep ? gamerep->r_nRoundNumber : -1,
				gamerep ? gamerep->r_nMaxRoundNumber : -1,
				gamerep ? (int)gamerep->r_nMissionTimerState : -1,
				Events.Num(),
				Events2.Num());
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
