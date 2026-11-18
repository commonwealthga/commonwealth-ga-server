#include "src/GameServer/TgGame/TgPawn/TrackBuff/TgPawn__TrackBuff.hpp"
#include "src/GameServer/GameModes/MatchGate.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the BUFFER pawn when they apply a buff.
// Accumulates buff value in r_Scores[STYPE_BUFFVALUE].
void __fastcall TgPawn__TrackBuff::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fValue) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fValue);

	// Pre-match gate: buff value is the third farmable scoreboard stat (stack
	// buffs on a teammate in spawn). Same gate as the damage/heal path.
	if (fValue > 0.0f && MatchGate::MatchLive(Pawn)) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[5] += (int)fValue;  // STYPE_BUFFVALUE
			// PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
