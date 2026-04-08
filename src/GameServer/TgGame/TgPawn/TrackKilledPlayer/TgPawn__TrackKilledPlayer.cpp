#include "src/GameServer/TgGame/TgPawn/TrackKilledPlayer/TgPawn__TrackKilledPlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the KILLER pawn when they kill a player.
// Note: TrackKill (on the victim) also credits the killer with r_Scores[STYPE_KILLS].
// This function exists for per-device tracking. We don't double-increment STYPE_KILLS
// here since TrackKill handles it. But if TrackKill doesn't fire for some reason,
// this serves as a safety net.
void __fastcall TgPawn__TrackKilledPlayer::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
