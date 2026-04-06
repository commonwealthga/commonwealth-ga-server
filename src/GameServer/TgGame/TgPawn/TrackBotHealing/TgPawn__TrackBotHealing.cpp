#include "src/GameServer/TgGame/TgPawn/TrackBotHealing/TgPawn__TrackBotHealing.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackBotHealing::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fDamage, fMissingHealth, nMaxHealth);
	LogCallEnd();
}
