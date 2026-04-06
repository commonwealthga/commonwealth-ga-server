#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackDamageTaken::Call(ATgPawn* Pawn, void* edx, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDamage);
	LogCallEnd();
}
