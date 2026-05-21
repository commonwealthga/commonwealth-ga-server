#include "src/GameServer/Core/FMallocWindows/Realloc/FMallocWindows__Realloc.hpp"

void* __fastcall FMallocWindows__Realloc::Call(void* This, void* edx, void* Ptr,
                                               unsigned int NewSize, int Alignment) {
	return CallOriginal(This, edx, Ptr, NewSize, Alignment);
}
