#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>
#include <cstring>
#include <string>

// =============================================================================
// asm_data_set_msg_translations IDs (verified against server.db, English)
// =============================================================================

// Mission timer (all modes)
static constexpr int MSG_60S_REMAINING        = 0x5A1A;  // "60 seconds remaining!"
static constexpr int MSG_30S_REMAINING        = 0x5A2C;  // "30 seconds remaining!"
static constexpr int MSG_10S_REMAINING        = 0x5A2D;  // "10 seconds remaining!"

// Ticket control + 90%
static constexpr int MSG_YOUR_TEAM_CONTROL    = 0x6C3B;  // "Your team has taken control!"
static constexpr int MSG_ENEMY_TEAM_CONTROL   = 0x6C3A;  // "The enemy has taken control!"
static constexpr int MSG_YOUR_TEAM_90PCT      = 0x988E;  // "You[r] team has 90 percent control."
static constexpr int MSG_ENEMY_TEAM_90PCT     = 0x988F;  // "The enemy team has 90 percent control."

// PointRotation lead alerts (immediate on score change)
static constexpr int MSG_YOUR_TEAM_LEAD       = 0x9890;  // "Your team has taken the lead."
static constexpr int MSG_ENEMY_TEAM_LEAD      = 0x9891;  // "The enemy team has taken the lead."
static constexpr int MSG_TIE_FOR_CONTROL      = 0x9892;  // "There is a tie for control."

// PointRotation rotation banner (fired 20s before next activation)
static constexpr int MSG_POINT_ROTATION_START = 0x79C1;  // "Point Rotation Started" (first rotation only)
static constexpr int MSG_POINT_CHANGING       = 0x79C2;  // "Point Changing"        (subsequent rotations)

// PointRotation activation countdown + activation
static constexpr int MSG_15S_TO_ACTIVATION    = 0x7032;  // "15 Seconds to Point Activation."
static constexpr int MSG_10S_TO_ACTIVATION    = 0x79BD;  // "10 Seconds to Point Activation."
static constexpr int MSG_5S_TO_ACTIVATION     = 0x79BE;  // "5 Seconds to Point Activation."
static constexpr int MSG_OBJECTIVE_ACTIVATED  = 0x7033;  // "Objective Location Activated"

// AlertPriority / AlertType — see TgObject.uc enums.
static constexpr unsigned char APT_NORMAL   = 1;
static constexpr unsigned char APT_HIGH     = 2;
static constexpr unsigned char APT_CRITICAL = 3;
static constexpr unsigned char ATT_REGULAR     = 0;
static constexpr unsigned char ATT_BENEFICIAL  = 1;
static constexpr unsigned char ATT_DETRIMENTAL = 2;
static constexpr unsigned char ATT_IMPORTANT   = 3;

// Rotation banner is shown this many seconds before the next activation.
static constexpr float ROTATION_BANNER_LEAD_SECS = 20.0f;

// =============================================================================
// Per-Game state
// =============================================================================
struct PerGameState {
	// Mission timer thresholds (latched: only fire once per mission)
	bool firedT60 = false;
	bool firedT30 = false;
	bool firedT10 = false;

	// Ticket — tracks whether each side currently has "map control" (≥2 captured)
	bool attHasControl = false;
	bool defHasControl = false;

	// Ticket — 90% milestone latch (per side)
	bool fired90Att = false;
	bool fired90Def = false;

	// PointRotation — lead tracking (1=att leads, 2=def leads, 0=tied, -1=unseeded)
	int  lastLeader = -1;

	// PointRotation — rotation cycle (set by NotifyNextObjectiveScheduled)
	bool  bFirstRotation = true;            // becomes false after first MSG_OBJECTIVE_ACTIVATED
	float nextActivationAtTime = 0.0f;      // WorldInfo.TimeSeconds for next activation; 0 = none scheduled
	bool  firedBanner = false;              // "Point Rotation Started" / "Point Changing"
	bool  firedAct15 = false;
	bool  firedAct10 = false;
	bool  firedAct5  = false;
	bool  firedActivated = false;

	// Successor pre-warm — one-shot per mission. Once the trigger condition
	// has fired we've already asked the control server to spin up the next
	// instance; further ticks are no-ops until the per-Game state map is
	// reset (Reset() / process exit). Sharing the same s_state map as the
	// announcer alerts keeps the throttle + class-discriminator code reuse.
	bool firedSuccessor = false;

	// Throttle: only run Tick() at most once per second of WorldInfo time
	float lastTickedAt = -1.0f;
};

static std::unordered_map<void*, PerGameState> s_state;

static PerGameState& StateFor(void* Game) {
	auto it = s_state.find(Game);
	if (it == s_state.end()) {
		auto inserted = s_state.emplace(Game, PerGameState{});
		return inserted.first->second;
	}
	return it->second;
}

// =============================================================================
// Helpers
// =============================================================================
static AWorldInfo* GetWorldInfoSafe() {
	if (Globals::Get().GWorld == nullptr) return nullptr;
	return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

static bool ClassNameContains(UObject* Obj, const char* needle) {
	if (Obj == nullptr || Obj->Class == nullptr) return false;
	const char* name = Obj->Class->GetFullName();
	return name && strstr(name, needle) != nullptr;
}

// Counts how many objectives in m_MissionObjectives are captured by `tf`.
// Gated on r_bHasBeenCapturedOnce so the map-default owner isn't counted
// before any real capture.
static int CountCapturedFor(ATgRepInfo_Game* GRI, int tf) {
	if (GRI == nullptr) return 0;
	int n = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr) continue;
		if (!Obj->r_bHasBeenCapturedOnce) continue;
		if (Obj->r_nOwnerTaskForce == tf) n++;
	}
	return n;
}

// =============================================================================
// Per-mode pollers
// =============================================================================

// "60 / 30 / 10 seconds remaining" — every mission, only during Mission Phase
// (m_eTimerState == 2). Setup (1) and Overtime (3) are excluded.
static void PollMissionTimer(ATgGame* Game, ATgRepInfo_Game* GRI, PerGameState& st) {
	AWorldInfo* WI = GetWorldInfoSafe();
	if (WI == nullptr) return;
	if ((int)Game->m_eTimerState != 2) return;

	const float now = WI->TimeSeconds;
	const float endsAt = Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime;
	const float remaining = endsAt - now;
	if (remaining <= 0.0f || remaining > 90.0f) return;

	auto fire = [&](int msgId, unsigned char type) {
		SendAlert::Broadcast(msgId, APT_HIGH, type, /*duration=*/3.0f);
	};

	if (!st.firedT60 && remaining <= 60.5f) {
		st.firedT60 = true;
		fire(MSG_60S_REMAINING, ATT_IMPORTANT);
		Logger::Log("announcer", "MissionTimer: 60s alert (remaining=%.1f)\n", remaining);

		// Infinite-loop maps: the engine's match-end is gated by a UC
		// SetTimer('MissionTimer', m_fGameMissionTime, false) armed at match start
		// in MissionTimerStart(). Just rewriting m_fGameMissionTime /
		// s_fMissionTimerStartedAt is cosmetic — the pending one-shot timer still
		// fires at its original time. Re-arm it via the same UC path
		// MissionTimerStart() uses. Effective behaviour: a "60 seconds remaining"
		// announcement every ~14 minutes, followed by 15 fresh minutes.
		if (Config::GetMapNameChar() == "Dome3_VR_Arena_P") {
			Game->s_fMissionTimerStartedAt = now;
			Game->m_fGameMissionTime       = 15.0f * 60.0f;
			Game->m_fPausedAtTime          = 0.0f;
			Game->eventSetMissionTime(15.0f * 60.0f);  // sets m_fMissionTime
			Game->eventMissionTimerStart();            // ClearTimer + SetTimer + HUD notify
			st.firedT60 = false;  // re-arm for the next cycle
			st.firedT30 = false;
			st.firedT10 = false;
			Logger::Log("announcer", "MissionTimer: Dome3_VR_Arena_P — timer reset to 15min after 60s alert\n");
		}
	}
	if (!st.firedT30 && remaining <= 30.5f) {
		st.firedT30 = true;
		fire(MSG_30S_REMAINING, ATT_IMPORTANT);
		Logger::Log("announcer", "MissionTimer: 30s alert (remaining=%.1f)\n", remaining);
	}
	if (!st.firedT10 && remaining <= 10.5f) {
		st.firedT10 = true;
		fire(MSG_10S_REMAINING, ATT_IMPORTANT);
		Logger::Log("announcer", "MissionTimer: 10s alert (remaining=%.1f)\n", remaining);
	}
}

// Ticket: control gained/lost on 2-point threshold + 90% milestone.
static void PollTicket(ATgGame* Game, ATgRepInfo_Game* GRI, PerGameState& st) {
	if (!GTeamsData.Attackers || !GTeamsData.Defenders) return;
	if (GRI->r_nMissionTimerState != 1) return;

	const int ownedAtt = CountCapturedFor(GRI, 1);
	const int ownedDef = CountCapturedFor(GRI, 2);
	const bool attControl = (ownedAtt >= 2);
	const bool defControl = (ownedDef >= 2);

	if (attControl && !st.attHasControl) {
		SendAlert::BroadcastToTaskforce(GTeamsData.Attackers, MSG_YOUR_TEAM_CONTROL,  APT_HIGH, ATT_BENEFICIAL,  3.0f);
		SendAlert::BroadcastToTaskforce(GTeamsData.Defenders, MSG_ENEMY_TEAM_CONTROL, APT_HIGH, ATT_DETRIMENTAL, 3.0f);
		Logger::Log("announcer", "Ticket: attackers gained map control (owned=%d)\n", ownedAtt);
	}
	if (defControl && !st.defHasControl) {
		SendAlert::BroadcastToTaskforce(GTeamsData.Defenders, MSG_YOUR_TEAM_CONTROL,  APT_HIGH, ATT_BENEFICIAL,  3.0f);
		SendAlert::BroadcastToTaskforce(GTeamsData.Attackers, MSG_ENEMY_TEAM_CONTROL, APT_HIGH, ATT_DETRIMENTAL, 3.0f);
		Logger::Log("announcer", "Ticket: defenders gained map control (owned=%d)\n", ownedDef);
	}
	st.attHasControl = attControl;
	st.defHasControl = defControl;

	// 90% milestone.
	const int target = GRI->r_nPointsToWin;
	if (target > 0) {
		const int threshold = (target * 9) / 10;
		const int att = GTeamsData.Attackers->r_nCurrentPointCount;
		const int def = GTeamsData.Defenders->r_nCurrentPointCount;

		if (!st.fired90Att && att >= threshold) {
			st.fired90Att = true;
			SendAlert::BroadcastToTaskforce(GTeamsData.Attackers, MSG_YOUR_TEAM_90PCT,  APT_HIGH, ATT_BENEFICIAL,  4.0f);
			SendAlert::BroadcastToTaskforce(GTeamsData.Defenders, MSG_ENEMY_TEAM_90PCT, APT_HIGH, ATT_DETRIMENTAL, 4.0f);
			Logger::Log("announcer", "Ticket: attackers at 90%% (%d/%d)\n", att, target);
		} else if (st.fired90Att && att < threshold) {
			st.fired90Att = false;
		}
		if (!st.fired90Def && def >= threshold) {
			st.fired90Def = true;
			SendAlert::BroadcastToTaskforce(GTeamsData.Defenders, MSG_YOUR_TEAM_90PCT,  APT_HIGH, ATT_BENEFICIAL,  4.0f);
			SendAlert::BroadcastToTaskforce(GTeamsData.Attackers, MSG_ENEMY_TEAM_90PCT, APT_HIGH, ATT_DETRIMENTAL, 4.0f);
			Logger::Log("announcer", "Ticket: defenders at 90%% (%d/%d)\n", def, target);
		} else if (st.fired90Def && def < threshold) {
			st.fired90Def = false;
		}
	}
}

// PointRotation lead detection. Fires the lead alert IMMEDIATELY on score
// change — no follow-up here; "Point Changing" is timed separately by the
// rotation poller. Routes per-recipient using the actual TF pointer so
// /changeteam etc. is honoured.
static void PollPointRotationLead(ATgGame* Game, ATgRepInfo_Game* GRI, PerGameState& st) {
	if (!GTeamsData.Attackers || !GTeamsData.Defenders) return;
	if (GRI->r_nMissionTimerState != 1) return;

	const int att = GTeamsData.Attackers->r_nCurrentPointCount;
	const int def = GTeamsData.Defenders->r_nCurrentPointCount;
	int leader = (att == def) ? 0 : (att > def ? 1 : 2);

	if (st.lastLeader == -1) {
		st.lastLeader = leader;
		return;
	}
	if (leader == st.lastLeader) return;

	if (leader == 0) {
		SendAlert::Broadcast(MSG_TIE_FOR_CONTROL, APT_HIGH, ATT_REGULAR, 3.0f);
		Logger::Log("announcer", "PointRotation: tied (%d-%d)\n", att, def);
	} else {
		ATgRepInfo_TaskForce* leadingTF = (leader == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
		ATgRepInfo_TaskForce* losingTF  = (leader == 1) ? GTeamsData.Defenders : GTeamsData.Attackers;
		SendAlert::BroadcastToTaskforce(leadingTF, MSG_YOUR_TEAM_LEAD,  APT_HIGH, ATT_BENEFICIAL,  3.0f);
		SendAlert::BroadcastToTaskforce(losingTF,  MSG_ENEMY_TEAM_LEAD, APT_HIGH, ATT_DETRIMENTAL, 3.0f);
		Logger::Log("announcer", "PointRotation: %s take the lead (%d-%d)\n",
			leader == 1 ? "attackers" : "defenders", att, def);
	}
	st.lastLeader = leader;
}

// PointRotation rotation cycle — drives the banner, the 15/10/5s countdowns,
// and the activation alert. All keyed off `nextActivationAtTime` (planted by
// NotifyNextObjectiveScheduled). Activation is poller-driven (not tied to the
// UnlockObjective hook) to avoid races with our own countdown latch clears.
static void PollPointRotationCycle(ATgGame* Game, PerGameState& st) {
	if (st.nextActivationAtTime <= 0.0f) return;
	AWorldInfo* WI = GetWorldInfoSafe();
	if (WI == nullptr) return;

	const float remaining = st.nextActivationAtTime - WI->TimeSeconds;

	// Rotation banner: 20s before next activation.
	if (!st.firedBanner && remaining <= ROTATION_BANNER_LEAD_SECS + 0.5f) {
		st.firedBanner = true;
		const int bannerMsgId = st.bFirstRotation ? MSG_POINT_ROTATION_START : MSG_POINT_CHANGING;
		SendAlert::Broadcast(bannerMsgId, APT_HIGH, ATT_IMPORTANT, 3.0f);
		Logger::Log("announcer", "PointRotation: %s banner (remaining=%.1f, firstRotation=%d)\n",
			st.bFirstRotation ? "ROTATION_STARTED" : "POINT_CHANGING", remaining, (int)st.bFirstRotation);
	}

	// Countdown — all High/Important now.
	if (!st.firedAct15 && remaining <= 15.5f) {
		st.firedAct15 = true;
		SendAlert::Broadcast(MSG_15S_TO_ACTIVATION, APT_HIGH, ATT_IMPORTANT, 3.0f);
		Logger::Log("announcer", "PointRotation: 15s to activation (remaining=%.1f)\n", remaining);
	}
	if (!st.firedAct10 && remaining <= 10.5f) {
		st.firedAct10 = true;
		SendAlert::Broadcast(MSG_10S_TO_ACTIVATION, APT_HIGH, ATT_IMPORTANT, 3.0f);
		Logger::Log("announcer", "PointRotation: 10s to activation (remaining=%.1f)\n", remaining);
	}
	if (!st.firedAct5 && remaining <= 5.5f) {
		st.firedAct5 = true;
		SendAlert::Broadcast(MSG_5S_TO_ACTIVATION, APT_HIGH, ATT_IMPORTANT, 3.0f);
		Logger::Log("announcer", "PointRotation: 5s to activation (remaining=%.1f)\n", remaining);
	}

	// Activation — fire once, then close the cycle. Flip bFirstRotation
	// immediately on activation (not later) so that even if CalcNextObjective
	// fires before our cleanup tick, the next banner correctly says
	// "Point Changing" instead of "Point Rotation Started".
	if (!st.firedActivated && remaining <= 0.5f) {
		st.firedActivated = true;
		st.bFirstRotation = false;
		SendAlert::Broadcast(MSG_OBJECTIVE_ACTIVATED, APT_HIGH, ATT_IMPORTANT, 4.0f);
		Logger::Log("announcer", "PointRotation: Objective Location Activated (remaining=%.1f)\n", remaining);
	}

	// Stop polling once the cycle is done.
	if (st.firedActivated && remaining < -1.0f) {
		st.nextActivationAtTime = 0.0f;
	}
}

// Successor pre-warm trigger. Per-mission one-shot: when ANY trigger condition
// matches, fires MSG_REQUEST_SUCCESSOR over IPC so the control server can spin
// up a DRAFTING successor instance ahead of the current mission ending.
// Conditions (verified per `tig --message` from the user):
//   - All modes: <=60s remaining on the mission timer (universal fallback)
//   - TgGame_PointRotation: any taskforce's r_nCurrentPointCount >= 2
//   - TgGame_Escort:        attackers (tf=1) have captured >= 2 objectives
//   - TgGame_Mission:       attackers (tf=1) have captured >= total - 1
//                           (i.e. "all but one" — works on 2pt + 3pt maps)
//   - TgGame_Ticket:        either side's r_nCurrentPointCount >= 90% of
//                           r_nPointsToWin (same threshold the existing
//                           "90 percent control" announcer uses)
//
// The control server checks the parent instance's queue config and refuses
// the spawn if the queue isn't opted into continue_in_queue, so it's safe
// to fire from any mode/queue without the DLL having to know which queue
// it was spawned for.
static void PollSuccessorTrigger(ATgGame* Game, ATgRepInfo_Game* GRI, PerGameState& st) {
	if (st.firedSuccessor) return;

	bool fire = false;
	const char* reason = nullptr;

	// 1) Universal — mission phase only, within last 60s of regulation time.
	//    Overtime (state 3) is intentionally excluded because the mission has
	//    already overrun and we don't have a clean "remaining" to measure.
	if ((int)Game->m_eTimerState == 2) {
		AWorldInfo* WI = GetWorldInfoSafe();
		if (WI != nullptr) {
			const float remaining = (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime) - WI->TimeSeconds;
			if (remaining > 0.0f && remaining <= 60.0f) {
				fire = true; reason = "1min-remaining";
			}
		}
	}

	// 2) Mode-specific score/capture triggers. ClassNameContains uses strstr
	//    on the class object's FullName; since TgGame_Mission / _Escort /
	//    _Ticket / _PointRotation are pairwise non-substrings of each other,
	//    these branches don't false-match between subclasses.
	if (!fire) {
		if (ClassNameContains((UObject*)Game, "TgGame_PointRotation")) {
			const int att = (GTeamsData.Attackers != nullptr) ? GTeamsData.Attackers->r_nCurrentPointCount : 0;
			const int def = (GTeamsData.Defenders != nullptr) ? GTeamsData.Defenders->r_nCurrentPointCount : 0;
			if (att >= 2 || def >= 2) {
				fire = true;
				reason = (att >= 2 && def >= 2) ? "PointRotation both ≥2"
				       : (att >= 2)             ? "PointRotation att ≥2"
				                                : "PointRotation def ≥2";
			}
		}
		else if (ClassNameContains((UObject*)Game, "TgGame_Ticket")) {
			const int target = GRI->r_nPointsToWin;
			if (target > 0 && GTeamsData.Attackers != nullptr && GTeamsData.Defenders != nullptr) {
				const int threshold = (target * 9) / 10;  // 90%, integer math
				const int att = GTeamsData.Attackers->r_nCurrentPointCount;
				const int def = GTeamsData.Defenders->r_nCurrentPointCount;
				if (att >= threshold || def >= threshold) {
					fire = true;
					reason = (att >= threshold) ? "Ticket att ≥90%" : "Ticket def ≥90%";
				}
			}
		}
		else if (ClassNameContains((UObject*)Game, "TgGame_Escort")) {
			if (CountCapturedFor(GRI, 1) >= 2) {
				fire = true; reason = "Escort att captured ≥2";
			}
		}
		else if (ClassNameContains((UObject*)Game, "TgGame_Mission")) {
			// Total = count of objectives in the list. The "all but one"
			// rule needs at least 2 objectives to be meaningful (otherwise
			// captured >= total - 1 trips at first capture).
			const int total = GRI->m_MissionObjectives.Count;
			if (total >= 2) {
				const int atk = CountCapturedFor(GRI, 1);
				if (atk >= total - 1) {
					fire = true; reason = "Mission att all-but-one";
				}
			}
		}
	}

	if (fire) {
		st.firedSuccessor = true;
		Logger::Log("successor", "Pre-warm trigger fired (%s) — sending REQUEST_SUCCESSOR\n",
			reason ? reason : "?");
		IpcClient::SendRequestSuccessor();
	}
}

// =============================================================================
// Public entry points
// =============================================================================

void MissionAlerts::Tick() {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game == nullptr) return;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) return;

	AWorldInfo* WI = GetWorldInfoSafe();
	if (WI == nullptr) return;

	PerGameState& st = StateFor((void*)Game);

	if (st.lastTickedAt >= 0.0f && (WI->TimeSeconds - st.lastTickedAt) < 1.0f) return;
	st.lastTickedAt = WI->TimeSeconds;

	PollMissionTimer(Game, GRI, st);

	const bool isTicket        = ClassNameContains((UObject*)Game, "TgGame_Ticket");
	const bool isPointRotation = ClassNameContains((UObject*)Game, "TgGame_PointRotation");

	if (isTicket) {
		PollTicket(Game, GRI, st);
	}
	if (isPointRotation) {
		PollPointRotationLead(Game, GRI, st);
		PollPointRotationCycle(Game, st);
	}

	// Cross-mode: evaluates universal time-remaining + per-mode capture/score
	// conditions for the continue-in-queue pre-warm. Does its own class
	// discrimination (covers Mission and Escort too, which the announcer
	// pollers above don't), and gates on a per-mission latch so the IPC
	// fires at most once.
	PollSuccessorTrigger(Game, GRI, st);
}

void MissionAlerts::NotifyNextObjectiveScheduled(void* Game, float scheduledAtWorldTime) {
	if (Game == nullptr) return;
	PerGameState& st = StateFor(Game);
	st.nextActivationAtTime = scheduledAtWorldTime;
	st.firedBanner    = false;
	st.firedAct15     = false;
	st.firedAct10     = false;
	st.firedAct5      = false;
	st.firedActivated = false;
}

void MissionAlerts::NotifyObjectiveActivated(void* /*Game*/) {
	// No-op: poller drives the activation alert now, so the UnlockObjective
	// hook calling this is harmless but unnecessary. Kept in the header to
	// avoid breaking the caller.
}

void MissionAlerts::Reset() {
	s_state.clear();
}
