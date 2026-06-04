#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__SetTaskForceNumber::Call(ATgPawn* Pawn, void* edx, int TaskForceNumber) {
	ATgRepInfo_Player* repinfo = reinterpret_cast<ATgRepInfo_Player*>(Pawn->PlayerReplicationInfo);
	ATgRepInfo_TaskForce* taskforce = GTeamsData.Attackers;
	if (TaskForceNumber != 1) {
		taskforce = GTeamsData.Defenders;
	}

	// SetTeam is the intact native @ 0x109f2430. It compares the old `Team`
	// field (offset 0x208) against the new value and, only if different,
	// fires RemovePRI on the old TF + AddPRI on the new TF (vtable[0x3b0]
	// and vtable[0x3ac]). Pre-writing `repinfo->Team = taskforce` before
	// calling SetTeam silently no-op'd the whole thing — the new TF's
	// m_PRIArray never got the switching player's PRI, so CheckBeacon's
	// PRI walk couldn't find the beacon carrier and spawned a duplicate
	// at the spawn point during pickup. SetTeam writes `Team` itself, so
	// our only job is r_TaskForce (the Tg-side mirror).
	repinfo->SetTeam(taskforce);
	repinfo->r_TaskForce = taskforce;

	Pawn->NotifyTeamChanged();
}

