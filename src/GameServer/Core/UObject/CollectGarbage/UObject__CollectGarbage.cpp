#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"

bool UObject__CollectGarbage::bDisableGarbageCollection = false;

void __cdecl UObject__CollectGarbage::Call(void* param_1, uint32_t param_2, void* param_3) {
	if (!bDisableGarbageCollection) {
		CallOriginal(param_1, param_2, param_3);
	}
}

