#include "src/GameServer/Engine/Actor/Tick/Actor__Tick.hpp"
#include "src/Utils/Logger/Logger.hpp"

void* __fastcall Actor__Tick::Call(void* a1, void* edx, float a2, int a3) {
	Logger::Log("debug", "Pawn__CrashingFunc::Call called");
	Logger::Log("debug", ", a1: %s\n", ((UObject*)a1)->GetFullName());

	AActor* Actor = (AActor*)a1;
	// Logger::Log("debug", "location.z: %f\n", Actor->Location.Z);
	//
	// if (((AActor*)a1)->bComponentOutsideWorld || ((AActor*)a1)->bDeleteMe) {
	// 	Logger::Log("debug", "Pawn__CrashingFunc::Call preventing crash");
	//
	// 	return nullptr;
	// }
	//
	// uint32_t b4 = *(uint32_t*)((char*)a1 + 0xB4);
	// uint32_t ac = *(uint32_t*)((char*)a1 + 0xAC);
	// Logger::Log("debug", "Pawn flags: b4=%08X ac=%08X\n", b4, ac);
	// if (*(uint32_t*)((char*)a1 + 0xB4) & 0x10) {
	// 	Logger::Log("debug", "Skipping tick for pawn %s (flag 0x10 set)\n", ((UObject*)a1)->GetFullName());
	// 	return nullptr;
	// }
	// if (*(uint32_t*)((char*)a1 + 0xAC) & 0x2) {
	// 	Logger::Log("debug", "Skipping tick for pawn %s (flag 0x2 set)\n", ((UObject*)a1)->GetFullName());
	// 	return nullptr;
	// }
	// if (Actor->Location.Z < -100.0f) {
	// 	Logger::Log("debug", "Skipping tick for pawn %s (z < -100.0f)\n", ((UObject*)a1)->GetFullName());
	// 	return nullptr;
	// }
	if (strcmp(Actor->Class->GetFullName(), "Class TgGame.TgPawn_Character") == 0) {
		Logger::DumpMemory("actortick", a1, 0x1c2c, 0);
	}
	return CallOriginal(a1, edx, a2, a3);
}

