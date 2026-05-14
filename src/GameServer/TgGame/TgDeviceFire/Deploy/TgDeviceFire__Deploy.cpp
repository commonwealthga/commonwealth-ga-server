#include "src/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

// FUN_10a1b7e0: aim-trace to find placement position for instant deployables.
// __cdecl(float* extent, ATgPawn* pawn, FVector* outLoc, FVector* outNormal, char adjustZ, char ffCheck)
// Writes target location + normal via out params. Returns non-null if valid placement found.
static constexpr uintptr_t FN_PLACEMENT_TRACE = 0x10a1b7e0;
using PlaceFn = void*(__cdecl*)(float*, int*, float*, float*, char, char);

void __fastcall TgDeviceFire__Deploy::Call(UTgDeviceFire* pThis, void* edx) {
	if (!pThis) return;

	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	if (!device) return;

	ATgPawn* pawn = (ATgPawn*)device->Instigator;
	if (!pawn) return;

	char* pFireModeSetup = (char*)pThis->m_pFireModeSetup.Dummy;
	if (!pFireModeSetup) return;

	int deployableId = *(int*)(pFireModeSetup + 0x28);

	// For instant deployables (342), trace from the pawn's aim to find the placement position.
	// This is the same trace UpdateDeployModeStatus uses to show the ghost preview.
	FVector spawnLoc  = pawn->Location;
	FVector spawnNorm = { 0.f, 0.f, 1.f };

	const bool bIsDomeShield = TgProj_Deployable__SpawnDeployable::IsDomeShieldDeployableId(deployableId);

	if (bIsDomeShield) {
		// Dome must be centered at the pawn's FEET, not their waist. `pawn->Location`
		// is at the cylinder CENTER (waist-height); drop it by the pawn's cylinder
		// half-height so the dome's actor.Location lands exactly at the feet.
		// SpawnDeployableActor's `+halfHeight+5` ground-lift is also skipped for
		// domes (see IsDomeShieldDeployableId branch there), so the value we set
		// here is the final actor.Location. Result on flat ground: dome center at
		// ground.Z → top hemisphere visible above ground, bottom hemisphere below.
		UCylinderComponent* pawnCyl = (UCylinderComponent*)pawn->CollisionComponent;
		if (pawnCyl) {
			spawnLoc.Z -= pawnCyl->CollisionHeight;
		}
	}
	else if (pThis->m_nAttackType == 0x156 /* TGTT_ATTACK_INSTANT_DEPLOYABLE */) {
		// Match the client's ghost-preview trace: pass the deployable's real
		// collision cylinder (radius, radius, halfHeight) as the box extent so
		// the returned contact point accounts for the footprint instead of the
		// raw aim ray.  Previous `{0,0,0}` extent caused the placement to land
		// "a bit closer" than the ghost because nothing pushed the contact point
		// back by the cylinder radius.  ffCheck=1 when the deployable is a
		// force field — tells the trace to respect solid FF volumes (UC
		// UpdateDeployModeStatus passes this flag based on class match).
		float radius = 0.f, halfHeight = 0.f;
		TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(deployableId, &radius, &halfHeight);
		bool bIsForceField = TgProj_Deployable__SpawnDeployable::IsForceFieldDeployableId(deployableId);
		FVector ext = { radius, radius, halfHeight };
		void* hit = ((PlaceFn)FN_PLACEMENT_TRACE)(
			&ext.X, (int*)pawn, &spawnLoc.X, &spawnNorm.X, 1, bIsForceField ? 1 : 0);
		if (!hit) {
			// Trace found no valid spot — fall back to standing position.
			spawnLoc  = pawn->Location;
			spawnNorm = { 0.f, 0.f, 1.f };
		}
		// Cylinder-center lift is applied uniformly inside SpawnDeployableActor
		// so both this path and the projectile-landing path (which passes the
		// impact point) get the same ground-to-center adjustment.
	}
	// Dome path summary: spawnLoc was set above to pawn->Location.Z - pawnHalfHeight
	// (pawn's feet). SpawnDeployableActor skips its +halfHeight ground-lift for
	// domes, so actor.Location.Z = feet. Net: on flat ground, dome center at
	// ground level → exactly half-sphere above ground. Can deploy mid-air too —
	// no ground snap, dome just centers on wherever the player's feet are.

	ATgDeployable* Deployable = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
		pawn, deployableId, spawnLoc, spawnNorm, device, pThis);

	if (Deployable) {
		// Only consume the spawning device for beacons. Stations, turrets,
		// sensors, force fields, mines etc. are re-usable — consuming them on
		// every deploy bricks the device-bar slot for the rest of the match.
		//
		// The original game uses UTgDevice::r_bConsumedOnUse to gate this,
		// which is set either by device AssemblyDat flags the stripped native
		// would read, or by TgDeployable.PickUpDeployable when the player
		// picks up a previously deployed object. Our server populates neither.
		// Class-based gating is a stand-in until the r_bConsumedOnUse pipe is
		// wired.
		bool bConsume = TgProj_Deployable__SpawnDeployable::IsBeaconDeployable(Deployable);

		if (Logger::IsChannelEnabled(GetLogChannel())) {
			Logger::Log(GetLogChannel(),
				"TgDeviceFire::Deploy: consume-on-deploy=%d (class=%s)\n",
				(int)bConsume,
				Deployable->Class ? Deployable->Class->GetFullName() : "<null>");
		}

		if (bConsume) {
			ATgInventoryManager* invMgr = (ATgInventoryManager*)pawn->InvManager;
			int nEquipPoint = (int)device->r_eEquippedAt;
			TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, nEquipPoint);
		}
	}
}
