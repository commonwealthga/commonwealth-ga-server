#include "src/GameServer/GameModes/CtrPointRotation/CtrPointRotation.hpp"

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Maps/CtrObjectives/CtrObjectives.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr const char* CH = "ctrrot";

// asm objective id 345 = "01_ROTATION_Large" (KOTH proximity capture point).
constexpr int   kObjectiveDefId = 345;
constexpr float kTimeToCapture  = 90.0f;
constexpr float kCylRadius      = 256.0f;
constexpr float kCylHalfHeight  = 128.0f;

// GRI->r_Objectives is ATgMissionObjective*[0x4B].
constexpr int kMaxObjectives = 0x4B;

// Set true at the end of a successful CTR-rotation Init. Gates
// ExcludeLateStockObjective so it only fires for objectives that register AFTER
// our points are seeded (i.e. the stock CTR objectives), never for our own
// points (which register DURING Init). Reset at the top of every Init.
bool s_bSeedingDone = false;

// AddObjectivePointToList @ 0x109f0580 — inserts into GRI lists sorted by
// nPriority and marks the GRI dirty (see SuperAgent.cpp).
typedef void(__fastcall* AddObjectivePointToList_t)(ATgRepInfo_Game*, void*, ATgMissionObjective*);
const AddObjectivePointToList_t AddObjectivePointToList =
	reinterpret_cast<AddObjectivePointToList_t>(0x109f0580);

// Spawn + configure one KOTH-345 rotation point and register it. Mirrors the
// proven SuperAgent::SpawnPoint recipe; differs only in the rotation-state bits
// (priority 1, neutral owner, re-capturable, pending FX, random-pool eligible).
ATgMissionObjective_Proximity* SpawnPoint(
		ATgGame* Game, ATgRepInfo_Game* GRI, UClass* cls, const FVector& loc) {
	FRotator rot; rot.Pitch = 0; rot.Yaw = 0; rot.Roll = 0;

	ATgMissionObjective_Proximity* Obj =
		(ATgMissionObjective_Proximity*)Game->Spawn(cls, (AActor*)Game, FName(), loc, rot, nullptr, 1);
	if (!Obj) { Logger::Log(CH, "SpawnPoint FAILED at (%.0f,%.0f,%.0f)\n", loc.X, loc.Y, loc.Z); return nullptr; }

	// Ground-snap so the mesh sits on the floor (survey coords are at player
	// cylinder height). bTraceActors=1 so static meshes count as floor.
	/*{
		FVector start = loc; start.Z += 64.0f;
		FVector end   = loc; end.Z   -= 8192.0f;
		FVector hitLoc, hitNorm;
		FTraceHitInfo hitInfo;
		std::memset(&hitInfo, 0, sizeof(hitInfo));
		if (Obj->Trace(end, start, 1, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) {
			FVector g = loc; g.Z = hitLoc.Z;
			Obj->SetLocation(g);
		}
	}*/

	// Plain dynamic actor (dynamic NetGUID); bNoDelete/bStatic would make it a
	// net-startup actor the client can't resolve.
	Obj->bNoDelete = 0;
	Obj->bStatic   = 0;

	Obj->nObjectiveId           = kObjectiveDefId;   // CPF_Net → client mesh/name
	Obj->nPriority              = 1;                 // all equal → random rotation
	Obj->nDefaultOwnerTaskForce = 0;                 // neutral / contested
	Obj->r_nOwnerTaskForce      = 0;
	Obj->m_nCurrOwnerTaskforce  = 0;
	Obj->m_bCaptureOnlyOnce     = 0;                 // re-capturable across rotations
	Obj->r_bIsLocked            = 1;                 // unlocked by the priority chain
	Obj->r_bUsePendingState     = 1;                 // "next-up" smoke FX
	Obj->s_bRandomPicked        = 1;                 // random-pool eligible
	Obj->r_eStatus              = 0;                 // TGMOS_NONE — start NEUTRAL (KOTH default)
	Obj->m_fTimeToCapture       = kTimeToCapture;
	// KOTH is a tug-of-war meter: midpoint = neutral, either end = captured.
	// TgBaseObjective_KOTH::ResetObjective centers it at m_fTimeToCapture/2, but
	// that ran (at PostBeginPlay) against the pre-config default time — re-center
	// it against ours, else the point activates already owned by one side.
	Obj->m_fCurrCaptureTime     = kTimeToCapture / 2.0f;
	Obj->r_fCurrCaptureTime     = Obj->m_fCurrCaptureTime;
	// r_bIsActive / bEnabled are left 0 — the unlock chain's SetObjectiveActive
	// flips them when the rotation activates this point.

	// Replication recipe (mirrors map-baked objectives).
	Obj->Role                          = 3;   // ROLE_Authority
	Obj->RemoteRole                    = 1;   // ROLE_SimulatedProxy
	Obj->bAlwaysRelevant               = 1;
	Obj->bNetInitial                   = 1;
	Obj->bNetDirty                     = 1;
	Obj->bForceNetUpdate               = 1;
	Obj->bOnlyDirtyReplication         = 0;
	Obj->bSkipActorPropertyReplication = 0;
	if (Game->WorldInfo) Obj->SetOwner((AActor*)Game->WorldInfo);

	// Capture cylinder (RegisterSelf is idempotent — spawns/sizes the proxy).
	TgMissionObjective__RegisterSelf::Call((ATgMissionObjective*)Obj, nullptr);
	if (Obj->s_CollisionProxy) {
		UCylinderComponent* cyl = (UCylinderComponent*)Obj->s_CollisionProxy->CollisionComponent;
		if (cyl) cyl->SetCylinderSize(kCylRadius, kCylHalfHeight);
		Obj->s_CollisionProxy->m_bIgnoreNonPlayers = 1;  // human capture (PvP)
	}

	AddObjectivePointToList(GRI, nullptr, (ATgMissionObjective*)Obj);
	return Obj;
}

}  // namespace

void CtrPointRotation::Init(ATgGame* Game) {
	// Clear before any early-return so a prior CTR match can't leave this set
	// when we load a non-CTR map next.
	s_bSeedingDone = false;

	if (!Game) return;

	const std::string mapName = Config::GetMapNameChar();
	const std::vector<CtrObjectives::Vec3> points = CtrObjectives::ForMap(mapName);
	if (points.empty()) return;  // not a surveyed CTR map

	// Only the custom PointRotation mode — leaves the stock CTR mode on the same
	// map (DualCTF/Mission) untouched.
	if (!ObjectClassCache::ClassNameContains((UObject*)Game, "PointRotation")) return;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (!GRI) { Logger::Log(CH, "Init: no GRI on %s\n", mapName.c_str()); return; }

	// 1) Neutralize the maps' baked CTR objectives and clear the rotation pools
	//    so CalcNextObjective only ever sees ours.
	ActorCache::CacheMapActors();
	int neutralized = 0;
	for (ATgMissionObjective* Obj : ActorCache::MissionObjectives) {
		if (!Obj) continue;
		Obj->r_bIsActive   = 0;
		Obj->r_bIsLocked   = 1;
		Obj->bEnabled      = 0;
		Obj->bHidden       = 1;
		Obj->bNetDirty     = 1;
		Obj->bForceNetUpdate = 1;
		neutralized++;
	}
	// Full clear (frees Data so no stale stock pointers survive for the
	// sorted-insert native to pick back up).
	GRI->m_MissionObjectives.Clear();
	for (int i = 0; i < kMaxObjectives; i++) GRI->r_Objectives[i] = nullptr;

	// 2) Seed our KOTH-345 points.
	UClass* kothCls = ClassPreloader::GetClass("Class TgGame.TgBaseObjective_KOTH");
	if (!kothCls) { Logger::Log(CH, "Init: no TgBaseObjective_KOTH class\n"); return; }

	// SpawnActor rejects classes whose defaults have bStatic/bNoDelete (KOTH is a
	// map-placed objective). Clear them on the class default object so dynamic
	// Spawn succeeds, then restore.
	AActor* kothCDO = (AActor*)ClassPreloader::GetObject(
		"TgBaseObjective_KOTH TgGame.Default__TgBaseObjective_KOTH");
	unsigned long savedStatic = 0, savedNoDelete = 0;
	if (kothCDO) {
		savedStatic   = kothCDO->bStatic;
		savedNoDelete = kothCDO->bNoDelete;
		kothCDO->bStatic   = 0;
		kothCDO->bNoDelete = 0;
	} else {
		Logger::Log(CH, "Init: KOTH CDO not found — Spawn will likely fail\n");
	}

	int spawned = 0;
	for (const CtrObjectives::Vec3& p : points) {
		FVector loc(p.x, p.y, p.z - 50);
		if (SpawnPoint(Game, GRI, kothCls, loc)) spawned++;
	}

	if (kothCDO) {
		kothCDO->bStatic   = savedStatic;
		kothCDO->bNoDelete = savedNoDelete;
	}

	Logger::Log(CH, "Init: %s — neutralized %d stock objectives, seeded %d/%d rotation points\n",
		mapName.c_str(), neutralized, spawned, (int)points.size());

	// Dump the final rotation pool so we can confirm ONLY our neutral KOTH-345
	// points are pickable (a stray stock/DualCTF objective here would be
	// pre-owned and read as captured the instant it's activated).
	Logger::Log(CH, "Init: final m_MissionObjectives.Count=%d\n", GRI->m_MissionObjectives.Count);
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* o = GRI->m_MissionObjectives.Data[i];
		if (!o) { Logger::Log(CH, "  [%d] <null>\n", i); continue; }
		const std::string cn = ObjectClassCache::GetClassName((UObject*)o);
		ATgMissionObjective_Proximity* po = (ATgMissionObjective_Proximity*)o;
		Logger::Log(CH, "  [%d] %s objId=%d status=%d active=%d locked=%d ownerTF=%d defOwnerTF=%d capOnce=%d time=%.2f curr=%.2f rCurr=%.2f\n",
			i, cn.c_str(), o->nObjectiveId, (int)o->r_eStatus,
			(int)o->r_bIsActive, (int)o->r_bIsLocked, po->m_nCurrOwnerTaskforce,
			po->nDefaultOwnerTaskForce, (int)po->m_bCaptureOnlyOnce,
			po->m_fTimeToCapture, po->m_fCurrCaptureTime, po->r_fCurrCaptureTime);
	}

	// Seeding complete — from now on, any objective that registers via
	// RegisterSelf on this CTR map is a stock straggler to be excluded.
	s_bSeedingDone = true;
}

void CtrPointRotation::ExcludeLateStockObjective(ATgMissionObjective* Obj) {
	// Only acts after a CTR-rotation Init has seeded our points (so our own
	// points, which register during Init, are never touched). On any other map
	// Init never sets the flag, so this is a no-op.
	if (!s_bSeedingDone || Obj == nullptr) return;

	// Our seeded points are KOTH objId 345 — those legitimately belong in the
	// rotation. Anything else registering this late is a stock CTR objective.
	if (Obj->nObjectiveId == kObjectiveDefId) return;

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (!GRI) return;

	// Remove from the rotation pool. Compact the TArray (don't leave a null
	// slot — UC's `foreach GRI.m_MissionObjectives` in CheckWinRound would
	// deref it) so the unmodified CalcNextObjective never picks it.
	bool removed = false;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		if (GRI->m_MissionObjectives.Data[i] == Obj) {
			for (int j = i; j < GRI->m_MissionObjectives.Count - 1; j++)
				GRI->m_MissionObjectives.Data[j] = GRI->m_MissionObjectives.Data[j + 1];
			GRI->m_MissionObjectives.Count--;
			removed = true;
			break;
		}
	}
	// Clear any slot in the fixed r_Objectives[0x4B] mirror as well.
	for (int i = 0; i < kMaxObjectives; i++) {
		if (GRI->r_Objectives[i] == Obj) GRI->r_Objectives[i] = nullptr;
	}

	// Disable so it can never activate / capture even if still referenced.
	Obj->r_bIsLocked     = 1;
	Obj->r_bIsActive     = 0;
	Obj->bEnabled        = 0;
	Obj->bHidden         = 1;
	Obj->bNetDirty       = 1;
	Obj->bForceNetUpdate = 1;

	const std::string cn = ObjectClassCache::GetClassName((UObject*)Obj);
	Logger::Log(CH, "ExcludeLateStockObjective: %s objId=%d removedFromList=%d — disabled\n",
		cn.c_str(), Obj->nObjectiveId, (int)removed);
}
