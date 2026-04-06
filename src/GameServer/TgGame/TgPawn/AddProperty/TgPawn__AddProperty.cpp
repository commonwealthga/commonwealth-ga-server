#include "src/GameServer/TgGame/TgPawn/AddProperty/TgPawn__AddProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__AddProperty::Call(ATgPawn* Pawn, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nPropId, fBase, fRaw, FMin, FMax);
	LogCallEnd();
}
