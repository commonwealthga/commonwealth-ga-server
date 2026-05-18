#include "src/GameServer/TgGame/TgPawn/TrackBotHealing/TgPawn__TrackBotHealing.hpp"

// Called on the HEALER pawn when they heal a bot OR repair a deployable.
// Same overheal semantics as TrackHealing: scoreboard credit only counts
// actual restored HP. Healing/repairing a target at full HP credits zero
// (fMissingHealth == 0 → effectiveHeal clamped to 0).
void __fastcall TgPawn__TrackBotHealing::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fDamage, fMissingHealth, nMaxHealth);

	if (fMissingHealth < 0.0f) fMissingHealth = 0.0f;
	float effectiveHeal = fDamage;
	if (effectiveHeal > fMissingHealth) effectiveHeal = fMissingHealth;

	if (effectiveHeal > 0.0f) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[6] += (int)effectiveHeal;  // STYPE_HEALING
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
