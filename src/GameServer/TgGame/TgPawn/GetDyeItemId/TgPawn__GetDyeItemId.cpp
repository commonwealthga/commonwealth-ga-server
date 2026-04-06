#include "src/GameServer/TgGame/TgPawn/GetDyeItemId/TgPawn__GetDyeItemId.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgPawn__GetDyeItemId::Call(ATgPawn* Pawn, void* edx, unsigned char eSlot) {
	LogCallBegin();
	auto result = CallOriginal(Pawn, edx, eSlot);
	LogCallEnd();
	return result;
}
