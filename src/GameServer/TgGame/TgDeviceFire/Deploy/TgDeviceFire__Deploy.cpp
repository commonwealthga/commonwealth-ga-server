#include "src/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Macros.hpp"
#include "src/Utils/Logger/Logger.hpp"

// native function Deploy();
// Called by CustomFire() for deployable attack types. Should spawn the deployable actor via
// ATgDevice::SpawnDeployable(vLocation, TargetActor, vNormal). Address not yet identified
// in the binary; stubbed until the spawn infrastructure is mapped.
void __fastcall TgDeviceFire__Deploy::Call(UTgDeviceFire* pThis, void* edx) {
	if (!pThis) return;

	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	if (!device) return;

	ATgPawn* pawn = (ATgPawn*)device->Instigator;

	char* pFireModeSetup = (char*)pThis->m_pFireModeSetup.Dummy;
	int deployableId = *(int*)(pFireModeSetup + 0x28);

	Logger::Log(GetLogChannel(), "TgDeviceFire::Deploy: device=%s pawn=%s attackType=%d deployableId=%d\n",
		device->GetFullName(),
		pawn ? pawn->GetFullName() : "null",
		pThis->m_nAttackType,
		deployableId);

	if (deployableId == 209) { // techro buff station
		UClass* TgDeployableClass = ClassPreloader::GetTgDeployableClass();

		FVector SpawnLocation = pawn->Location;

		ATgDeployable* Deployable = (ATgDeployable*)pawn->Spawn(
			TgDeployableClass,
			pawn,
			FName(),
			SpawnLocation,
			pawn->Rotation,
		   nullptr,
		   1
		);
		if (Deployable != nullptr) {
			ATgRepInfo_Player* pawnrep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
			Deployable->eventInitReplicationInfo();
			Deployable->SetTaskForceNumber(pawnrep->r_TaskForce->r_nTaskForce);
			TARRAY_INIT(pawn, s_SelfDeployableList, ATgDeployable*, 0x1514, 255);
			TARRAY_ADD(s_SelfDeployableList, Deployable);
			Deployable->r_Owner = device;
			Deployable->r_nHealth = 100;
			Deployable->r_nDeployableId = deployableId;
			Deployable->m_bInDestroyedState = 0;
			Deployable->r_bTakeDamage = 1;
			Deployable->s_bIsActivated = 1;
			Deployable->m_bIsDeployed = 1;
			Deployable->bOnlyDirtyReplication = 0;
			Deployable->Role = 3;
			Deployable->RemoteRole = 1;
			Deployable->bNetInitial = 1;
			Deployable->bNetDirty = 1;
			Deployable->bForceNetUpdate = 1;
			Deployable->bAlwaysRelevant = 1;
		}



	}

	// Logger::DumpMemory("firedeploy_m_pAmSetup", (void*)pThis->m_pAmSetup.Dummy, 0x1000);
	// Logger::DumpMemory("firedeploy_m_pFireModeSetup", (void*)pThis->m_pFireModeSetup.Dummy, 0x1000);

	// TODO: call ATgDevice::SpawnDeployable once its binary address is identified.
	// Expected: device->SpawnDeployable(pawn->Location, nullptr, FVector{0,0,1});
}

