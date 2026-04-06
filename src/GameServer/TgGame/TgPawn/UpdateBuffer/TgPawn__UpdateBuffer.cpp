#include "src/GameServer/TgGame/TgPawn/UpdateBuffer/TgPawn__UpdateBuffer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdateBuffer::Call(ATgPawn* Pawn, void* edx, ATgPawn* Buffer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Buffer);
	LogCallEnd();
}
