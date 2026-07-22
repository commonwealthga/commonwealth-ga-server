#include "src/GameServer/Stats/SpectatorOverlayFeed/SpectatorOverlayFeed.hpp"

#include <unordered_map>

#include "lib/nlohmann/json.hpp"

#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"

namespace SpectatorOverlayFeed {

namespace {

// ~3 pushes/sec per pawn — plenty for a health bar overlay, cheap enough to
// not matter next to everything else Actor::Tick already does per frame.
constexpr DWORD kPushIntervalMs = 300;

std::unordered_map<ATgPawn*, DWORD> g_lastPushMs;

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
	DWORD& last = g_lastPushMs[pawn];
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

} // namespace SpectatorOverlayFeed
