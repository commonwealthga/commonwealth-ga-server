#include "src/GameServer/TgGame/TgDeployableFactory/SpawnObject/TgDeployableFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

// Resolve a task-force number to its TgRepInfo_TaskForce (mirrors the beacon
// factory's ResolveTaskForce). The two team RIs are cached in GTeamsData.
ATgRepInfo_TaskForce* ResolveTaskForce(int taskForceNumber) {
	if (taskForceNumber <= 0) return nullptr;
	if (GTeamsData.Attackers && GTeamsData.Attackers->r_nTaskForce == taskForceNumber)
		return GTeamsData.Attackers;
	if (GTeamsData.Defenders && GTeamsData.Defenders->r_nTaskForce == taskForceNumber)
		return GTeamsData.Defenders;
	return nullptr;
}

// True if a pawn is a live combatant on `taskForce`.
bool IsLivePawnOnTaskForce(APawn* p, int taskForce) {
	if (!p || p->bDeleteMe || p->Health <= 0) return false;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)p->PlayerReplicationInfo;
	return pri && pri->r_TaskForce && pri->r_TaskForce->r_nTaskForce == taskForce;
}

// Preferred instigator source: a mission objective bot (e.g. the defended NPC)
// on the EMP's task force. Crediting the AoE kills to an NPC objective bot is
// far better than crediting a random player. ActorCache::MissionObjectives is
// built once at map load and includes the _Bot subclass; r_ObjectiveBot is the
// spawned pawn (null until it spawns). The task-force filter keeps the instigator
// on the EMP's own side so targeter=Enemy still resolves to the other team —
// which also naturally excludes an enemy-team objective bot (e.g. the boss).
APawn* FindObjectiveBotPawn(int taskForce) {
	if (taskForce <= 0) return nullptr;
	for (ATgMissionObjective* obj : ActorCache::MissionObjectives) {
		if (!obj || !ObjectClassCache::ClassNameContains(obj, "TgMissionObjective_Bot"))
			continue;
		ATgPawn* bot = ((ATgMissionObjective_Bot*)obj)->r_ObjectiveBot;
		if (IsLivePawnOnTaskForce((APawn*)bot, taskForce)) return (APawn*)bot;
	}
	return nullptr;
}

// Fallback: any live pawn on the given task force. Walks WorldInfo.PawnList
// (the engine pawn list — NOT GObjObjects) so it's cheap and safe. May select a
// player — only used when no objective bot is available.
APawn* FindTaskForcePawn(AWorldInfo* wi, int taskForce) {
	if (!wi || taskForce <= 0) return nullptr;
	for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn) {
		if (IsLivePawnOnTaskForce(p, taskForce)) return p;
	}
	return nullptr;
}

}  // namespace

// Stripped native (TgDeployableFactory__SpawnObject @ 0x10a8c250). Spawns the
// factory's configured deployable at the factory's location. The deployable id
// (s_nSelectedObjectId) and team/task force come from map_object_config, loaded
// in TgActorFactory::LoadObjectConfig (PreBeginPlay). Callers in UC:
//   - TgDeployableFactory.PostBeginPlay (auto-spawn — gated off by s_bAutoSpawn=0
//     in config for the kismet-timed map FX, so this path is a no-op for them).
//   - TgDeployableFactory.OnToggle (kismet "Turn On" → the actual trigger).
//   - TgDeployableFactory.DeployableDied (respawn while s_bAutoSpawn) — suppressed
//     for s_bSpawnOnce factories by the one-shot guard below.
//
// This is a self-contained copy of the pawn-independent portion of
// TgProj_Deployable::SpawnDeployableActor (which hard-requires a player pawn).
// Duplicated deliberately: factory FX have no owner pawn, no buffs, no per-pawn
// deploy limits, and may need use-case-specific tweaks without risking the
// player deploy path.
void __fastcall TgDeployableFactory__SpawnObject::Call(ATgDeployableFactory* Obj, void* edx) {
	if (Obj == nullptr) return;

	const int deployableId = Obj->s_nSelectedObjectId;
	if (deployableId <= 0) {
		Logger::Log("deployablefactory",
			"SpawnObject mapObjectId=%d: no s_nSelectedObjectId — nothing to spawn\n",
			Obj->m_nMapObjectId);
		return;
	}

	// One-shot guard. s_fLastSpawnTime ships at 0 and is stamped after the first
	// spawn; s_bSpawnOnce factories must not re-spawn (e.g. DeployableDied's
	// respawn after the FX expires).
	if (Obj->s_bSpawnOnce && Obj->s_fLastSpawnTime > 0.0f) {
		return;
	}

	if (!DeployableClassify::IsKnownDeployableId(deployableId)) {
		Logger::Log("deployablefactory",
			"SpawnObject mapObjectId=%d: unknown deployableId=%d — dropping\n",
			Obj->m_nMapObjectId, deployableId);
		return;
	}

	const char* clsName =
		TgProj_Deployable__SpawnDeployable::GetDeployableClassName(deployableId);
	if (!clsName) {
		Logger::Log("deployablefactory",
			"SpawnObject mapObjectId=%d: deployableId=%d has no class row — dropping\n",
			Obj->m_nMapObjectId, deployableId);
		return;
	}
	UClass* cls = ClassPreloader::GetClass(clsName);
	if (!cls) {
		Logger::Log("deployablefactory",
			"SpawnObject mapObjectId=%d: class not preloaded (%s) — dropping\n",
			Obj->m_nMapObjectId, clsName);
		return;
	}

	// Ground-to-center lift along world +Z (factory placements are upright;
	// no surface-normal projection needed like the projectile path). Uses the
	// LEGACY raw*0.5 lift value — see GetDeployableSpawnZLift.
	float cylRadius = 0.f, cylHalfHeight = 0.f, liftHalfHeight = 0.f;
	TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(
		deployableId, &cylRadius, &cylHalfHeight);
	TgProj_Deployable__SpawnDeployable::GetDeployableSpawnZLift(
		deployableId, &liftHalfHeight);

	FVector spawnLoc = Obj->Location;
	spawnLoc.Z += liftHalfHeight + 5.0f;

	ATgDeployable* dep = (ATgDeployable*)Obj->Spawn(
		cls, Obj, FName(), spawnLoc, Obj->Rotation, nullptr, 1);
	if (dep == nullptr) {
		Logger::Log("deployablefactory",
			"SpawnObject mapObjectId=%d: Spawn returned null for deployable=%d\n",
			Obj->m_nMapObjectId, deployableId);
		return;
	}

	// r_nDeployableId MUST be set before eventInitReplicationInfo /
	// ApplyDeployableSetup / InitializeDefaultProps — all three read it to look
	// up cfg + asm rows. See SpawnDeployableActor for the full rationale.
	dep->r_nDeployableId = deployableId;
	dep->AdjustMeshToGround();

	// Factory ownership. s_DeployFactory is ATgActorFactory* — the factory base —
	// so assigning Obj is type-safe.
	dep->s_DeployFactory = Obj;

	// Damage source. The bomb's AoE (ApplyHit -> TakeDamage) needs a valid Pawn
	// Instigator on the deployable's own task force; without one the hit can't
	// apply/attribute (this is why the factory EMP dealt no damage — its
	// Instigator was null, unlike a player-thrown bomb). Prefer a mission
	// objective bot (an NPC) so the AoE kills aren't credited to a random
	// player; fall back to any same-team pawn. Set before eventInitReplicationInfo
	// so the fire path (StartFire timer, fires ~s_fActivationTime later) sees it.
	AWorldInfo* WI = Obj->WorldInfo;
	const int ownTaskForce = (int)Obj->s_nTaskForce;
	APawn* instigator = FindObjectiveBotPawn(ownTaskForce);
	const char* instSrc = "objectiveBot";
	if (!instigator) {
		instigator = FindTaskForcePawn(WI, ownTaskForce);
		instSrc = instigator ? "sameTeamPawn" : "none";
	}
	dep->Instigator = instigator;

	ATgRepInfo_TaskForce* tf = ResolveTaskForce((int)Obj->s_nTaskForce);

	dep->eventInitReplicationInfo();

	// Team ownership so friendly/enemy resolution + team-based damage target
	// the right side (TgRepInfo_Game::GetTaskForceFor / GetTaskForceNumber read
	// the DRI's r_bOwnedByTaskforce + r_TaskforceInfo path; r_InstigatorInfo is
	// the parallel path — set both so they agree on the owning task force).
	if (dep->r_DRI) {
		if (tf) {
			dep->r_DRI->r_bOwnedByTaskforce = 1;
			dep->r_DRI->r_TaskforceInfo     = tf;
		}
		if (instigator && instigator->PlayerReplicationInfo) {
			dep->r_DRI->r_InstigatorInfo =
				(ATgRepInfo_Player*)instigator->PlayerReplicationInfo;
		}
		dep->r_DRI->bNetDirty       = 1;
		dep->r_DRI->bForceNetUpdate = 1;
	}
	dep->r_bInitialIsEnemy = 0;

	// Register in the global per-game deployable list (the populating native is
	// stripped — see RegisterDeployableInGRI).
	TgProj_Deployable__SpawnDeployable::RegisterDeployableInGRI(dep);

	// Activation flags — mirror SpawnDeployableActor's pawn-independent set.
	const bool destructible = DeployableClassify::IsDestructible(deployableId);
	dep->m_bInDestroyedState = 0;
	dep->r_bTakeDamage       = destructible ? 1 : 0;
	dep->s_bIsActivated      = 1;
	dep->m_bIsDeployed       = 0;
	dep->r_nPhysicalType     = 861;  // TgPawn.TG_PHYSICALITY_MECHANICAL
	dep->bAlwaysRelevant     = 1;

	// Destructible deployables need an EffectManager so incoming effects
	// (damage / repair) resolve; HP=0 FX/bombs deliberately skip it (matches
	// SpawnDeployableActor — null EM short-circuits incoming effects).
	if (!dep->r_EffectManager && destructible) {
		UClass* emClass = ClassPreloader::GetClass("Class TgGame.TgEffectManager");
		if (emClass) {
			FVector emLoc = {0.0f, 0.0f, 0.0f};
			dep->r_EffectManager = (ATgEffectManager*)dep->Spawn(
				emClass, dep, FName(), emLoc, dep->Rotation, nullptr, 1);
		}
		if (dep->r_EffectManager) {
			dep->r_EffectManager->r_Owner    = (AActor*)dep;
			dep->r_EffectManager->SetOwner((AActor*)dep);
			dep->r_EffectManager->Base       = (AActor*)dep;
			dep->r_EffectManager->Role       = 3;
			dep->r_EffectManager->RemoteRole = 1;
		}
	}

	// Populate s_Properties (HP, s_fActivationTime, s_fPersistTime, …) from the
	// DB FIRST — BEFORE ApplyDeployableSetup. This is the critical ordering for a
	// factory bomb: ApplyDeployableSetup -> LoadDeployableFromAsm fires the
	// deploy-trigger UC event at the end of setup, and because a factory bomb has
	// no deploy animation it runs Deploy -> DeployComplete -> Active SYNCHRONOUSLY
	// inside that call. Active.BeginState schedules the detonation with
	// `if(s_fActivationTime > 0) SetTimer('StartFire')` (TgDeployable.uc:2001). If
	// s_fActivationTime is still 0 at that moment (i.e. InitializeDefaultProps
	// hasn't run yet), NO StartFire timer is set and the bomb never detonates —
	// the exact symptom. SpawnDeployableActor (player path) gets away with the
	// reverse order only because player bombs have a real deploy anim, so Active
	// runs a frame later, after InitializeDefaultProps. We seed first instead.
	// InitializeDefaultProps reads the DB by deployable_id only (no dependency on
	// ApplyDeployableSetup), so this order is safe.
	dep->InitializeDefaultProps();

	// Drive the native setup chain (loads mesh / form / fire-mode / equip effect;
	// fires the setup + deploy UC events — which now see s_fActivationTime set).
	dep->ApplyDeployableSetup();
	if (cylRadius > 0.0f && cylHalfHeight > 0.0f) {
		dep->SetCollisionSize(cylRadius, cylHalfHeight);
		if (dep->m_TargetComponent) {
			dep->m_TargetComponent->SetCylinderSize(cylRadius, cylHalfHeight);
		}
	}

	// Fire the passive equip effect (e.g. station protection aura) if
	// ApplyDeployableSetup wired one.
	if (dep->m_EquipEffect) {
		dep->eventApplyEquipEffects();
	}

	// Safety-net deploy kickoff. ApplyDeployableSetup -> LoadDeployableFromAsm
	// normally fires the deploy-trigger UC event itself (delay_deploy_flag gates
	// StartDeploy vs DelayDeploy), which is why SpawnDeployableActor never calls
	// it explicitly. This is a no-op if the deployable already deployed
	// (StartDeploy early-returns on m_bIsDeployed); it only matters if the asm
	// event path didn't fire. By now s_fActivationTime is seeded (above), so any
	// resulting Active.BeginState schedules StartFire correctly.
	dep->eventStartDeploy();

	// Guaranteed despawn. We set s_bIsActivated=1 above (mirroring
	// SpawnDeployableActor), which CONSUMES StartFire's one-shot latch
	// (`if(!s_bIsActivated){ if(s_fPersistTime!=0) LifeSpan=s_fPersistTime; ... }`,
	// TgDeployable.uc:2017) — the same immortality bug AfterShock hit. Rather
	// than juggle the latch, write LifeSpan directly (the faithful "spent bomb
	// vanishes" path, per the ServerDetonate hook). The detonation fires at
	// ~s_fActivationTime; its effect groups live on the TARGETS independently, so
	// the deployable itself can expire shortly after. Respect any LifeSpan that
	// setup already established.
	if (dep->LifeSpan <= 0.0f) {
		dep->LifeSpan = dep->s_fActivationTime + 5.0f;
	}

	Obj->nCurrentCount++;
	Obj->s_fLastSpawnTime = WI ? WI->TimeSeconds : 0.0f;

	// Fire-path diagnostics. For a target-less timed bomb, FireAmmunitionDeployable
	// branches on m_bInstantFire: instant → InstantFireDeployable (trace+radial),
	// else → ProjectileFireDeployable (gated on m_Target!=none → fires nothing).
	// m_nShotsPerFire==0 fires nothing either way. These confirm the fire mode
	// loaded a usable config.
	UTgDeviceFire* fm = dep->m_FireMode;
	Logger::Log("deployablefactory",
		"SpawnObject mapObjectId=%d spawned deployable=%d (%s) at (%.0f,%.0f,%.0f) "
		"taskForce=%d hp=%d destructible=%d  m_FireMode=%p s_fActivationTime=%.3f "
		"s_fProximityRadius=%.3f s_fPersistTime=%.3f LifeSpan=%.3f m_bIsDeployed=%d  "
		"m_bInstantFire=%d fireType=%d shotsPerFire=%d targeterType=%d  instigator=%p (%s)%s\n",
		Obj->m_nMapObjectId, deployableId, clsName,
		spawnLoc.X, spawnLoc.Y, spawnLoc.Z, (int)Obj->s_nTaskForce,
		dep->r_nHealth, destructible ? 1 : 0,
		(void*)fm, dep->s_fActivationTime,
		dep->s_fProximityRadius, dep->s_fPersistTime, dep->LifeSpan,
		(int)dep->m_bIsDeployed,
		(int)dep->m_bInstantFire,
		fm ? (int)fm->m_nFireType : -1,
		fm ? fm->m_nShotsPerFire : -1,
		fm ? (int)fm->m_eTargeterType : -1,
		(void*)dep->Instigator, instSrc,
		instigator ? "" : "  [NO same-team pawn/objective bot found — AoE may not apply]");
}
