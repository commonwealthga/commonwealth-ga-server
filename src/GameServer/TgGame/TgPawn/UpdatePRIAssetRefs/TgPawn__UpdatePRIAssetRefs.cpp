#include "src/GameServer/TgGame/TgPawn/UpdatePRIAssetRefs/TgPawn__UpdatePRIAssetRefs.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdatePRIAssetRefs::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
