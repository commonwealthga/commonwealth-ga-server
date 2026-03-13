#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/Utils/Logger/Logger.hpp"

// native function SpawnPet(bool bPet);
// Called by CustomFire() for pet/bot attack types. Should retrieve the bot ID from the fire
// mode's assembly data (m_pFireModeSetup / m_pAmSetup) then delegate to TgGame::SpawnBotById.
// The spawn data offsets within m_pFireModeSetup are not yet mapped; stubbed for now.
void __fastcall TgDeviceFire__SpawnPet::Call(UTgDeviceFire* pThis, void* edx, BOOL bPet) {
	if (!pThis) return;

	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	if (!device) return;

	ATgPawn* pawn = (ATgPawn*)device->Instigator;

	char* pFireModeSetup = (char*)pThis->m_pFireModeSetup.Dummy;
	int petId = *(int*)(pFireModeSetup + 0x2C);

	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: bPet=%d device=%s pawn=%s petId=%d\n",
		(int)bPet,
		device->GetFullName(),
		pawn ? pawn->GetFullName() : "null",
		petId);

	
	ATgGame* game = reinterpret_cast<ATgGame*>(Globals::Get().GGameInfo);
	FVector spawnLocation = pawn->Location;
	spawnLocation.Z += 200;

	ATgPawn* PetPawn = (ATgPawn*)game->SpawnBotById(petId, spawnLocation, pawn->Rotation, true, nullptr, false, pawn, false, pThis, 0.0f);
	if (PetPawn) {
		ATgRepInfo_Player* PetRep = (ATgRepInfo_Player*)PetPawn->PlayerReplicationInfo;
		ATgRepInfo_Player* PawnRep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
		PetRep->r_TaskForce = PawnRep->r_TaskForce;
		PetRep->Team = PawnRep->Team;
		PetRep->SetTeam(PawnRep->r_TaskForce);
		PetPawn->NotifyTeamChanged();

		PetPawn->Role = 3;
		PetPawn->RemoteRole = 1;
		PetPawn->bNetDirty = 1;
		PetPawn->bNetInitial = 1;
		PetPawn->bForceNetUpdate = 1;
		PetPawn->bSkipActorPropertyReplication = 0;
		// PetPawn->bAlwaysRelevant = 1;

		PetRep->Role = 3;
		PetRep->RemoteRole = 1;
		PetRep->bNetDirty = 1;
		PetRep->bNetInitial = 1;
		PetRep->bForceNetUpdate = 1;
		PetRep->bSkipActorPropertyReplication = 0;
		// PetRep->bAlwaysRelevant = 1;
	}

	// ATgPawn* PetPawn = reinterpret_cast<ATgPawn*>(game->SpawnBotById(petId, pawn->Location, pawn->Rotation, true, nullptr, false, pawn, false, pThis, 0.0f));
	// if (PetPawn) {
	// 	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: PetPawn=%s\n", PetPawn ? PetPawn->GetFullName() : "null");
	// } else {
	// 	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: PetPawn=null\n");
	// }


	// Logger::DumpMemory("firespawnpet_m_pAmSetup", (void*)pThis->m_pAmSetup.Dummy, 0x1000);
	// Logger::DumpMemory("firespawnpet_m_pFireModeSetup", (void*)pThis->m_pFireModeSetup.Dummy, 0x1000);
	

	// TODO: read spawn bot ID from pThis->m_pFireModeSetup (offsets unknown).
	// Expected flow:
	//   int nBotId = *(int*)((char*)pThis->m_pFireModeSetup.Dummy + OFFSET_SPAWN_ENTITY_ID);
	//   ATgGame* game = (ATgGame*)Globals::Get().GWorld->...;
	//   game->SpawnBotById(nBotId, pawn->Location, pawn->Rotation,
	//                      false, nullptr, false, pawn, !bPet, pThis, 0.f);
}

