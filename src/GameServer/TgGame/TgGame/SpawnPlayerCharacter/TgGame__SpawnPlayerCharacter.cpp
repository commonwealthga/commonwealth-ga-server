#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;

	PlayerController->Role = 3;
	PlayerController->RemoteRole = 2;
	PlayerController->bNetInitial = 1;
	PlayerController->bNetDirty = 1;
	PlayerController->bForceNetUpdate = 1;

	newpawn->PlayerReplicationInfo = PlayerController->PlayerReplicationInfo;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	newpawn->bForceNetUpdate = 1;

	return newpawn;
}

