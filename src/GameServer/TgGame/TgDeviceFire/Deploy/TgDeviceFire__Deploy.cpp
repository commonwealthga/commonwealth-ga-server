#include "src/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/ApplyPlayerModsToDeployable.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconCarryReaper/BeaconCarryReaper.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

// FUN_10a1b7e0: aim-trace to find placement position for instant deployables.
// __cdecl(float* extent, ATgPawn* pawn, FVector* outLoc, FVector* outNormal, char adjustZ, char ffCheck)
// Writes target location + normal via out params. Returns non-null if valid placement found.
static constexpr uintptr_t FN_PLACEMENT_TRACE = 0x10a1b7e0;
using PlaceFn = void*(__cdecl*)(float*, int*, float*, float*, char, char);

using SetCollisionFromCollisionType_Fn = void(__thiscall*)(AActor*);
inline void SetCollisionFromCollisionType(AActor* a) {
	reinterpret_cast<SetCollisionFromCollisionType_Fn>(0x10c65910)(a);
}


void __fastcall TgDeviceFire__Deploy::Call(UTgDeviceFire* pThis, void* edx) {
	if (!pThis) return;

	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	if (!device) return;

	ATgPawn* pawn = (ATgPawn*)device->Instigator;
	if (!pawn) return;

	char* pFireModeSetup = (char*)pThis->m_pFireModeSetup.Dummy;
	if (!pFireModeSetup) return;

	int deployableId = *(int*)(pFireModeSetup + 0x28);

	device->UpdateDeployModeStatus();
	//if (device->c_bDeployModeStatus != 3) { //DMS_OK

		const char* clsName = TgProj_Deployable__SpawnDeployable::GetDeployableClassName(deployableId);
		if (!clsName) {
			Logger::Log("inventory", "[deploy] no class name for deployableId=%d, aborting\n", deployableId);
			return;
		}
		UClass* cls = ClassPreloader::GetClass(clsName);
		if (!cls) {
			Logger::Log("inventory", "[deploy] class %s not preloaded, aborting deployableId=%d\n", clsName, deployableId);
			return;
		}
		bool bIsBeacon = (strcmp(clsName, "Class TgGame.TgDeploy_Beacon") == 0);
		bool bIsForceField = (strcmp(clsName, "Class TgGame.TgDeploy_ForceField") == 0);

		// Data-driven self-spawn discriminator: any deployable whose device-
		// mode row has `require_los_flag = 0` skips the aim trace and spawns
		// on the firing pawn. As of `server.db` v20 this picks out exactly
		// the dome shield, but any future self-targeting deployable inherits
		// the behavior automatically by setting the same DB flag — no code
		// change needed. Replaces the old mesh-name `DEV_ForceField_Dome%`
		// LIKE match. `c_DeployModeSpawnLocation` (the player's AIM hit
		// point) is meaningless for self-spawning deployables — it could be
		// a wall 30m away, the floor in front, or mid-air.
		bool bSelfSpawn = DeployableClassify::DeploysOnSelf(deployableId);
		bool bIsForceFieldByClass = TgProj_Deployable__SpawnDeployable::IsForceFieldDeployableId(deployableId);
		AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

		FVector spawnLocation = bSelfSpawn
			? pawn->Location
			: device->c_DeployModeSpawnLocation;

		if (bSelfSpawn) {
			// Drop Z from cylinder center to feet so the dome's pivot sits on
			// the ground. `pawn->Location` is the cylinder CENTER (waist-height
			// in UE3); without this drop the sphere sits ~46uu (one pawn half-
			// height) too high — visible top half clipping ~head-level, bottom
			// half hanging in mid-air. This matches the pre-refactor dome-place-
			// ment behavior the user established 2026-05-14. Cylinder is null-
			// guarded because the pawn might not have one in a degenerate state
			// (we'd rather spawn at waist than crash).
			if (pawn->CylinderComponent) {
				spawnLocation.Z -= pawn->CylinderComponent->CollisionHeight;
			}

			Logger::Log("inventory",
				"[self-spawn deploy] deployableId=%d pawn=0x%p pawnLoc=(%.1f,%.1f,%.1f) "
				"halfH=%.1f -> spawnLoc=(%.1f,%.1f,%.1f) (centered on pawn feet; "
				"aim-point ignored: c_DeployModeSpawnLocation=(%.1f,%.1f,%.1f))\n",
				deployableId, pawn,
				pawn->Location.X, pawn->Location.Y, pawn->Location.Z,
				pawn->CylinderComponent ? pawn->CylinderComponent->CollisionHeight : 0.0f,
				spawnLocation.X, spawnLocation.Y, spawnLocation.Z,
				device->c_DeployModeSpawnLocation.X,
				device->c_DeployModeSpawnLocation.Y,
				device->c_DeployModeSpawnLocation.Z);
		}

		float cylRadius = 0.f, cylHalfHeight = 0.f;
		float liftHalfHeight = 0.f;
		if (!bIsForceField && !bSelfSpawn) {
			// spawnLocation is the ghost's own placement: UpdateDeployModeStatus
			// above ran the same native trace (raw asm extents) the client ghost
			// uses. Re-tracing with our scaled extents put stations off the ghost.
			TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(deployableId, &cylRadius, &cylHalfHeight);
			TgProj_Deployable__SpawnDeployable::GetDeployableSpawnZLift(deployableId, &liftHalfHeight);
			// Trace output is ground contact (bAdjustZ=1); lift to cylinder center.
			spawnLocation.Z += liftHalfHeight + 5.0f;

			// FVector spawnNormal   = { 0.f, 0.f, 1.f };
			// if (cylRadius > 0.0f && cylHalfHeight > 0.0f) {
			// 	FVector ext = { cylRadius, cylRadius, cylHalfHeight };
			// 	void* hit = ((PlaceFn)FN_PLACEMENT_TRACE)(
			// 		&ext.X, (int*)pawn, &spawnLocation.X, &spawnNormal.X, 1, 0);
			// 	if (hit) {
			// 		spawnLocation.Z += liftHalfHeight + 5.0f;
			// 	}
			// }
		}

		// Anti-exploit: a beacon deployed inside a damaging device volume (the
		// toxic sludge pools on Push_Toxicity / Push_Dust_P) is effectively
		// indestructible — the volume's own hit can't touch it (device 7037
		// gates on target_affect Biological, which rejects a deployable) and
		// the brush interferes with shooting it. Refuse the placement.
		//
		// Nothing here consumes the carry device — that happens in UC
		// DeviceFiring.EndState, which keys only off m_bIsBeaconPlacing and
		// would eat a beacon that never entered the world. Mark the refused
		// fire cycle so that cleanup skips itself exactly once and the player
		// keeps his beacon.
		if (bIsBeacon && HazardVolumes::ContainsLocation(spawnLocation)) {
			const int refusedInvId = device->r_nInventoryId;
			BeaconCarryReaper::MarkDeployRefused((int)pawn->r_nPawnId, refusedInvId);
			Logger::Log("beacon",
				"[deploy] refused: spawnLoc=(%.1f,%.1f,%.1f) inside a hazard volume "
				"(pawn=%d invId=%d) — carry device kept\n",
				spawnLocation.X, spawnLocation.Y, spawnLocation.Z,
				(int)pawn->r_nPawnId, refusedInvId);
			return;
		}

		ATgDeployable* Deployable = (ATgDeployable*)pawn->Spawn(
			cls,
			WorldInfo,
			FName(),
			spawnLocation,
			device->c_DeployModeSpawnRotation,
			nullptr,
			1
		);
		if (!Deployable) {
			Logger::Log("inventory", "[deploy] Spawn returned null for %s (deployableId=%d), aborting\n", clsName, deployableId);
			return;
		}

		// Register immediately after Spawn so LOSTrace can disable this dome's
		// collision before any LOS check fires.
		if (bIsForceFieldByClass)
			TgProj_Deployable__SpawnDeployable::RegisterForceField(Deployable);

		// cylHalfHeight is now the SCALED half-extent (UE3 convention), so it
		// goes straight into SetCollisionSize. The previous `* 2` was a vestige
		// of the old `* 0.5` halving inside GetDeployableCollisionCylinder.
		Deployable->SetCollisionSize(cylRadius, cylHalfHeight);
		if (Deployable->m_TargetComponent) {
			Deployable->m_TargetComponent->SetCylinderSize(cylRadius, cylHalfHeight);
		}


		// Deployable->AdjustMeshToGround();

		Deployable->r_nDeployableId = deployableId;
		Deployable->Instigator = pawn;
		if (device)   Deployable->r_Owner             = device;

		ATgRepInfo_Player* pawnrep = pawn->GetPRI();

		// Snapshot the team's currently-registered beacon BEFORE the setup
		// chain runs. `ApplyDeployableSetup()` below fires the deployable's
		// full lifecycle synchronously — StartDeploy() -> GotoState('Deploy')
		// -> UC `TgDeploy_Beacon.Deploy.BeginState` -> `RegisterBeacon(self,
		// false)` — and RegisterBeacon (0x109f1ed0) assigns `r_Beacon =
		// pBeacon` UNCONDITIONALLY, with no compare and no teardown of the
		// incumbent. So by the time the beacon block further down runs,
		// `mgr->r_Beacon` is already the NEW beacon and the old
		// `if (r_Beacon && r_Beacon != Deployable) DestroyIt()` guard there
		// could never fire — it was dead code, and any beacon already in the
		// world simply became an untracked orphan.
		ATgTeamBeaconManager* preDeployMgr =
			(bIsBeacon && pawnrep && pawnrep->r_TaskForce)
				? pawnrep->r_TaskForce->r_BeaconManager : nullptr;
		ATgDeploy_Beacon* prevTeamBeacon =
			preDeployMgr ? preDeployMgr->r_Beacon : nullptr;

		// Carry device identity, captured before the deploy can consume it —
		// used to arm the slot-11 verification sweep further down.
		int carryInvId = 0;
		if (bIsBeacon) {
			ATgDevice* carryDev = pawn->m_EquippedDevices[11];
			if (carryDev) carryInvId = carryDev->r_nInventoryId;
		}

		Deployable->eventInitReplicationInfo();
		if (pawnrep && pawnrep->r_TaskForce) {
			Deployable->SetTaskForceNumber(pawnrep->r_TaskForce->r_nTaskForce);
		}

		// DRI ownership wiring — required for the HUD device-bar healthbar to
		// appear (matches what SpawnDeployableActor does for the projectile
		// path).
		//
		// Chain: server sets `dri->r_InstigatorInfo = pawnrep` → CPF_Net field
		// replicates to clients → client UC `TgRepInfo_Deployable::Replicated-
		// Event('r_InstigatorInfo')` fires → calls `r_InstigatorInfo.AddMinion(
		// self)` → DRI gets appended to the owner PRI's `m_DRIArray` → HUD's
		// `TgUIPrimaryHUD_DeviceBar.FindSpawnedPet(playerPawn, dev)` walks
		// `localPlayerPRI.m_DRIArray` and matches by `dri->r_nDeployableId ==
		// fireMode.deployable_id` → returns the DRI → healthbar drawn.
		//
		// Without this, every non-beacon instant-deploy (stations, turrets,
		// dome shields) lands on the client with `r_InstigatorInfo == null`,
		// so AddMinion never runs, m_DRIArray stays empty, and the device bar
		// shows no healthbar. The beacon branch below was already setting
		// these via `r_bOwnedByTaskforce` + `r_TaskforceInfo`; we now do it
		// for every deployable so the projectile and instant-deploy paths
		// converge on the same DRI shape.
		//
		// Setting BOTH the taskforce path AND the instigator path is
		// intentional: TgRepInfo_Game::GetTaskForceFor reads
		// `r_bOwnedByTaskforce ? r_TaskforceInfo : r_InstigatorInfo->r_TaskForce`,
		// so populating both gives team-resolution redundancy. The native
		// `SetTaskForce` would zero r_InstigatorInfo as a side-effect — we
		// avoid it and write the fields directly.
		if (Deployable->r_DRI && pawnrep) {
			ATgRepInfo_Deployable* dri = Deployable->r_DRI;
			dri->r_InstigatorInfo = pawnrep;
			if (pawnrep->r_TaskForce) {
				dri->r_bOwnedByTaskforce = 1;
				dri->r_TaskforceInfo     = pawnrep->r_TaskForce;
			}
			dri->bNetDirty        = 1;
			dri->bForceNetUpdate  = 1;
		}

		TgProj_Deployable__SpawnDeployable::RegisterDeployableInGRI(Deployable);

		bool destructible = DeployableClassify::IsDestructible(deployableId);
		Deployable->m_bInDestroyedState = 0;
		Deployable->r_bTakeDamage      = destructible ? 1 : 0;
		Deployable->s_bIsActivated     = 1;
		Deployable->m_bIsDeployed      = 0;
		Deployable->r_nPhysicalType    = 861; // TgPawn.TG_PHYSICALITY_MECHANICAL
		Deployable->bAlwaysRelevant    = 1;

		if (!Deployable->r_EffectManager && destructible) {
			UClass* emClass = ClassPreloader::GetClass("Class TgGame.TgEffectManager");
			if (emClass) {
				FVector emLoc = {0.0f, 0.0f, 0.0f};
				Deployable->r_EffectManager = (ATgEffectManager*)Deployable->Spawn(
					emClass,
					Deployable,         // SpawnOwner
					FName(),
					emLoc,
					Deployable->Rotation,
					nullptr,            // ActorTemplate
					1                   // bNoCollisionFail
				);
				// Spawn can return null (rare — OOM, collision-fail edge cases);
				// guard before the wire-up writes.
				if (Deployable->r_EffectManager) {
					Deployable->r_EffectManager->r_Owner    = (AActor*)Deployable;
					Deployable->r_EffectManager->SetOwner((AActor*)Deployable);
					Deployable->r_EffectManager->Base       = (AActor*)Deployable;
				}
			}
		}

		// The native ApplyDeployableSetup runs the deployable's FULL deploy
		// lifecycle synchronously (StartDeploy → Deploy → DeployComplete →
		// UpdateHealth → Active). Our InitializeDefaultProps — which sets
		// r_nHealth to the real max — runs AFTER, so without this the lifecycle
		// executes at r_nHealth==0 and health<=0 code paths can treat the dome
		// as already dead and destroy it at launch. Seed the real health FIRST
		// so it deploys alive; InitializeDefaultProps re-applies it right after.
		{
			const int seedHp = DeployableClassify::GetMaxHealth(deployableId);
			if (seedHp > 0) Deployable->r_nHealth = seedHp;
		}

		Deployable->ApplyDeployableSetup();

		if (bIsBeacon) {
			ATgRepInfo_Deployable* dri = Deployable->r_DRI;
			Logger::Log("beacon",
				"[deploy] post-ApplyDeployableSetup: beacon=0x%p hp=%d driCur=%d driMax=%d pct=%.4f state=%s\n",
				Deployable, Deployable->r_nHealth,
				dri ? dri->r_nHealthCurrent : -1, dri ? dri->r_nHealthMaximum : -1,
				dri ? dri->r_fDeployMaxHealthPCT : -99.0f,
				Deployable->GetStateName().GetName() ? Deployable->GetStateName().GetName() : "<null>");
		}

		Deployable->InitializeDefaultProps();

		if (bIsBeacon) {
			ATgRepInfo_Deployable* dri = Deployable->r_DRI;
			Logger::Log("beacon",
				"[deploy] post-InitializeDefaultProps: beacon=0x%p hp=%d driCur=%d driMax=%d pct=%.4f\n",
				Deployable, Deployable->r_nHealth,
				dri ? dri->r_nHealthCurrent : -1, dri ? dri->r_nHealthMaximum : -1,
				dri ? dri->r_fDeployMaxHealthPCT : -99.0f);
		}

		if (pThis) Deployable->s_SpawnerDeviceMode = pThis;

		// Thrower-side PERSIST_TIME → auto-despawn timer.
		//
		// DB convention: prop 150 on the player's THROWING device-mode
		// (force fields / mines / bombs). InitializeDefaultProps' internal-
		// device seed misses these, so we read from the thrower here.
		//
		// We DELIBERATELY don't write `s_fPersistTime` — that would let UC's
		// `Active.BeginState` set `LifeSpan = s_fPersistTime`, and raw
		// LifeSpan expiry calls `Destroy()` directly with NO FX (silent
		// despawn — jarring for the player). Instead, schedule a one-shot
		// timer that fires UC's `DestroyIt` event, which runs the full
		// destruction sequence: stops fire, plays `FxActivateIndependant(
		// 'Destroyed', ...)` client-side, swaps to destroyed mesh, flips
		// `r_nReplicateDestroyIt` so the repnotify carries to clients.
		// (See TgDeployable.uc:1049-1100.)
		if (pThis) {
			const float throwerPersist =
				TgProj_Deployable__SpawnDeployable::GetThrowerPersistTime(pThis->m_nId);
			if (throwerPersist > 0.0f) {
				Actor__SetTimer::SetTimer(Deployable, throwerPersist,
					/*bLoop=*/false, FName("DestroyIt"), nullptr);
				Logger::Log("inventory",
					"[lifetime] deployable=0x%p id=%d  thrower-mode=%d -> "
					"SetTimer(%.2fs, 'DestroyIt') for natural-expire FX path\n",
					Deployable, deployableId, pThis->m_nId, throwerPersist);
			}
		}

		// if (destructible) {
		// 	*(unsigned char*)((char*)Deployable + 0x93) = 0; // CollisionType = COLLIDE_CustomDefault
		// 	*(unsigned char*)((char*)Deployable + 0x94) = 0; // ReplicatedCollisionType
		// 	// *(uint32_t*)((char*)Deployable + 0xAC) |= 0x100000; // bNetDirty
		// 	SetCollisionFromCollisionType(Deployable);
		// }

		if (Deployable->m_EquipEffect) {
			Deployable->eventApplyEquipEffects();
		}

		// Per-device deploy limit (TGPID_MAX_DEPLOYABLES_OUT, prop 154 in DB).
		// Compacts pawn->s_SelfDeployableList and destroys the oldest matching
		// entries when adding this new one would exceed the configured limit.
		// No-op when the firing fire mode has no prop-154 row. Mirrors the
		// projectile-path call site (SpawnDeployableActor immediately before
		// its own s_SelfDeployableList.Add) so behavior is consistent across
		// instant-deploy and thrown deployables.
		TgProj_Deployable__SpawnDeployable::EnforceDeployableLimit(pawn, Deployable, pThis);

		pawn->s_SelfDeployableList.Add(Deployable);

		float deploySecs = TgProj_Deployable__SpawnDeployable::ApplyDeployTimeBuff(pawn, TgProj_Deployable__SpawnDeployable::GetDeployTimeSecs(deployableId),
		                                       device ? device->r_nDeviceInstanceId : 0);
		Deployable->r_fTimeToDeploySecs = deploySecs;

		ApplyPlayerModsToDeployable::Apply(pawn, Deployable, device ? device->r_nDeviceInstanceId : 0);

		Deployable->m_FireSkillId = device->m_nSkillId;
		if (Deployable->m_FireMode && Deployable->m_FireMode->m_Owner) {
			AActor* fireOwner = Deployable->m_FireMode->m_Owner;
			ATgDevice* internalDev = (ATgDevice*)fireOwner;
			internalDev->r_nDeviceInstanceId = device->r_nDeviceInstanceId;
			internalDev->m_nSkillId          = device->m_nSkillId;
		}

		if (bIsBeacon) {
			// r_TaskForce can be null (unassigned / mid-team-change pawn) —
			// the old `pawnrep ? pawnrep->r_TaskForce->r_BeaconManager : nullptr`
			// dereferenced it unguarded.
			ATgTeamBeaconManager* beaconMgr =
				(pawnrep && pawnrep->r_TaskForce) ? pawnrep->r_TaskForce->r_BeaconManager : nullptr;
			if (beaconMgr) {
				if (Deployable->r_DRI && beaconMgr->r_TaskForce) {
					Deployable->r_DRI->r_bOwnedByTaskforce = 1;
					Deployable->r_DRI->r_TaskforceInfo     = beaconMgr->r_TaskForce;
					Deployable->r_DRI->r_InstigatorInfo    = pawnrep;
				}
				Deployable->r_bInitialIsEnemy = 0;

				// s_fHealthPercent is what RegisterBeacon(bDeployed=false) copies
				// into DRI.r_fDeployMaxHealthPCT (SetInitialHealthPercent). PCT<=0
				// zeroes GetMaxDeployHealth and disables ALL TickDeploy health
				// writes for the whole deploy.
				Logger::Log("beacon",
					"[deploy] pre-RegisterBeacon(false): beacon=0x%p mgr=0x%p s_fHealthPercent=%.4f deploySecs=%.2f\n",
					Deployable, beaconMgr, beaconMgr->s_fHealthPercent, deploySecs);

				BeaconSdk::RegisterBeacon(beaconMgr, (ATgDeploy_Beacon*)Deployable, false);

				{
					ATgRepInfo_Deployable* dri = Deployable->r_DRI;
					const int deployTarget = dri
						? (int)((float)dri->r_nHealthMaximum * dri->r_fDeployMaxHealthPCT) : -1;
					Logger::Log("beacon",
						"[deploy] post-RegisterBeacon(false): beacon=0x%p hp=%d driCur=%d driMax=%d pct=%.4f -> deployTarget=%d%s\n",
						Deployable, Deployable->r_nHealth,
						dri ? dri->r_nHealthCurrent : -1, dri ? dri->r_nHealthMaximum : -1,
						dri ? dri->r_fDeployMaxHealthPCT : -99.0f, deployTarget,
						(deployTarget <= 0) ? "  ** ramp DISABLED (target<=0) **" : "");
				}

				// One exit beacon per team. Destroy the beacon that was
				// registered BEFORE this deploy (snapshot taken above, before
				// ApplyDeployableSetup's RegisterBeacon overwrote r_Beacon).
				// Runs after our own RegisterBeacon so the destroy's
				// UnRegisterBeacon -> CheckBeacon(true) sees a live r_Beacon
				// and cannot trigger a factory respawn.
				if (prevTeamBeacon && prevTeamBeacon != (ATgDeploy_Beacon*)Deployable &&
					!prevTeamBeacon->m_bInDestroyedState) {
					Logger::Log("beacon",
						"[deploy] destroying superseded team beacon 0x%p (new=0x%p mgr=0x%p)\n",
						prevTeamBeacon, Deployable, beaconMgr);
					prevTeamBeacon->s_bWasPickedUp = 0;   // destruction, not a pickup
					prevTeamBeacon->eventDestroyIt(0);
				}
				// Belt and braces: sweep any other untracked id-36 beacon for
				// this taskforce (e.g. one orphaned by an earlier duplicate
				// registration before this fix shipped).
				BeaconSdk::ReapOrphanBeacons(beaconMgr);

				if (pawn) {
					ATgDevice* carryDev = pawn->m_EquippedDevices[11];
					if (carryDev && carryDev->r_nDeviceId == 1918) {
						uint32_t devFlags = *(uint32_t*)((char*)carryDev + 0x22C);
						int bEquipEffectsApplied = (devFlags & 0x4) != 0;
						Logger::Log("beacon",
							"SpawnDeployableActor[beacon]: tearing down carrier visual on pawn=0x%p slot=11 device=0x%p m_bEquipEffectsApplied=%d\n",
							pawn, carryDev, bEquipEffectsApplied);
						carryDev->RemoveEquipEffects();
					}
				}

				// Arm the slot-11 verification sweep. The primary removal is
				// still DeviceFiring.EndState (which disarms this); the sweep
				// only acts if that path was missed. Deadline comfortably past
				// the device's own refire time so we never win the race
				// against the normal cleanup.
				if (carryInvId > 0) {
					const float refire = pThis ? pThis->GetRefireTime() : 0.0f;
					BeaconCarryReaper::ArmConsume(pawn, carryInvId, refire + 1.0f);
				}
			}
		}
	//}

}
