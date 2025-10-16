#include "src/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

ATgPawn* __fastcall TgGame__SpawnBotPawn::Call(ATgGame* Game, void* edx, ATgAIController* pTgAI, FVector vLocation, FRotator rRotation, bool bIgnoreCollision, ATgPawn* pOwner, bool bIsDecoy, float fDeploySecs) {

	Logger::Log("debug", "MINE TgGame__SpawnBotPawn START\n");

	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), pTgAI, FName(), vLocation, rRotation, nullptr, 1);

	pTgAI->Pawn = newpawn;
	newpawn->Controller = pTgAI;

	// pTgAI->Role = 3;
	// pTgAI->RemoteRole = 2;
	// pTgAI->bNetInitial = 1;

	newpawn->r_nReplicateDying = 0;

	newpawn->PlayerReplicationInfo = pTgAI->PlayerReplicationInfo;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	newpawn->bForceNetUpdate = 1;
	newpawn->bSkipActorPropertyReplication = 0;
	newpawn->bOnlyDirtyReplication = 0;
	// newpawn->bReplicateMovement = 0;

	// newpawn->Role = 3;
	// newpawn->RemoteRole = 1;

	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(pTgAI->PlayerReplicationInfo);
	// newrepplayer->Team = defenders;
	// newrepplayer->bAdmin = 1;
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

	// newrepplayer->eventSetPlayerName(FString(L"Zaxik"));
	// newrepplayer->SetTeam(GTeamsData.Attackers);
	newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

	// ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
	// ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;
	//
	// FTGTEAM_ENTRY newplayerteamentry;
	// newplayerteamentry.fsName = FString(L"Zaxik");
	// newplayerteamentry.fsMapName = FString(L"Tetra Pier");
	// newplayerteamentry.nHealth = 1300;
	// newplayerteamentry.nMaxHealth = 1300;
	// newplayerteamentry.bLeader = 0;
	// newplayerteamentry.pPrep = newrepplayer;
	//
	// TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
	// TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);
	//
	// TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);

	Logger::Log("debug", "MINE TgGame__SpawnBotPawn END\n");

	return newpawn;
}

