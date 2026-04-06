#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevice/TgPawn__ServerOnEquipCharDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ServerOnEquipCharDevice::Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevice* Action) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Action);
	LogCallEnd();
}
