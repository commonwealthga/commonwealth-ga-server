#include "src/GameServer/TgGame/_effect_core/BuffWindowTracking.hpp"
#include "src/GameServer/TgGame/_effect_core/EffectCredit.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

// The tuning table. offensive=true → credit damage the buffed pawn DEALS;
// false → credit damage the buffed pawn TAKES. See header for the rules.
struct TrackedBuff {
	int  effectGroupId;
	bool offensive;
};
constexpr TrackedBuff kTracked[] = {
	{ 10746, true  },   // Frenzy Wave      (+25 dmg/attack type, 10s)
	{ 10740, false },   // Protection Wave  (+10 Physical protection, 7s)
	{  8964, false },   // Protection Boost (+25 protection/attack type, 10s)
	// Group Heal Savior (+10 Phys +10% speed, 5s) — skill-sourced, but its
	// instance's source-device id resolves to the DELIVERING group heal
	// (verified instances 207/208), so damage-taken-under-savior lands on
	// the wave/grenade that applied it. Refresh rule (836): a second medic
	// re-triggering extends the FIRST medic's instance, so extension-window
	// damage credits the original caster — known, accepted bias.
	{ 16587, false },
};

// Scan `pawn`'s applied groups for tracked entries of the wanted polarity and
// credit `amount` to each match's caster + device.
void ScanAndCredit(ATgPawn* pawn, bool offensive, int amount) {
	if (!pawn || !pawn->r_EffectManager) return;
	ATgEffectManager* mgr = pawn->r_EffectManager;

	for (int i = 0; i < mgr->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* g = mgr->s_AppliedEffectGroups.Data[i];
		// Null / small-int corruption guard, same as RemoveEffectGroupsByCategory.
		if (!g || reinterpret_cast<uintptr_t>(g) < 0x10000u) continue;

		for (const TrackedBuff& t : kTracked) {
			if (t.offensive != offensive) continue;
			if (g->m_nEffectGroupId != t.effectGroupId) continue;

			ATgPawn* credit = EffectCredit::ResolveCreditPawn(g->m_Instigator);
			const int deviceId = EffectCredit::ResolveDeviceId(g, credit);
			if (Logger::IsChannelEnabled("devusage")) {
				Logger::Log("devusage",
					"[BUFF-WINDOW] egId=%d %s pawn=%p amount=%d credit=%p deviceId=%d\n",
					t.effectGroupId, offensive ? "dealt" : "taken",
					(void*)pawn, amount, (void*)credit, deviceId);
			}
			MatchStats::OnBuffWindowDamage(credit, deviceId, offensive, amount);
		}
	}
}

}  // namespace

namespace BuffWindowTracking {

void OnEnemyPawnDamage(ATgPawn* shooter, ATgPawn* victim, int amount) {
	if (amount <= 0) return;
	ScanAndCredit(shooter, /*offensive=*/true,  amount);
	ScanAndCredit(victim,  /*offensive=*/false, amount);
}

}  // namespace BuffWindowTracking
