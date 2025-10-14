#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	Logger::Log("debug", "MINE TgGame__SpawnPlayerCharacter START\n");

	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;

	PlayerController->Role = 3;
	PlayerController->RemoteRole = 2;
	PlayerController->bNetInitial = 1;
	PlayerController->bNetDirty = 1;
	PlayerController->bForceNetUpdate = 1;
	PlayerController->bReplicateMovement = 0;

	newpawn->PlayerReplicationInfo = PlayerController->PlayerReplicationInfo;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	newpawn->bForceNetUpdate = 1;
	newpawn->bReplicateMovement = 0;


	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);
	// newrepplayer->Team = defenders;
	newrepplayer->bAdmin = 1;
	newrepplayer->r_CustomCharacterAssembly.SuitMeshId = 1225;
	newrepplayer->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	newrepplayer->r_CustomCharacterAssembly.HairMeshId = 1974;
	newrepplayer->r_CustomCharacterAssembly.HelmetMeshId = -1;
	newrepplayer->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.bBald = false;
	newrepplayer->r_CustomCharacterAssembly.bHideHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	newrepplayer->r_CustomCharacterAssembly.bHalfHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	newrepplayer->r_CustomCharacterAssembly.HeadFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.SuitFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	newrepplayer->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_nProfileId = 567; // medic
	newrepplayer->r_nHealthCurrent = 1300;
	newrepplayer->r_nHealthMaximum = 1300;
	newrepplayer->r_nCharacterId = newpawn->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawn;
	newrepplayer->r_ApproxLocation = newpawn->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");

	newrepplayer->eventSetPlayerName(FString(L"Zaxik"));
	// newrepplayer->SetTeam(GTeamsData.Attackers);
	newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

	ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
	ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;

	FTGTEAM_ENTRY newplayerteamentry;
	newplayerteamentry.fsName = FString(L"Zaxik");
	newplayerteamentry.fsMapName = FString(L"Tetra Pier");
	newplayerteamentry.nHealth = 1300;
	newplayerteamentry.nMaxHealth = 1300;
	newplayerteamentry.bLeader = 0;
	newplayerteamentry.pPrep = newrepplayer;

	TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
	TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);

	TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);

	Logger::Log("debug", "MINE TgGame__SpawnPlayerCharacter END\n");

	return newpawn;
}

