#include "src/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Tutorial/TutorialChapterEnd.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Config/Config.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"

#include <string>
#include <vector>

// Client-side camera fade on one player. Black at alpha 1 stays up until the
// client travels (HandlePlayerConnected clears it on the next map).
bool TgPawn__ServerOnRequestMission::SetCameraFade(AController* Controller, bool bOn, float Alpha, float Time) {
	if (!Controller || !Controller->Class) return false;
	const char* raw = Controller->Class->GetFullName();
	if (!raw || std::string(raw).find("PlayerController") == std::string::npos) return false;
	UFunction* fadeFn = (UFunction*)ObjectCache::Find("Function TgGame.TgPlayerController.ClientSetCameraFade");
	if (!fadeFn) return false;
	struct {
		uint32_t bEnableFading;
		uint8_t fadeColor[4];   // B G R A
		float fadeAlpha[2];     // from, to
		float fadeTime;
	} parms = { bOn ? 1u : 0u, { 0, 0, 0, 255 }, { Alpha, Alpha }, Time > 0.0f ? Time : 0.05f };
	Controller->ProcessEvent(fadeFn, &parms, nullptr);
	return true;
}

// Controllers held black after RequestMission. The client terminates its own
// copy of the matinee a moment after the server does, and the fade track's
// termination switches camera fading off again - one fade sent at the
// server's end is wiped and the pawn camera (start of the jump) shows until
// the loading screen. Re-send the black fade every 0.1 s until the player is
// gone (travel) or kHoldSeconds pass.
namespace {
	struct HeldFade { AController* Controller; float Left; float NextSend; int Sends; };
	std::vector<HeldFade> g_held;
	// The control server now routes on RequestMission itself; the hold has
	// to last until the next instance is up and the client travels (instance
	// boot ~6-10 s). Ends early when the controller leaves (travel).
	constexpr float kHoldSeconds = 45.0f;
	constexpr float kResendEvery = 0.1f;
	// Always-on one-line log of the hold path (own channel, few lines per
	// map change) so a player's report can be read from the instance log.
	void HoldLog(const char* fmt, AController* C, int a, int b) {
		if (!Logger::IsChannelEnabled("hold")) Logger::EnableChannel("hold");
		Logger::Log("hold", fmt, C, a, b);
	}
}

// AgencyZero_P's RequestMission is the 'endmission' event key at
// 24.78 s of the 50.13 s 'Dropship pickup' matinee (SeqAct_Interp_3 ->
// InterpTrackEvent_0). Holding black + routing at the key cut the second half
// of the closing cinematic and put the results screen over it.
// There the hand-off waits for the matinee's end (+0.4 s); still capped.
namespace {
	struct DeferredRm { ATgPawn* Pawn; AController* Controller; int MapGameId; float Left; };
	std::vector<DeferredRm> g_deferredRm;
	constexpr float kAgencyZeroCineTail = 50.13f - 24.78f + 0.4f;
}
static void DoRequestMissionHandOff(ATgPawn* Pawn, int mapGameId);

void TgPawn__ServerOnRequestMission::Tick(float DeltaSeconds) {
	for (size_t i = 0; i < g_deferredRm.size();) {
		DeferredRm& d = g_deferredRm[i];
		d.Left -= DeltaSeconds;
		if (d.Left > 0.0f) { ++i; continue; }
		DeferredRm done = d;
		g_deferredRm.erase(g_deferredRm.begin() + i);
		bool alive = false;
		AWorldInfo* W = Globals::Get().GWorld ? World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0) : nullptr;
		if (W) for (AController* C = W->ControllerList; C != nullptr; C = C->NextController)
			if (C == done.Controller && C->Pawn == (APawn*)done.Pawn) { alive = true; break; }
		Logger::Log("mission", "[RequestMission] AgencyZero cine tail over: hand-off pawn=%p alive=%d\n", done.Pawn, (int)alive);
		if (alive) DoRequestMissionHandOff(done.Pawn, done.MapGameId);
	}
	if (g_held.empty()) return;
	if (Globals::Get().GWorld == nullptr) { g_held.clear(); return; }
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI == nullptr) { g_held.clear(); return; }
	for (size_t i = 0; i < g_held.size();) {
		HeldFade& h = g_held[i];
		h.Left -= DeltaSeconds;
		h.NextSend -= DeltaSeconds;
		// Only touch controllers still in the world's list (left = destroyed).
		bool alive = false;
		for (AController* C = WI->ControllerList; C != nullptr; C = C->NextController) {
			if (C == h.Controller) { alive = true; break; }
		}
		if (!alive || h.Left <= 0.0f) {
			HoldLog("[Hold] end controller=%p traveled=%d sends=%d\n", h.Controller, (int)!alive, h.Sends);
			g_held.erase(g_held.begin() + i);
			continue;
		}
		if (h.NextSend <= 0.0f) {
			SetCameraFade(h.Controller, true, 1.0f, 0.05f);
			h.NextSend = kResendEvery;
			if (++h.Sends % 50 == 0) HoldLog("[Hold] re-send controller=%p sends=%d left_s=%d\n", h.Controller, h.Sends, (int)h.Left);
		}
		++i;
	}
}

// Kismet TgSeqAct_RequestMission (e.g. the end of the intro jump matinee).
// The native is a backend stub; the client leaves the map on its own. Hold
// the screen black so the matinee's fade does not drop back to the pawn's
// own camera for the moments before the next map's loading screen.
// Leaving the map from a cinematic (intro jump): black camera + re-send
// until travel, and cinematic mode's hidden HUD switched back on - the
// client's map loading screen belongs to the HUD and is not drawn while
// the HUD is hidden (the old view stays frozen through the whole load).
// Movement/look stay blocked.
bool TgPawn__ServerOnRequestMission::HoldBlack(AController* Controller) {
	if (!SetCameraFade(Controller, true, 1.0f, 0.05f)) return false;
	UFunction* cineFn = (UFunction*)ObjectCache::Find("Function TgGame.TgPlayerController.ClientSetCinematicMode");
	if (cineFn) {
		struct { uint32_t bInCinematicMode, bAffectsMovement, bAffectsTurning, bAffectsHUD; } parms = { 0u, 0u, 0u, 1u };
		Controller->ProcessEvent(cineFn, &parms, nullptr);
	}
	bool known = false;
	for (auto& h : g_held) if (h.Controller == Controller) { h.Left = kHoldSeconds; known = true; }
	if (!known) g_held.push_back({ Controller, kHoldSeconds, kResendEvery, 0 });
	HoldLog("[Hold] start controller=%p cine_off=%d renew=%d\n", Controller, cineFn ? 1 : 0, (int)known);
	return true;
}

void __fastcall TgPawn__ServerOnRequestMission::Call(ATgPawn* Pawn, void* edx, UTgSeqAct_RequestMission* Action) {
	LogCallBegin();
	HoldLog("[Hold] RequestMission hook pawn=%p mapGameId=%d session=%d\n", (AController*)Pawn,
		Action ? Action->m_nMapGameId : -1, (int)(Pawn && GPawnSessions.count(Pawn)));
	TutorialChapterEnd::OnRequestMission();
	if (Pawn && Pawn->Controller && Config::GetMapNameChar() == "AgencyZero_P") {
		bool known = false;
		for (auto& d : g_deferredRm) if (d.Pawn == Pawn) known = true;
		if (!known) g_deferredRm.push_back({ Pawn, Pawn->Controller, Action ? Action->m_nMapGameId : 0, kAgencyZeroCineTail });
		Logger::Log("mission", "[RequestMission] AgencyZero: hand-off after the closing cinematic (%.1fs) pawn=%p dup=%d\n",
			kAgencyZeroCineTail, Pawn, (int)known);
	} else {
		DoRequestMissionHandOff(Pawn, Action ? Action->m_nMapGameId : 0);
	}
	CallOriginal(Pawn, edx, Action);
	LogCallEnd();
}

static void DoRequestMissionHandOff(ATgPawn* Pawn, int mapGameId) {
	const bool faded = Pawn && TgPawn__ServerOnRequestMission::HoldBlack(Pawn->Controller);
	Logger::Log("mission", "[RequestMission] pawn=%p mapGameId=%d hold-black=%d\n",
		Pawn, mapGameId, (int)faded);
	// Tell the control server this player finished the map
	// (tutorial chapter chaining). Esc > Quit Tutorial never gets here.
	auto sessIt = Pawn ? GPawnSessions.find(Pawn) : GPawnSessions.end();
	if (sessIt != GPawnSessions.end()) {
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "request_mission";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = sessIt->second;
		ev["map_game_id"]  = mapGameId;
		IpcClient::Send(ev.dump());
	}
}
