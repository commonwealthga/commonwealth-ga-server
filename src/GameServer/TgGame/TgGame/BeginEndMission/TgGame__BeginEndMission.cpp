#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <cstring>

// Stripped native — both `TgGame::BeginEndMission` and the `TgGame_Ticket`
// override are `_notimplemented` stubs in this binary. UC `TgGame.uc` calls
// `BeginEndMission()` from ObjectiveTaken, MissionTimer-expired, and round-end
// branches expecting the engine to (a) flip every PC into `state RoundEnded`
// so the HUD opens its Scaleform end-mission scene and (b) schedule a
// follow-up `FinishEndMission`. With the natives no-op'd, the screen never
// opens.
//
// Phase 1 here: reuse UC's own `event AllPlayersEndGame(Camera)` (TgGame.uc:462)
// — it already iterates Controllers and calls `GameHasEnded(Camera)`, which is
// the base UC native that transitions the PC to RoundEnded. We pre-broadcast
// `SendClientSetGameWinState` to seed `c_GameWinState` so RoundEnded.BeginState
// reads the right banner (Attackers/Defenders/Tie), then ProcessEvent the
// AllPlayersEndGame event and arm a 20s timer for FinishEndMission.
//
// Reward-details (TCP opcode 0x012C GIVE_REWARD_DETAILS) is intentionally not
// sent here — Phase 2 work, requires reverse-engineering ATgHUD::vftable[247]
// to learn the wire format. The end-mission UI still opens with empty reward
// panels, which is good enough to verify the screen pipeline.
namespace {

constexpr float kFinishDelaySeconds = 20.0f;  // Scaleform end-screen budget.

// Last-chance winner derivation when m_GameWinState wasn't already set by the
// caller's game-type-specific logic. Objective modes (CTF/Control/Defense/
// PointRotation) set it in UC ObjectiveTaken; Ticket sets it in its own
// BeginEndMission hook before delegating to us. This path is a tie/null
// fallback for unexpected entry points only.
//   1 = defenders win, 2 = attackers win, 3 = tie (matches client UI enum).
unsigned char DeriveWinStateFromGRI(ATgRepInfo_Game* GRI) {
	if (GRI && GRI->r_Winner) {
		// r_eCoalition: 1 = Attacker, 2 = Defender.
		if (GRI->r_Winner->r_eCoalition == 2) return 1;
		if (GRI->r_Winner->r_eCoalition == 1) return 2;
	}
	return 3;
}

void BroadcastGameWinState(ATgGame* Game, unsigned char winState) {
	AWorldInfo* WI = Game->WorldInfo;
	if (!WI) return;
	for (AController* C = WI->ControllerList; C != nullptr; C = C->NextController) {
		if (!C || !C->Class) continue;
		const char* clsName = C->Class->GetFullName();
		if (!clsName || !strstr(clsName, "PlayerController")) continue;
		ATgPlayerController* PC = (ATgPlayerController*)C;
		PC->eventSendClientSetGameWinState(winState);
	}
}

}  // namespace

void BeginEndMissionImpl(ATgGame* Game, ACameraActor* endMissionCamera) {
	if (Game == nullptr) return;
	if (Game->s_bGameEndMissionProcessed) {
		Logger::Log("endmission",
			"BeginEndMission — already processed, skipping double-fire\n");
		return;
	}
	Game->s_bGameEndMissionProcessed = 1;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	unsigned char winState = Game->m_GameWinState;
	if (winState == 0) {
		winState = DeriveWinStateFromGRI(GRI);
		Game->m_GameWinState = winState;
	}

	Logger::Log("endmission",
		"BeginEndMission fired — winState=%u (1=Def,2=Att,3=Tie) winner=%p camera=%p\n",
		(unsigned)winState,
		(void*)(GRI ? GRI->r_Winner : nullptr),
		(void*)endMissionCamera);

	// Engine-level "match is over" flags. Two purposes:
	//  - bGameEnded gates UC GameInfo::StartHumans/StartBots so pawns don't
	//    keep getting auto-respawned for the duration of the end-mission
	//    screen. Without this, every PC death/dropped-pawn re-enters the
	//    spawn pipeline.
	//  - bMatchIsOver is replicated (V2 rep list entry already wired at
	//    Actor__GetOptimizedRepListV2.cpp:2529) → client repnotify fires
	//    `ReplicatedEvent('bMatchIsOver')` (GameReplicationInfo.uc:107)
	//    which winds down client-side HUD/audio.
	//  - bStopCountDown freezes the HUD mission timer cosmetically.
	//
	// NOTE: we deliberately do NOT mirror stock `TgGame::EndGame` (TgGame.uc:744)
	// which calls `GotoState('GameOver')`. That state's `Timer()` eventually
	// calls `RestartGame()` → `ServerTravel`, which would kill the end-mission
	// screen and leave the instance prematurely. Our FinishEndMission timer is
	// the only thing that should drive the next phase.
	Game->bGameEnded = 1;
	if (GRI) {
		GRI->bMatchIsOver       = 1;
		GRI->bStopCountDown     = 1;
		GRI->bNetDirty          = 1;
		GRI->bForceNetUpdate    = 1;
	}

	BroadcastGameWinState(Game, winState);

	// Tell the control server to warm up a home map instance now — the
	// end-mission screen has a single "End Mission" button that fires
	// GSC_CHANGE_INSTANCE{0,0} (return-to-home). If the home instance got
	// reaped for idleness, we want it READY by the time the player clicks.
	// Fire-and-forget; control server no-ops if a home instance already exists.
	{
		nlohmann::json msg;
		msg["type"]        = IpcProtocol::MSG_NEED_HOME_MAP;
		msg["instance_id"] = IpcClient::GetInstanceId();
		IpcClient::Send(msg.dump());
	}

	// Authoritative end-of-mission signal: stamps end_mission_at on the
	// parent row AND promotes any DRAFTING successor to READY in one DB
	// transaction. After this fires, GSC_CHANGE_INSTANCE{0,0} from any
	// player still in this mission consults the parent's end_mission_at +
	// the queue's continue_in_queue config to decide between home vs the
	// successor. Always emit, regardless of queue config — the home-pin
	// reaper and continue routing both depend on it.
	{
		nlohmann::json msg;
		msg["type"]        = IpcProtocol::MSG_MISSION_ENDED;
		msg["instance_id"] = IpcClient::GetInstanceId();
		IpcClient::Send(msg.dump());
	}

	// UC `AllPlayersEndGame` handles per-PC GameHasEnded, training-mission
	// specialization, beacon-freeze, and deployable cleanup. Native call so
	// we don't risk re-entering our own BeginEndMission hook.
	Game->eventAllPlayersEndGame(endMissionCamera);

	// Arm FinishEndMission. UC signature lets callers override the delay;
	// honor it, but default to 20s (Scaleform end-screen runtime).
	Actor__SetTimer::SetTimer(
		Game, kFinishDelaySeconds, /*bLoop=*/false,
		FName("FinishEndMission"), nullptr);

	Logger::Log("endmission",
		"BeginEndMission — AllPlayersEndGame dispatched, FinishEndMission armed for %.1fs\n",
		kFinishDelaySeconds);
}

bool __fastcall TgGame__BeginEndMission::Call(ATgGame* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
	LogCallBegin();
	BeginEndMissionImpl(Game, endMissionCamera);
	LogCallEnd();
	return true;
}
