#include "src/GameServer/TgGame/TgAIController/LOSTrace/TgAIController__LOSTrace.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"

static const uint32_t kCollide  = 0x08000000;
static const uint32_t kDeleteMe = 0x00000008;

// Depth counter — prevents re-disabling/re-enabling on recursive self-calls.
static int s_depth = 0;

int __fastcall TgAIController__LOSTrace::Call(int* param_1, void* edx,
	int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10)
{
	if (s_depth > 0)
		return CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);

	auto& ffs = TgProj_Deployable__SpawnDeployable::GetForceFieldSet();
	if (ffs.empty())
		return CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);

	std::vector<ATgDeployable*> disabled;
	std::vector<ATgDeployable*> toRemove;

	for (ATgDeployable* d : ffs) {
		if (!d || (*(uint32_t*)((char*)d + 0xAC) & kDeleteMe)) {
			toRemove.push_back(d);
			continue;
		}
		*(uint32_t*)((char*)d + 0xB0) &= ~kCollide;
		disabled.push_back(d);
	}
	for (ATgDeployable* d : toRemove)
		ffs.erase(d);

	s_depth++;
	int result = CallOriginal(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);
	s_depth--;

	for (ATgDeployable* d : disabled)
		*(uint32_t*)((char*)d + 0xB0) |= kCollide;

	return result;
}
