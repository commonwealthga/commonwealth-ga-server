#include "src/GameServer/TgGame/TgPlayerController/FinalSave/TgPlayerController__FinalSave.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__FinalSave::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
