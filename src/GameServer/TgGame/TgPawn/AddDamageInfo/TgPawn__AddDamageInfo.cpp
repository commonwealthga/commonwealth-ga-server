#include "src/GameServer/TgGame/TgPawn/AddDamageInfo/TgPawn__AddDamageInfo.hpp"

void __fastcall TgPawn__AddDamageInfo::Call(ATgPawn* Pawn, void* edx, ATgPawn* Source, void* DamageString, unsigned char Type) {
	CallOriginal(Pawn, edx, Source, DamageString, Type);
}
