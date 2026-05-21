#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"

bool FMallocWindows__Free::bLogEnabled = false;

void __fastcall FMallocWindows__Free::Call(void *Malloc, void *edx, void *Ptr) {
	CallOriginal(Malloc, edx, Ptr);
}
