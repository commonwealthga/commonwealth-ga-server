#include "src/GameServer/TgGame/TgPawn/SetPhase/TgPawn__SetPhase.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// TgPawn::SetPhase — intact native @ 0x109c8280. We wrap it to inject a
// per-class, per-target-phase m_fPhaseChangeTime BEFORE the native's
// post-event (OnPhaseChange UC event) fires. The UC OnPhaseChange path is:
//
//   aic.PauseAI(true, m_fPhaseChangeTime);   // TgPawn.uc:9898
//
// which pauses the controller for that many seconds before ChooseNextAction
// can pick the next firing action. Boss Viking (TgPawn_UberWalker) and
// Support Destroyer (TgPawn_Boss_Destroyer) both inherit the TgPawn default
// of 0.0 — PauseAI(true, 0.0) floors to 0.1s inside state Pause, so their
// COMBAT 2 firing action (ROCKET BARRAGE / BLAST RADIUS) runs the same tick
// the phase flips. No telegraph window. Players get killed by attacks they
// had no chance to react to.
//
// Values are borrowed from the closest structurally-analogous transitions in
// asm_data_set_bot_actions (no per-Viking/Destroyer source-of-truth exists):
//   - phase 2 high-damage windup → 3.0s (matches Boss Inquisitor PREP
//     FLAMETHROWER's animation_seconds — the only existing "telegraph
//     before a phase-gated high-damage attack" row in the DB)
//   - phase 1 return-to-combat   → 1.0s (matches DesertWarlord's 5 variants
//     for "PHASE 5-0 CHARGE TIME OUT / MISSED" — 14 rows total in the DB)
//   - phase 0 initial entry      → 0.0s (matches 291/316 phase-change rows
//     including Reaper, Shrike, Vulcan, Switchblade, Overseer, Vanguard)

namespace {

struct PhaseTelegraphConfig {
	const char* classNeedle;
	float secondsPerPhase[6];
};

constexpr PhaseTelegraphConfig kConfigs[] = {
	{ "TgPawn_UberWalker",     { 0.0f, 1.0f, 2.5f, 0.0f, 0.0f, 0.0f } },
	{ "TgPawn_Boss_Destroyer", { 0.0f, 1.0f, 2.0f, 0.0f, 0.0f, 0.0f } },
	// Dune/Doom Commander (both TgPawn_DuneCommander, behavior 712): phase 3
	// is SHOCKER — rooted AoE with a ~4s fire-bubble telegraph. COMBAT 3 SHOCK
	// (action 12595) otherwise fires the same tick phase 3 starts.
	{ "TgPawn_DuneCommander",  { 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f } },
};

const PhaseTelegraphConfig* MatchConfig(ATgPawn* Pawn) {
	for (const auto& cfg : kConfigs) {
		if (ObjectClassCache::ClassNameContains(Pawn, cfg.classNeedle)) return &cfg;
	}
	return nullptr;
}

}  // namespace

void __fastcall TgPawn__SetPhase::Call(ATgPawn* Pawn, void* edx, int nNewPhase) {
	if (!Pawn) {
		CallOriginal(Pawn, edx, nNewPhase);
		return;
	}

	if (nNewPhase != Pawn->r_nPhase && nNewPhase >= 0 && nNewPhase < 6) {
		if (const PhaseTelegraphConfig* cfg = MatchConfig(Pawn)) {
			Pawn->m_fPhaseChangeTime = cfg->secondsPerPhase[nNewPhase];
		}
	}

	CallOriginal(Pawn, edx, nNewPhase);
}
