#include "src/GameServer/TgGame/TgPawn/UpdateDamagers/TgPawn__UpdateDamagers.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdateDamagers::Call(ATgPawn* Pawn, void* edx, ATgPawn* Damager) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Damager);
	LogCallEnd();
}
