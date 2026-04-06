#include "src/GameServer/TgGame/TgPawn/MakeInvulnerable/TgPawn__MakeInvulnerable.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__MakeInvulnerable::Call(ATgPawn* Pawn, void* edx, float fLength) {
	LogCallBegin();
	CallOriginal(Pawn, edx, fLength);
	LogCallEnd();
}
