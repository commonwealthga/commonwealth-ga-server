#include "src/GameServer/TgGame/TgPawn/CanMove/TgPawn__CanMove.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgPawn__CanMove::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	auto result = CallOriginal(Pawn, edx);
	LogCallEnd();
	return result;
}
