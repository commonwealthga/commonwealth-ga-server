#include "src/GameServer/TgGame/_effect_core/CleanseTracking.hpp"
#include "src/GameServer/TgGame/_effect_core/EffectCredit.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

// The one in-flight cleanse. Valid between NotePendingCleanse and the
// RemoveEffectGroupsByCategory call the same ApplyEffect issues next.
struct PendingCleanse {
	AActor*  target       = nullptr;  // g->m_Target — the pawn being cleansed
	ATgPawn* creditPawn   = nullptr;  // instigator, pet/deployable → owner
	int      deviceId     = 0;        // asm device id, 0 if unresolved
	int      effectGroupId = 0;       // diagnostics only
	bool     armed        = false;
};
PendingCleanse g_pending;

}  // namespace

namespace CleanseTracking {

void NotePendingCleanse(UTgEffect* effect) {
	if (!effect || effect->m_nPropertyId != 140) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g || !g->m_Target) return;

	ATgPawn* creditPawn = EffectCredit::ResolveCreditPawn(g->m_Instigator);

	g_pending.target        = g->m_Target;
	g_pending.creditPawn    = creditPawn;
	g_pending.deviceId      = EffectCredit::ResolveDeviceId(g, creditPawn);
	g_pending.effectGroupId = g->m_nEffectGroupId;
	g_pending.armed         = true;

	if (Logger::IsChannelEnabled("devusage")) {
		Logger::Log("devusage",
			"[CLEANSE-NOTE] egId=%d target=%p credit=%p deviceId=%d\n",
			g->m_nEffectGroupId, (void*)g->m_Target, (void*)creditPawn,
			g_pending.deviceId);
	}
}

void OnRemoved(ATgEffectManager* Manager, int nCategoryCode, int nQuantity,
               int removed) {
	// Mitigation bracket — every damaging impact enters the function with
	// exactly these args (TgEffectDamage.uc:206). Never a cleanse, and it
	// must not consume a pending record either.
	if (nCategoryCode == 431 && nQuantity == 99) return;
	if (!Manager) return;

	if (!g_pending.armed) {
		// Unarmed callers, catalogued from the instance-199 full-logging
		// capture: UC housekeeping polls (Rest 1095 and Team Stealth Buff
		// 1036, every pawn, continuously) — never player cleanses. The same
		// capture settled the miss question: a cleanse's non-matching
		// categories produce NO purge call at all (no unarmed burst around a
		// real NOTE→consume pair), so the game pre-filters remove-effect
		// groups upstream — "cast but nothing to strip" is invisible here by
		// design of the UC, not a gap in this hook. Only removed>0 is worth
		// a line: an unarmed call that actually stripped something would be
		// a provenance miss and must stay visible.
		if (removed > 0 && Logger::IsChannelEnabled("devusage")) {
			Logger::Log("devusage",
				"[CLEANSE-UNATTRIBUTED] mgr=%p owner=%p cat=%d qty=%d removed=%d\n",
				(void*)Manager, (void*)Manager->r_Owner, nCategoryCode,
				nQuantity, removed);
		}
		return;
	}

	// A note is only valid for the immediately-following purge. Wrong pawn →
	// drop it (logged); letting it linger could mis-attribute an unrelated
	// purge on the noted pawn much later.
	if ((AActor*)Manager->r_Owner != g_pending.target) {
		if (Logger::IsChannelEnabled("devusage")) {
			Logger::Log("devusage",
				"[CLEANSE-MISMATCH] egId=%d noted target=%p, purge on owner=%p cat=%d removed=%d — note dropped\n",
				g_pending.effectGroupId, (void*)g_pending.target,
				(void*)Manager->r_Owner, nCategoryCode, removed);
		}
		g_pending = PendingCleanse{};
		return;
	}

	const PendingCleanse rec = g_pending;
	g_pending = PendingCleanse{};

	if (Logger::IsChannelEnabled("devusage")) {
		Logger::Log("devusage",
			"[CLEANSE] egId=%d cat=%d removed=%d credit=%p deviceId=%d target=%p\n",
			rec.effectGroupId, nCategoryCode, removed, (void*)rec.creditPawn,
			rec.deviceId, (void*)rec.target);
	}

	// "Used, ineffective, 0 removed" stays out of the stats — only actual
	// strips count. removed==0 still consumed the record above so the next
	// category's call starts clean.
	if (removed <= 0 || !rec.creditPawn) return;

	ATgPawn* targetPawn =
		ObjectClassCache::ClassNameContains(rec.target, "TgPawn")
			? static_cast<ATgPawn*>(rec.target) : nullptr;
	MatchStats::OnDeviceCleanse(rec.creditPawn, targetPawn, rec.deviceId,
	                            nCategoryCode, removed);
}

}  // namespace CleanseTracking
