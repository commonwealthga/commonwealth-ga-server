#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the VICTIM pawn when they take damage.
// Accumulates damage taken in r_Scores[STYPE_DAMAGETAKEN].
void __fastcall TgPawn__TrackDamageTaken::Call(ATgPawn* Pawn, void* edx, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDamage);

	if (nDamage > 0) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[3] += nDamage;  // STYPE_DAMAGETAKEN
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
