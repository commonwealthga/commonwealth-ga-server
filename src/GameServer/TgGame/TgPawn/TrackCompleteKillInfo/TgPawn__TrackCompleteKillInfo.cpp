#include "src/GameServer/TgGame/TgPawn/TrackCompleteKillInfo/TgPawn__TrackCompleteKillInfo.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackCompleteKillInfo::Call(ATgPawn* Pawn, void* edx, int nKillerCharacterID, int nKillerDeviceModeID, int nVictimCharacterID, int nVictimDeviceModeID, FVector KillerLocation, FVector VictimLocation, FVector PetLocation, unsigned long bPetKill) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nKillerCharacterID, nKillerDeviceModeID, nVictimCharacterID, nVictimDeviceModeID, KillerLocation, VictimLocation, PetLocation, bPetKill);
	LogCallEnd();
}
