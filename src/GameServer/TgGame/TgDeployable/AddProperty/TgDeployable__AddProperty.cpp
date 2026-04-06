#include "src/GameServer/TgGame/TgDeployable/AddProperty/TgDeployable__AddProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgDeployable__AddProperty::Call(ATgDeployable* Deployable, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax) {
	LogCallBegin();
	CallOriginal(Deployable, edx, nPropId, fBase, fRaw, FMin, FMax);
	LogCallEnd();
}
