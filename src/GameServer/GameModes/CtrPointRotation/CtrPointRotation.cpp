#include "src/GameServer/GameModes/CtrPointRotation/CtrPointRotation.hpp"

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Maps/CtrObjectives/CtrObjectives.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cmath>
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
	// Binary IsDefender/IsAttackerTaskForce (instanced matches): defender ⇔
	// r_nTaskForce == nDefaultOwnerTaskForce, attacker ⇔ everyone else. 0 here
	// made BOTH teams attackers, so any player pushed the meter toward attacker
	// capture. 2 = defender team (matches the unreplicated client default).
	Obj->nDefaultOwnerTaskForce = 2;
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

// ---- Beacon factory seeding ------------------------------------------------
// Placement offsets surveyed from the Rot_Redistribution03 map dump
// (map_tg_beacon_factory vs map_tg_team_player_start): the entrance pad sits
// ~252-264uu in front of the team start, the exit beacon ~374-415uu, both on
// the spawn-room floor (~43uu below the start point), facing the start's yaw.
constexpr float kEntranceFwd = 256.0f;
constexpr float kExitFwd     = 384.0f;

// Floor Z under `at` (down-trace, bTraceActors=1 so static meshes count).
float GroundZ(AActor* tracer, const FVector& at, float fallbackZ) {
	FVector start = at; start.Z += 64.0f;
	FVector end   = at; end.Z   -= 8192.0f;
	FVector hitLoc, hitNorm;
	FTraceHitInfo hitInfo;
	std::memset(&hitInfo, 0, sizeof(hitInfo));
	if (tracer->Trace(end, start, 1, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo))
		return hitLoc.Z;
	return fallbackZ;
}

ATgBeaconFactory* SpawnBeaconFactory(ATgGame* Game, UClass* cls,
		const FVector& loc, int yaw, int tfNum, bool bExit) {
	FRotator rot; rot.Pitch = 0; rot.Yaw = yaw; rot.Roll = 0;

	ATgBeaconFactory* f =
		(ATgBeaconFactory*)Game->Spawn(cls, (AActor*)Game, FName(), loc, rot, nullptr, 1);
	if (!f) {
		Logger::Log(CH, "SpawnBeaconFactory FAILED tf=%d exit=%d at (%.0f,%.0f,%.0f)\n",
			tfNum, (int)bExit, loc.X, loc.Y, loc.Z);
		return nullptr;
	}

	f->bNoDelete = 0;
	f->bStatic   = 0;
	// The binary PopulateBeaconFactoryList filter is a plain byte compare:
	// factory->s_nTaskForce == manager->r_TaskForce->r_nTaskForce.
	f->s_nTaskForce  = (unsigned char)tfNum;
	f->m_bBeaconExit = bExit ? 1 : 0;
	f->m_bIsFallback = 0;
	f->m_nPriority     = -1;  // untiered — always eligible
	f->m_nPrevPriority = -1;
	// Re-enable after the CDO suppression (InitBeacons). Also what makes the
	// binary CheckBeacon classify the factory-anchored beacon as AT_SPAWN.
	f->s_bAutoSpawn = 1;
	return f;
}

}  // namespace

void CtrPointRotation::InitBeacons(ATgGame* Game) {
	if (!s_bSeedingDone || !Game) return;  // only the seeded CTR rotation variant

	UClass* cls = ClassPreloader::GetClass("Class TgGame.TgBeaconFactory");
	if (!cls) { Logger::Log(CH, "InitBeacons: no TgBeaconFactory class\n"); return; }

	// CDO trick (same as the KOTH spawn in Init): clear bNoDelete/bStatic so
	// dynamic Spawn succeeds, and clear s_bAutoSpawn so the factory's
	// PostBeginPlay auto-SpawnObject — which fires DURING Spawn, before the
	// team/exit fields are set — early-outs in our hook instead of spawning an
	// unowned entrance pad.
	ATgActorFactory* cdo = (ATgActorFactory*)ClassPreloader::GetObject(
		"TgBeaconFactory TgGame.Default__TgBeaconFactory");
	unsigned long savedStatic = 0, savedNoDelete = 0, savedAutoSpawn = 0;
	if (cdo) {
		savedStatic    = cdo->bStatic;
		savedNoDelete  = cdo->bNoDelete;
		savedAutoSpawn = cdo->s_bAutoSpawn;
		cdo->bStatic     = 0;
		cdo->bNoDelete   = 0;
		cdo->s_bAutoSpawn = 0;
	} else {
		Logger::Log(CH, "InitBeacons: TgBeaconFactory CDO not found — Spawn will likely fail\n");
	}

	for (int tfNum = 1; tfNum <= 2; ++tfNum) {
		// Team spawn anchor: centroid of the team's TgTeamPlayerStarts, facing
		// the first start's yaw (spawn-room starts all face the room exit).
		FVector anchor(0, 0, 0);
		int yaw = 0, n = 0;
		for (ATgTeamPlayerStart* ps : ActorCache::PlayerStarts) {
			if (!ps || (int)ps->m_nTaskForce != tfNum) continue;
			if (n == 0) yaw = ps->Rotation.Yaw;
			anchor.X += ps->Location.X;
			anchor.Y += ps->Location.Y;
			anchor.Z += ps->Location.Z;
			n++;
		}
		if (n == 0) {
			Logger::Log(CH, "InitBeacons: no TgTeamPlayerStart for tf %d — no beacons for this team\n", tfNum);
			continue;
		}
		anchor.X /= n; anchor.Y /= n; anchor.Z /= n;

		const float yawRad = yaw * (3.14159265f / 32768.0f);
		const FVector fwd(std::cos(yawRad), std::sin(yawRad), 0.0f);

		// Wall clamp — CTR spawn rooms weren't built for these offsets, so pull
		// the spots back if a wall sits closer than the reference distances.
		float exitDist = kExitFwd, entrDist = kEntranceFwd;
		{
			FVector ts = anchor;
			FVector te(anchor.X + fwd.X * (kExitFwd + 160.0f),
			           anchor.Y + fwd.Y * (kExitFwd + 160.0f), anchor.Z);
			FVector hitLoc, hitNorm;
			FTraceHitInfo hitInfo;
			std::memset(&hitInfo, 0, sizeof(hitInfo));
			if (Game->Trace(te, ts, 1, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) {
				const float dx = hitLoc.X - anchor.X, dy = hitLoc.Y - anchor.Y;
				const float wall = std::sqrt(dx * dx + dy * dy);
				if (exitDist > wall - 96.0f)      exitDist = wall - 96.0f;
				if (entrDist > exitDist - 128.0f) entrDist = exitDist - 128.0f;
			}
		}
		if (entrDist < 48.0f)               entrDist = 48.0f;
		if (exitDist < entrDist + 96.0f)    exitDist = entrDist + 96.0f;

		FVector entrLoc(anchor.X + fwd.X * entrDist, anchor.Y + fwd.Y * entrDist, 0.0f);
		entrLoc.Z = GroundZ((AActor*)Game, FVector(entrLoc.X, entrLoc.Y, anchor.Z), anchor.Z - 43.0f);
		FVector exitLoc(anchor.X + fwd.X * exitDist, anchor.Y + fwd.Y * exitDist, 0.0f);
		exitLoc.Z = GroundZ((AActor*)Game, FVector(exitLoc.X, exitLoc.Y, anchor.Z), anchor.Z - 43.0f);

		ATgBeaconFactory* entrF = SpawnBeaconFactory(Game, cls, entrLoc, yaw, tfNum, false);
		ATgBeaconFactory* exitF = SpawnBeaconFactory(Game, cls, exitLoc, yaw, tfNum, true);

		// Kick — mirrors the retail init order. The taskforces' eventPostInit
		// already ran InitFor → PopulateBeaconFactoryList + CheckBeacon against
		// an EMPTY factory list (our factories didn't exist yet), so redo it:
		// refresh the list, auto-spawn the entrance pad, then CheckBeacon → no
		// beacon + no holder → vtable SpawnNewBeaconForTeam → our exit factory.
		ATgRepInfo_TaskForce* tf = (tfNum == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
		ATgTeamBeaconManager* mgr = tf ? tf->r_BeaconManager : nullptr;
		if (mgr) BeaconSdk::PopulateBeaconFactoryList(mgr);
		if (entrF) TgBeaconFactory__SpawnObject::Call(entrF, nullptr);
		if (mgr) BeaconSdk::CheckBeacon(mgr, true);
		else Logger::Log(CH, "InitBeacons: tf %d has no beacon manager — exit beacon not spawned\n", tfNum);

		Logger::Log(CH,
			"InitBeacons: tf=%d starts=%d anchor=(%.0f,%.0f,%.0f) yaw=%d "
			"entrF=0x%p %.0fuu@(%.0f,%.0f,%.0f) exitF=0x%p %.0fuu@(%.0f,%.0f,%.0f) "
			"mgr=0x%p factories=%d beacon=0x%p\n",
			tfNum, n, anchor.X, anchor.Y, anchor.Z, yaw,
			entrF, entrDist, entrLoc.X, entrLoc.Y, entrLoc.Z,
			exitF, exitDist, exitLoc.X, exitLoc.Y, exitLoc.Z,
			mgr, mgr ? mgr->s_BeaconFactoryList.Num() : -1,
			mgr ? mgr->r_Beacon : nullptr);
	}

	if (cdo) {
		cdo->bStatic     = savedStatic;
		cdo->bNoDelete   = savedNoDelete;
		cdo->s_bAutoSpawn = savedAutoSpawn;
	}
}

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

bool CtrPointRotation::IsActive() {
	return s_bSeedingDone;
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
