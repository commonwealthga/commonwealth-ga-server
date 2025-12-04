#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool FMallocWindows__Free::bLogEnabled = false;

void __fastcall FMallocWindows__Free::Call(void *Malloc, void *edx, void *Ptr) {
	// workaround for a crash that happens when a client disconnects (UNetConnection::Cleanup)
	// crash point: 0x1090b4dc
	int* piVar4 = (int *)(((int)Ptr >> 0x10 & 0x7ff) * 0x20 + *(int *)((int)Malloc + 0x208 + ((int)Ptr >> 0x1b) * 4));
	if (piVar4[7] == 0) {
		if (bLogEnabled) {
			Logger::Log(GetLogChannel(), "Detected a crash condition - skipping call to the original FMallocWindows__Free.\n");
		}

		return;
	}
	CallOriginal(Malloc, edx, Ptr);
}

