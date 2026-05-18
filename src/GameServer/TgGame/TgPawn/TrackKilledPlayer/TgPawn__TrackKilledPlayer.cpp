#include "src/GameServer/TgGame/TgPawn/TrackKilledPlayer/TgPawn__TrackKilledPlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the KILLER pawn when a player victim is killed. Symmetric with
// TrackKilledBot (the bot/deployable variant). Credits r_Scores[STYPE_KILLS].
//
// TgEffect::TrackStats invokes TgPawn::TrackKill on the victim, which also
// credits STYPE_KILLS on the killer's PRI with pet-owner resolution and
// assist crediting. The two paths would converge on r_Scores[1] if both
// fired; in practice the underlying native has no UC caller in our build,
// so this only runs when explicitly called by other reimplemented code.
// Keeping it as a real implementation preserves the parallel naming
// contract and makes a future caller credit correctly.
void __fastcall TgPawn__TrackKilledPlayer::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->r_Scores[1]++;  // STYPE_KILLS
		PRI->bNetDirty = 1;
		PRI->bForceNetUpdate = 1;
	}

	LogCallEnd();
}
