#include "src/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ServerOnRequestMission::Call(ATgPawn* Pawn, void* edx, UTgSeqAct_RequestMission* Action) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Action);
	LogCallEnd();
}
