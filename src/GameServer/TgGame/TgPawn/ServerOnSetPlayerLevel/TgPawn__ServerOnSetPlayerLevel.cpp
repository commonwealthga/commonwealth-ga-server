#include "src/GameServer/TgGame/TgPawn/ServerOnSetPlayerLevel/TgPawn__ServerOnSetPlayerLevel.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ServerOnSetPlayerLevel::Call(ATgPawn* Pawn, void* edx, int nLevelId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nLevelId);
	LogCallEnd();
}
