#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevices/TgPawn__ServerOnEquipCharDevices.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ServerOnEquipCharDevices::Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Action);
	LogCallEnd();
}
