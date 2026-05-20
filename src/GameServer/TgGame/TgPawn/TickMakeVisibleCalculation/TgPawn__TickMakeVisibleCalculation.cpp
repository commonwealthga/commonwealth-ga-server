#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include <unordered_map>

// Server-side native stripped to `RET 0x4` in the shipping binary. We
// reimplement what the original "per-tick stealth-visibility ticker" had to
// do, based on the surviving binary code:
//
// Ground truth from ghidra:
//   - TgPawn::Tick @ 0x109cb910 (LAB_109cbbcf) decays `m_fMakeVisibleCurrent`
//     (pawn+0xEFC) per tick by `r_fMakeVisibleFadeRate * DeltaTime` (with
//     DAT_1169058c multiplier when NOT stealthed → slower decay), clamps to
//     [0, DAT_11690c58], and pushes the result into the mesh as
//     `SetMeshScalarValue("MakeVisible", value)`. Gated by vtable[0x914]
//     which appears to return non-zero only on the side that owns the
//     visual mesh (client side, normally). Server's m_fMakeVisibleCurrent
//     therefore would NOT naturally decay without our help, leaving any
//     server-side bump (e.g. damage reveal) stuck at its post-bump value
//     forever.
//   - TgPlayerController::CheckStealthedCharacter @ 0x10967020 reads the
//     TARGET pawn's `m_fMakeVisibleCurrent` (param_1[0x3bf] == +0xEFC) to
//     decide whether the target is visible from this viewer. So the visual
//     must be driven by the client's local m_fMakeVisibleCurrent, which is
//     non-replicated.
//   - Client's `TgPawn::NativeReplicatedEvent` @ 0x109D3450 folds
//     `r_fMakeVisibleIncreased` (pawn+0xEF4) into m_fMakeVisibleCurrent and
//     zeros the local field on each replication arrival.
//
// Server is the source of truth. We:
//   1. Decay server-side `m_fMakeVisibleCurrent` here (mirrors what the
//      binary does on the client).
//   2. Compute the delta vs. what the client last received (per-pawn
//      tracker) and write it to `r_fMakeVisibleIncreased`, marking
//      bNetDirty. The V2 rep list (now with `r_fMakeVisibleIncreased` in
//      the bNetDirty delta block) ships it. Client folds, stays in sync.
//
// Per-pawn last-sent tracker is needed because UE3 CPF_Net rep only fires
// on per-connection value CHANGE. By writing the delta we want the client
// to fold, AND only writing when delta != 0, we guarantee each rep carries
// a meaningful payload. Identical-delta ticks (rare — would need exactly
// equal frame-pair decay) silently drop one fold; semantics recover the
// tick after when the next non-equal delta replicates. Drift is bounded by
// one tick of decay.
//
// Skill `Stealth Restealth` (581) gives +50% to r_fMakeVisibleFadeRate via
// a class-80 TgEffect calc=68 buff on prop 353. Honoring the replicated
// field here is what makes the skill actually shorten reveal time.

static std::unordered_map<ATgPawn*, float> g_lastReplicatedMakeVisible;

void __fastcall TgPawn__TickMakeVisibleCalculation::Call(ATgPawn* Pawn, void* /*edx*/, float DeltaTime) {
	if (!Pawn) return;

	// Decay. Match the binary's behavior: slower fade when NOT stealthed
	// (DAT_1169058c is a sub-1 multiplier in ghidra; pick 0.5 as a sane
	// stand-in until the constant is read out). Skip the multiplier path
	// when stealthed → full fade rate brings the reveal back down quickly.
	if (Pawn->m_fMakeVisibleCurrent > 0.0f) {
		float rate = Pawn->r_fMakeVisibleFadeRate;
		if (rate <= 0.0f) rate = 25.0f;
		if (!Pawn->r_bIsStealthed) rate *= 0.5f;

		Pawn->m_fMakeVisibleCurrent -= rate * DeltaTime;
		if (Pawn->m_fMakeVisibleCurrent < 0.0f) Pawn->m_fMakeVisibleCurrent = 0.0f;
	}

	// Replicate the delta to the client. NativeReplicatedEvent on the
	// client adds r_fMakeVisibleIncreased to client-local m_fMakeVisibleCurrent,
	// so writing the diff between server's new value and what the client
	// last received keeps the two in sync.
	float& lastSent = g_lastReplicatedMakeVisible[Pawn];
	const float delta = Pawn->m_fMakeVisibleCurrent - lastSent;
	if (delta != 0.0f) {
		Pawn->r_fMakeVisibleIncreased = delta;
		Pawn->bNetDirty = 1;
		lastSent = Pawn->m_fMakeVisibleCurrent;
	}
}
