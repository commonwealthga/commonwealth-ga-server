#include "src/GameServer/TgGame/TgDeployable/GetTaskForce/TgDeployable__GetTaskForce.hpp"

ATgRepInfo_TaskForce* __fastcall TgDeployable__GetTaskForce::Call(
	ATgDeployable* Deployable, void* /*edx*/)
{
	if (!Deployable) return nullptr;
	ATgRepInfo_Deployable* dri = Deployable->r_DRI;
	if (!dri) return nullptr;

	if (dri->r_InstigatorInfo && dri->r_InstigatorInfo->r_TaskForce) {
		return dri->r_InstigatorInfo->r_TaskForce;
	}
	if (dri->r_bOwnedByTaskforce && dri->r_TaskforceInfo) {
		return dri->r_TaskforceInfo;
	}
	return nullptr;
}
