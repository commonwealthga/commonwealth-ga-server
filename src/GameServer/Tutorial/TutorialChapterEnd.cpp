#include "src/GameServer/Tutorial/TutorialChapterEnd.hpp"

#include <string>

#include "src/Config/Config.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TutorialChapterEnd {

namespace {

constexpr float kFallbackSeconds = 90.0f;
// Agency Zero's end is the player walking to the dropship
// (TriggerVolume -> 'Dropship pickup' matinee -> 'endmission' key ->
// RequestMission) - in the original there is no time limit after the capture.
// 90 s ended the mission under players still fighting near the point (seen
// live: results + quit -> Dome without Bancroft's intro, because the
// client never travelled from Agency Zero by the hand-off). Long safety net only.
constexpr float kAgencyZeroFallbackSeconds = 900.0f;

ATgGame* g_game = nullptr;    // deferred end waiting for RequestMission
float g_deferredAt = 0.0f;
bool g_bypass = false;        // the fallback's own BeginEndMission call
int g_requests = 0;           // ServerOnRequestMission hook calls

float FallbackSeconds() {
	return Config::GetMapNameChar() == "AgencyZero_P" ? kAgencyZeroFallbackSeconds : kFallbackSeconds;
}

bool IsChapterMap() {
	const std::string m = Config::GetMapNameChar();
	return m == "Adrenaline_P" || m == "Skylark_P" || m == "AgencyZero_P";
}

float Now() {
	if (Globals::Get().GWorld == nullptr) return -1.0f;
	AWorldInfo* wi = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	return wi ? wi->TimeSeconds : -1.0f;
}

}  // namespace

bool DeferEnd(ATgGame* Game, unsigned char winState) {
	if (g_bypass || Game == nullptr || winState != 2 || !IsChapterMap()) return false;
	if (g_game == nullptr) {
		g_game = Game;
		g_deferredAt = Now();
		Logger::Log("endmission",
			"[Tutorial] %s objective win: no mission end, waiting for the map's RequestMission (fallback %.0fs)\n",
			Config::GetMapNameChar().c_str(), FallbackSeconds());
	}
	return true;
}

void OnRequestMission() {
	g_requests++;
	if (g_game == nullptr) return;
	Logger::Log("endmission", "[Tutorial] RequestMission after the objective win - chapter hand-off (%.1fs)\n",
		Now() - g_deferredAt);
	g_game = nullptr;
}

int RequestCount() { return g_requests; }

void AfterKismetRequestMission(UObject* Action, int countBefore) {
	if (g_requests != countBefore || !IsChapterMap()) return;
	int n = 0;
	for (auto& kv : GClientConnectionsData) {
		ATgPawn_Character* pawn = kv.second.Pawn;
		if (pawn == nullptr || pawn->bDeleteMe) continue;
		TgPawn__ServerOnRequestMission::Call((ATgPawn*)pawn, nullptr, (UTgSeqAct_RequestMission*)Action);
		n++;
	}
	Logger::Log("endmission", "[Tutorial] Kismet RequestMission reached no pawn handler - ran it for %d player(s)\n", n);
}

void Tick() {
	if (g_game == nullptr) return;
	const float now = Now();
	if (now < 0.0f || now - g_deferredAt < FallbackSeconds()) return;
	ATgGame* game = g_game;
	g_game = nullptr;
	Logger::Log("endmission", "[Tutorial] no RequestMission %.0fs after the objective win - normal mission end\n",
		now - g_deferredAt);
	g_bypass = true;
	BeginEndMissionImpl(game, nullptr, 0.0f);
	g_bypass = false;
}

}  // namespace TutorialChapterEnd
