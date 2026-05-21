#include "src/GameServer/Core/FMallocWindows/Malloc/FMallocWindows__Malloc.hpp"

// Default Call is a pure passthrough — no interception. This stays out of the
// hot path entirely unless someone explicitly calls Install() from dllmain.
void* __fastcall FMallocWindows__Malloc::Call(void* This, void* edx,
                                              unsigned int Size, int Alignment) {
	return CallOriginal(This, edx, Size, Alignment);
}
