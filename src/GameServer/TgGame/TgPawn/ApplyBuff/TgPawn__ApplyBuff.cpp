#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ApplyBuff::Call(ATgPawn* Pawn, void* edx, FBuffHeader BuffFilter, int nCalcMethodCode, float fAmount, unsigned long bRemove, unsigned char buffSourceType) {
	LogCallBegin();
	CallOriginal(Pawn, edx, BuffFilter, nCalcMethodCode, fAmount, bRemove, buffSourceType);
	LogCallEnd();
}
