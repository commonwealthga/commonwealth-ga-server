#include "src/GameServer/TgGame/TgPawn/TrackDamagedPlayer/TgPawn__TrackDamagedPlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the ATTACKER pawn when they damage a player.
// Accumulates damage dealt in r_Scores[STYPE_DAMAGE].
void __fastcall TgPawn__TrackDamagedPlayer::Call(ATgPawn* Pawn, void* edx, ATgPawn* TargetPawn, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, TargetPawn, nDeviceModeID, nDamage);

	if (nDamage > 0) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[4] += nDamage;  // STYPE_DAMAGE
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
