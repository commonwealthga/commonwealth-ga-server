#include "src/GameServer/Matchmaking/SetupRebalanceTrigger/SetupRebalanceTrigger.hpp"

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <unordered_map>

namespace {

// TgGame.GameTimerState (TgGame.uc enum): SETUP = 1, MISSION_RUNNING = 2, ...
constexpr unsigned char TGMTS_SETUP = 1;

// Fire this many seconds before the setup timer reaches zero.
constexpr float kLeadSeconds = 5.0f;

struct State {
	bool fired = false;  // one-shot latch per setup phase
};

std::unordered_map<void*, State> s_state;

AWorldInfo* GetWorldInfoSafe() {
	if (Globals::Get().GWorld == nullptr) return nullptr;
	return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

// Seconds until the engine's 'MissionTimer' fires. During setup the same timer
// is armed (TgGame.uc:MissionTimerStart). Prefer the engine timer (single
// source of truth); fall back to the static end-time field. Mirrors
// MissionAlerts::ComputeRealMissionRemaining.
float SetupRemaining(ATgGame* Game) {
	float remaining = Game->GetRemainingTimeForTimer(FName("MissionTimer"), nullptr);
	if (remaining < 0.0f) {
		AWorldInfo* WI = GetWorldInfoSafe();
		if (WI == nullptr) return -1.0f;
		// During setup, s_fMissionTimerStartedAt holds the absolute setup-END
		// time (StartGameTimer adds fSetupTime to it). So remaining = end - now.
		remaining = Game->s_fMissionTimerStartedAt - WI->TimeSeconds;
	}
	return remaining;
}

} // namespace

void SetupRebalanceTrigger::Tick() {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game == nullptr) return;

	State& st = s_state[(void*)Game];

	// Only meaningful during the setup phase. Leaving setup re-arms the latch
	// for the next match on this Game.
	if ((int)Game->m_eTimerState != (int)TGMTS_SETUP) {
		st.fired = false;
		return;
	}
	if (st.fired) return;

	const float remaining = SetupRemaining(Game);
	if (remaining <= 0.0f || remaining > kLeadSeconds) return;

	st.fired = true;
	IpcClient::SendRequestRebalance();
	Logger::Log("team-balance",
		"[SetupRebalance] setup T-%.1fs — requested rebalance (instance=%lld)\n",
		remaining, (long long)IpcClient::GetInstanceId());
}

void SetupRebalanceTrigger::Reset() {
	s_state.clear();
}
