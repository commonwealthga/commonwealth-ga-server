#include "src/GameServer/TgGame/TgPlayerController/ServerRequestAssignment/TgPlayerController__ServerRequestAssignment.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgPlayerController__ServerRequestAssignment::Call(ATgPlayerController* Controller, void* edx, ATgMissionObjective* Objective) {
	LogCallBegin();
	auto result = CallOriginal(Controller, edx, Objective);
	LogCallEnd();
	return result;
}
