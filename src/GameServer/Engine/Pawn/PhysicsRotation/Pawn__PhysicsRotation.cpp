#include "src/GameServer/Engine/Pawn/PhysicsRotation/Pawn__PhysicsRotation.hpp"

void __fastcall Pawn__PhysicsRotation::Call(APawn* Pawn, void* edx, float a, float b, float c) {
	if (Pawn != nullptr && Pawn->Physics == 4 /*PHYS_Flying*/ &&
	    Pawn->Controller == nullptr) {
		Pawn->Physics = 2 /*PHYS_Falling*/;
		CallOriginal(Pawn, edx, a, b, c);
		Pawn->Physics = 4;
		return;
	}
	CallOriginal(Pawn, edx, a, b, c);
}
