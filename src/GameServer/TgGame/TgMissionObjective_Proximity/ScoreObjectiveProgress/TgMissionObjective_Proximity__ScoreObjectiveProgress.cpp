#include "src/GameServer/TgGame/TgMissionObjective_Proximity/ScoreObjectiveProgress/TgMissionObjective_Proximity__ScoreObjectiveProgress.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>
#include <unordered_map>

// Reimplements the stripped UC native
//   `native function ScoreObjectiveProgress(float fDeltaTime)`
// on TgMissionObjective_Proximity. Original entry at 0x10a79790 is an empty
// stub.
//
// Call site: TgMissionObjective_Proximity.uc:442, inside UpdateObjectiveStatus,
// fired per-tick when capture is actively progressing (currStatus == 6:
// attackers on point, no defender contesting).
//
// Per-player credit policy (matches old GA observed behaviour):
//   - 100 obj points per 0.5s real time per attacker on the point (= 200/sec base)
//   - Recon class (r_nProfileId == INFILTRATOR_PROFILE_ID = 681) scores at 2x
//   - Attackers only — defenders parked on their own point don't score
//   - Humans only — bots on the point don't score
//
// Per-player fractional accumulator drained in 100-obj batches; each drain
// emits +100 STYPE_OBJS scoreboard credit and a blue ^100^ floating-number
// popup via COMBAT_MESSAGE event 0xa49b. Pacing: one popup ~every 0.5s
// (~every 0.25s for recon) at default tick rate.
//
// CalculateAdjDeltaTime already boosts capture-progress rate when multiple
// players stack on a point (m_fCaptureAccelRate), but obj-credit per player
// stays at the flat base rate: two attackers each get the same scoreboard
// credit per tick as one would solo. Their *team* captures faster; their
// individual scoreboards don't inflate by virtue of having teammates.

namespace {
	// UC `TgPawn.INFILTRATOR_PROFILE_ID` (the "Recon" class to the player).
	constexpr int   kInfiltratorProfileId = 681;
	constexpr float kBaseObjPointsPerSec  = 200.0f;  // 100 per 0.5s
	constexpr int   kPopupBatch           = 100;

	// Per-PRI fractional accumulator. PRI* keys can dangle if the engine
	// recycles slots, but the worst-case symptom is one slightly delayed
	// first popup for the recycled player; no crash, no leak of consequence
	// (bounded by lifetime player count × ~4 bytes).
	std::unordered_map<APlayerReplicationInfo*, float>& Accum() {
		static std::unordered_map<APlayerReplicationInfo*, float> g;
		return g;
	}
}

void __fastcall TgMissionObjective_Proximity__ScoreObjectiveProgress::Call(
	ATgMissionObjective_Proximity* Obj, void* /*edx*/, float fDeltaTime)
{
	// Original is an empty stub — no CallOriginal.
	if (!Obj || !Obj->s_CollisionProxy || fDeltaTime <= 0.0f) {
		Logger::Log("stats", "[SOP] early-out obj=%p proxy=%p dt=%.4f\n",
			(void*)Obj, (void*)(Obj ? Obj->s_CollisionProxy : nullptr), fDeltaTime);
		return;
	}

	ATgCollisionProxy* Proxy = Obj->s_CollisionProxy;
	const int nNearby = Proxy->m_NearByPlayers.Count;
	Logger::Log("stats",
		"[SOP] objId=%d defTF=%d ownerTF=%d nearby=%d dt=%.4f\n",
		Obj->nObjectiveId, Obj->nDefaultOwnerTaskForce, Obj->r_nOwnerTaskForce,
		nNearby, fDeltaTime);

	auto& accum = Accum();

	for (int i = 0; i < nNearby; i++) {
		ATgPawn* P = Proxy->m_NearByPlayers.Data[i];
		if (!P || !P->Controller || !P->Controller->Class) {
			Logger::Log("stats", "[SOP]  [%d] skip: missing P/Controller/Class\n", i);
			continue;
		}

		const char* ctrlClass = P->Controller->Class->GetFullName();
		const bool isHuman = ctrlClass && strstr(ctrlClass, "PlayerController");

		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)P->PlayerReplicationInfo;
		const int pawnTF = (PRI && PRI->r_TaskForce) ? (int)PRI->r_TaskForce->r_nTaskForce : -1;

		Logger::Log("stats",
			"[SOP]  [%d] pawn=%s ctrl=%s isHuman=%d PRI=%p pawnTF=%d profileId=%d\n",
			i, P->GetName(), ctrlClass ? ctrlClass : "<null>",
			(int)isHuman, (void*)PRI, pawnTF, P->r_nProfileId);

		if (!isHuman) continue;
		if (!PRI || !PRI->r_TaskForce) continue;
		// No team filter: UC's UpdateObjectiveStatus only invokes
		// ScoreObjectiveProgress when bDefenderFound == false, i.e. nobody
		// in m_NearByPlayers is currently classified as a defender by UC's
		// IsDefenderTaskForceNumber native. Anyone left in the proxy is by
		// definition an attacker / neutral / valid scorer. Filtering on
		// nDefaultOwnerTaskForce here additionally breaks game modes like
		// TgGame_Ticket where the map-default owner is set to the defender
		// team but the points are functionally neutral until first capture
		// (mirrors `MissionAlerts.cpp:128` / `TickTicketsCalculation.cpp:60`
		// which use r_bHasBeenCapturedOnce to detect the same condition).
		if (pawnTF <= 0) {
			Logger::Log("stats", "[SOP]  [%d] skip: invalid tf=%d\n", i, pawnTF);
			continue;
		}

		const bool  isRecon = (P->r_nProfileId == kInfiltratorProfileId);
		const float rate    = kBaseObjPointsPerSec * (isRecon ? 2.0f : 1.0f);

		accum[PRI] += rate * fDeltaTime;
		while (accum[PRI] >= (float)kPopupBatch) {
			PRI->r_Scores[9] += kPopupBatch;  // STYPE_OBJS
			PRI->bNetDirty   = 1;
			SendCombatMessage::Call(P, /*Source=*/nullptr, /*Target=*/P,
			                        kPopupBatch, SendCombatMessage::Type::OBJ_POINTS);
			accum[PRI] -= (float)kPopupBatch;

			Logger::Log("stats",
				"[SOP]  [%d] +%d obj (recon=%d) total=%d\n",
				i, kPopupBatch, (int)isRecon, PRI->r_Scores[9]);
		}
	}
}
