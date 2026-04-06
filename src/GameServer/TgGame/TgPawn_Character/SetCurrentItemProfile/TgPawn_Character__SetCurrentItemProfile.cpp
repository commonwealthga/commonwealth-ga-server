#include "src/GameServer/TgGame/TgPawn_Character/SetCurrentItemProfile/TgPawn_Character__SetCurrentItemProfile.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__SetCurrentItemProfile::Call(ATgPawn_Character* Pawn, void* edx, int nItemProfileId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nItemProfileId);
	LogCallEnd();
}
