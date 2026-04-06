#include "src/GameServer/TgGame/TgPawn/GetMoraleDevice/TgPawn__GetMoraleDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgDevice_Morale* __fastcall TgPawn__GetMoraleDevice::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	auto result = CallOriginal(Pawn, edx);
	LogCallEnd();
	return result;
}
