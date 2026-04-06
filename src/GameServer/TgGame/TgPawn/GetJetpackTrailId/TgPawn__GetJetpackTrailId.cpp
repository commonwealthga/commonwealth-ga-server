#include "src/GameServer/TgGame/TgPawn/GetJetpackTrailId/TgPawn__GetJetpackTrailId.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgPawn__GetJetpackTrailId::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	auto result = CallOriginal(Pawn, edx);
	LogCallEnd();
	return result;
}
