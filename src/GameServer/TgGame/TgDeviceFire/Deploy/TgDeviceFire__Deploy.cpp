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

	Logger::Log(GetLogChannel(), "TgDeviceFire::Deploy: device=%s pawn=%s attackType=%d deployableId=%d\n",
		device->GetFullName(),
		pawn->GetFullName(),
		pThis->m_nAttackType,
		deployableId);

	// For instant deployables (342), trace from the pawn's aim to find the placement position.
	// This is the same trace UpdateDeployModeStatus uses to show the ghost preview.
	FVector spawnLoc  = pawn->Location;
	FVector spawnNorm = { 0.f, 0.f, 1.f };

	if (pThis->m_nAttackType == 0x156 /* TGTT_ATTACK_INSTANT_DEPLOYABLE */) {
		FVector ext = { 0.f, 0.f, 0.f };
		void* hit = ((PlaceFn)FN_PLACEMENT_TRACE)(
			&ext.X, (int*)pawn, &spawnLoc.X, &spawnNorm.X, 1, 0);
		if (!hit) {
			// Trace found no valid spot — fall back to standing position.
			spawnLoc  = pawn->Location;
			spawnNorm = { 0.f, 0.f, 1.f };
		}
	}

	ATgDeployable* Deployable = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
		pawn, deployableId, spawnLoc, spawnNorm);

	if (Deployable) {
		// Remove the beacon item from the player's inventory now that it has been placed.
		ATgInventoryManager* invMgr = (ATgInventoryManager*)pawn->InvManager;
		int nEquipPoint = (int)device->r_eEquippedAt;
		TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, nEquipPoint);
	}
}
