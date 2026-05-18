#include "src/GameServer/TgGame/TgPawn/TrackHealing/TgPawn__TrackHealing.hpp"

// Called on the HEALER pawn when they heal a player.
// fDamage is the requested heal amount; fMissingHealth is how much HP the
// target was actually missing.
//
// Scoreboard credit only counts ACTUAL restored HP — overheal is excluded.
// In particular, healing a target who's already at full HP credits zero
// (fMissingHealth == 0 → effectiveHeal clamped to 0).
//
// Negative fMissingHealth (target above max via temporary buff, or default
// UC sentinel from an unintended caller) treated as zero: no credit.
void __fastcall TgPawn__TrackHealing::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth) {
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
