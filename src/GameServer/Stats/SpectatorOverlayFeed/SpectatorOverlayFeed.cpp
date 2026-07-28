#include "src/GameServer/Stats/SpectatorOverlayFeed/SpectatorOverlayFeed.hpp"

#include <unordered_map>

#include "lib/nlohmann/json.hpp"

#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ActiveSpectatorCount/ActiveSpectatorCount.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"

namespace SpectatorOverlayFeed {

namespace {

// ~3 pushes/sec per pawn — plenty for a health bar overlay, cheap enough to
// not matter next to everything else Actor::Tick already does per frame.
constexpr DWORD kPushIntervalMs = 300;

// Keyed by r_nPawnId, NOT by ATgPawn* — a pawn pointer gets freed and can be
// reused by an unrelated LATER pawn allocation (dies+respawns, or a totally
// different player's pawn landing at the same freed address). A pointer-keyed
// map would let that new pawn inherit the old one's last-push timestamp,
// wrongly rate-limiting it. r_nPawnId is assigned once per pawn instance via
// TgGame.GetNextPawnId() and never reused within a map's lifetime, so it's
// the stable identity every other per-pawn IPC-correlation map in this
// codebase already keys on. Entries are erased via ForgetPawn() on
// disconnect (NetConnection__Cleanup) rather than left to accumulate.
std::unordered_map<int, DWORD> g_lastPushMs;

// Same cast pattern as TgPawn__SetTaskForceNumber.cpp / ChangeTeam.cpp — the
// engine PlayerReplicationInfo* on a TgPawn is always an ATgRepInfo_Player.
int ResolveTaskForceNumber(ATgPawn* pawn) {
	if (!pawn || !pawn->PlayerReplicationInfo) return 0;
	ATgRepInfo_Player* repinfo = reinterpret_cast<ATgRepInfo_Player*>(pawn->PlayerReplicationInfo);
	auto* tf = repinfo->r_TaskForce;
	if (!tf) return 0;
	if (tf == GTeamsData.Attackers) return 1;
	if (tf == GTeamsData.Defenders) return 2;
	return 0;
}

void CollectEffectIds(TArray<UTgEffectGroup*>& arr, nlohmann::json& out) {
	if (!arr.Data) return;
	for (int i = 0; i < arr.Num(); ++i) {
		UTgEffectGroup* eg = arr.Data[i];
		if (eg) out.push_back(eg->m_nEffectGroupId);
	}
}

} // namespace

void MaybePushSnapshot(AActor* actor) {
	// Nobody is spectating this instance right now — skip everything below
	// entirely rather than gathering/sending data nobody's reading. This is
	// the actual gate: an int compare, cheaper than even the class-name
	// check that used to run unconditionally for every actor every tick.
	// Also means the always-on home map (which a spectator can never be
	// routed to — enforced control-server-side) never pushes either, since
	// its count never leaves 0.
	if (GActiveSpectatorCount <= 0) return;

	if (!actor) return;
	// Cheap-bail before any of the more expensive work below. Substring match
	// on the cached class name — SDK StaticClass()/IsA() is unreliable on
	// this binary (see hooking-and-sdk.md).
	if (!ObjectClassCache::ClassNameContains(actor, "TgPawn_Character")) return;

	ATgPawn* pawn = reinterpret_cast<ATgPawn*>(actor);

	// Real players only — bots get TgAIController, and bIsPlayer is unreliable
	// for this (it's a "valid combatant" gate, not "has client connection").
	if (!pawn->Controller || !ObjectClassCache::ClassNameContains(pawn->Controller, "PlayerController")) {
		return;
	}

	const DWORD now = GetTickCount();
	DWORD& last = g_lastPushMs[(int)pawn->r_nPawnId];
	if (now - last < kPushIntervalMs) return;
	last = now;

	// No session mapping (e.g. a bot possessed via -possess, or a pawn whose
	// PLAYER_REGISTER hasn't landed yet) -> nothing to correlate this with on
	// the control-server side. Silently skip rather than send an orphan row.
	auto it = GPawnSessions.find(pawn);
	if (it == GPawnSessions.end()) return;
	const std::string& session_guid = it->second;

	nlohmann::json effect_ids = nlohmann::json::array();
	if (pawn->r_EffectManager) {
		CollectEffectIds(pawn->r_EffectManager->s_AppliedEffectGroups, effect_ids);
		CollectEffectIds(pawn->r_EffectManager->s_SkillBasedEffectGroups, effect_ids);
	}

	nlohmann::json j;
	j["type"]         = IpcProtocol::MSG_PAWN_HEALTH_SNAPSHOT;
	j["instance_id"]  = IpcClient::GetInstanceId();
	j["session_guid"] = session_guid;
	j["task_force"]   = ResolveTaskForceNumber(pawn);
	j["health"]       = pawn->Health;
	j["health_max"]   = pawn->HealthMax;
	j["effect_ids"]   = effect_ids;
	IpcClient::Send(j.dump());
}

void ForgetPawn(int pawnId) {
	g_lastPushMs.erase(pawnId);
}

} // namespace SpectatorOverlayFeed
