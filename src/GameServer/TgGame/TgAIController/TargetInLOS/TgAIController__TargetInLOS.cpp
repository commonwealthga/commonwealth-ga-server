#include "src/GameServer/TgGame/TgAIController/TargetInLOS/TgAIController__TargetInLOS.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"

int __fastcall TgAIController__TargetInLOS::Call(ATgAIController* aic, void* edx) {
	// ForceField domes are transparent to friendly fire (CheckTeamPassThrough),
	// so the LOS trace should also see through them. Disable their collision
	// for the duration of the check so the recursive trace (FUN_10a806e0)
	// never hits them in the first place. (Hook not installed — superseded by
	// TgAIController__LOSTrace, which covers every LOS path.)
	std::vector<ATgDeployable*> disabled;
	TgProj_Deployable__SpawnDeployable::DisableForceFieldCollision(disabled);

	int result = CallOriginal(aic, edx);

	TgProj_Deployable__SpawnDeployable::RestoreForceFieldCollision(disabled);

	return result;
}
