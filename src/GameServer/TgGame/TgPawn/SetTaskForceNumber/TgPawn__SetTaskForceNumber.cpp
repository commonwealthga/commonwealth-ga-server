#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgPawn__SetTaskForceNumber::Call(ATgPawn* Pawn, void* edx, int TaskForceNumber) {
	ATgRepInfo_Player* repinfo = reinterpret_cast<ATgRepInfo_Player*>(Pawn->PlayerReplicationInfo);
	ATgRepInfo_TaskForce* taskforce = GTeamsData.Attackers;
	if (TaskForceNumber != 1) {
		taskforce = GTeamsData.Defenders;
	}


	repinfo->r_TaskForce = taskforce;
	repinfo->Team = taskforce;
	repinfo->SetTeam(taskforce);

	Pawn->NotifyTeamChanged();
}

