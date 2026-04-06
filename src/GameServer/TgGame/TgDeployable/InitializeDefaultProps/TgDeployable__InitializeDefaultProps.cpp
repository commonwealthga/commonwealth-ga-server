#include "src/GameServer/TgGame/TgDeployable/InitializeDefaultProps/TgDeployable__InitializeDefaultProps.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgDeployable__InitializeDefaultProps::Call(ATgDeployable* Deployable, void* edx) {
	LogCallBegin();
	CallOriginal(Deployable, edx);
	LogCallEnd();
}
