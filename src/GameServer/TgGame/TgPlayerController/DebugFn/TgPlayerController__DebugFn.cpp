#include "src/GameServer/TgGame/TgPlayerController/DebugFn/TgPlayerController__DebugFn.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__DebugFn::Call(ATgPlayerController* Controller, void* edx, unsigned long val, unsigned long val2) {
	LogCallBegin();
	CallOriginal(Controller, edx, val, val2);
	LogCallEnd();
}
