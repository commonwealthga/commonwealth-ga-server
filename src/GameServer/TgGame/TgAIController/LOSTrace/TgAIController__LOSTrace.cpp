#include "src/GameServer/TgGame/TgAIController/LOSTrace/TgAIController__LOSTrace.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"

// Depth counter — prevents re-disabling/re-enabling on recursive self-calls.
static int s_depth = 0;

int __fastcall TgAIController__LOSTrace::Call(int* param_1, void* edx,
	int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10)
{
	if (s_depth > 0)
		return CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);

	if (TgProj_Deployable__SpawnDeployable::ForceFieldCount() == 0)
		return CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);

	// static: game thread only + s_depth guard; avoids a heap alloc per line
	// check (this fires for every AI LOS trace).
	static std::vector<ATgDeployable*> s_disabled;
	s_disabled.clear();
	TgProj_Deployable__SpawnDeployable::DisableForceFieldCollision(s_disabled);

	s_depth++;
	int result = CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);
	s_depth--;

	TgProj_Deployable__SpawnDeployable::RestoreForceFieldCollision(s_disabled);

	return result;
}
