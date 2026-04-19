#include "src/GameServer/TgGame/TgPawn_Character/SetCurrentItemProfile/TgPawn_Character__SetCurrentItemProfile.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__SetCurrentItemProfile::Call(ATgPawn_Character* Pawn, void* edx, int nItemProfileId) {
	LogCallBegin();
	int before = Pawn ? Pawn->r_nItemProfileId : -1;
	CallOriginal(Pawn, edx, nItemProfileId);
	int after = Pawn ? Pawn->r_nItemProfileId : -1;
	Logger::Log("skills_debug",
		"[SetCurrentItemProfile] pawn=%p before=%d arg=%d after=%d\n",
		(void*)Pawn, before, nItemProfileId, after);
	LogCallEnd();
}
