#include "src/GameServer/TgGame/TgPlayerController/ShouldAutoKick/TgPlayerController__ShouldAutoKick.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgPlayerController__ShouldAutoKick::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	auto result = CallOriginal(Controller, edx);
	LogCallEnd();
	return result;
}
