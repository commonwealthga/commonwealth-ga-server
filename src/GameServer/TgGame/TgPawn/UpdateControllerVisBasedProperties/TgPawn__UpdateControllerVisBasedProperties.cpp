#include "src/GameServer/TgGame/TgPawn/UpdateControllerVisBasedProperties/TgPawn__UpdateControllerVisBasedProperties.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdateControllerVisBasedProperties::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
