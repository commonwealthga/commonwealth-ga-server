#include "src/GameServer/TgGame/TgAIController/LOSTrace/TgAIController__LOSTrace.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"

#include <stdint.h>
#include <string.h>

namespace {
// p1100 perf counters (perf channel only): calls, exact-argument repeats within one frame, distinct callers.
constexpr int kSetSize = 4096;  // open addressing, power of two
uint64_t g_argSet[kSetSize];
uint64_t g_ctrlSet[kSetSize];
int64_t  g_setFrame = -1;
bool SetInsert(uint64_t* set, uint64_t key) {  // true = newly inserted
	if (key == 0) key = 1;
	for (uint32_t i = (uint32_t)(key * 0x9E3779B97F4A7C15ull >> 52), n = 0; n < 64; i = (i + 1) & (kSetSize - 1), n++) {
		if (set[i] == key) return false;
		if (set[i] == 0) { set[i] = key; return true; }
	}
	return true;  // table crowded: count as new
}
void NoteCall(int* self, const int* args, int nargs) {
	const int64_t frame = PerfProbe::FrameNumber();
	if (frame != g_setFrame) { memset(g_argSet, 0, sizeof g_argSet); memset(g_ctrlSet, 0, sizeof g_ctrlSet); g_setFrame = frame; }
	uint64_t h = 1469598103934665603ull ^ (uint64_t)(uintptr_t)self;
	for (int i = 0; i < nargs; i++) { h ^= (uint32_t)args[i]; h *= 1099511628211ull; }
	PerfProbe::Count(PerfProbe::CTR_LOS_TRACE);
	if (!SetInsert(g_argSet, h)) PerfProbe::Count(PerfProbe::CTR_LOS_DUP);
	if (SetInsert(g_ctrlSet, (uint64_t)(uintptr_t)self)) PerfProbe::Count(PerfProbe::CTR_LOS_CTRL);
}
}  // namespace

// Depth counter — prevents re-disabling/re-enabling on recursive self-calls.
static int s_depth = 0;

int __fastcall TgAIController__LOSTrace::Call(int* param_1, void* edx,
	int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10)
{
	if (s_depth == 0 && PerfProbe::Enabled()) {
		const int a[9] = {p2, p3, p4, p5, p6, p7, p8, p9, p10};
		NoteCall(param_1, a, 9);
	}
	PerfProbe::Scope _los(PerfProbe::SLOT_LOS_TRACE);
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
