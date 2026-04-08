#include "src/GameServer/TgGame/TgPawn/TrackHealing/TgPawn__TrackHealing.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the HEALER pawn when they heal a player.
// fDamage is the heal amount, fMissingHealth is how much the target was missing.
// Effective healing = min(fDamage, fMissingHealth) — don't count overheal.
void __fastcall TgPawn__TrackHealing::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDamage, float fMissingHealth, int nMaxHealth) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fDamage, fMissingHealth, nMaxHealth);

	float effectiveHeal = fDamage;
	if (fMissingHealth > 0.0f && fDamage > fMissingHealth) {
		effectiveHeal = fMissingHealth;
	}

	if (effectiveHeal > 0.0f) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[6] += (int)effectiveHeal;  // STYPE_HEALING
			PRI->bNetDirty = 1;
		}
	}

	LogCallEnd();
}
