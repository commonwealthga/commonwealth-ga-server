#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native UTgPawn::execKillDeployables @ 0x109c0870 is a void stub. UC declares
//     native function KillDeployables(bool bAll);
// and the exec stub makes UC calls to it no-ops. Callers:
//     TgPawn.uc:3089  KillDeployables(true)   ← round-end / destroy-all path
//     TgPawn.uc:4378  KillDeployables(false)  ← regular death: only mark-on-death
//
// This replaces the no-op with the logic UC would have had: iterate
// s_SelfDeployableList, DestroyIt each entry that either (a) the caller asked
// for blanket destruction via bAll, or (b) has m_bDestroyOnOwnerDeathFlag
// (from asm_data_set_deployables.destroy_on_owner_death_flag via
// InitializeFromDeployableDat's cfg+0x58 bit0).
//
// Intentionally does NOT call the original (it would be a no-op anyway).
void __fastcall TgPawn__KillDeployables::Call(ATgPawn* Pawn, void* edx, unsigned long bAll) {
	if (!Pawn) return;

	const int count = Pawn->s_SelfDeployableList.Count;
	Logger::Log("debug",
		"[KillDeployables] pawn=0x%p pawnId=%d bAll=%d listCount=%d\n",
		Pawn, (int)Pawn->r_nPawnId, (int)bAll, count);

	// Walk in reverse in case the destroy path unlinks entries from the list
	// (it shouldn't on the server path — DestroyIt sets flags and bumps
	// r_nReplicateDestroyIt without mutating s_SelfDeployableList — but the
	// reverse walk is robust either way and matches UE3's foreach semantics
	// against a destroyed-during-iteration actor).
	for (int i = count - 1; i >= 0; --i) {
		ATgDeployable* dep = Pawn->s_SelfDeployableList.Data[i];
		if (!dep) continue;

		const bool shouldDestroy = bAll || dep->m_bDestroyOnOwnerDeathFlag;
		if (!shouldDestroy) {
			Logger::Log("debug",
				"  [KillDeployables] keep [%d] 0x%p deployableId=%d (destroy_on_owner_death=0)\n",
				i, dep, dep->r_nDeployableId);
			continue;
		}
		if (dep->m_bInDestroyedState) {
			Logger::Log("debug",
				"  [KillDeployables] skip [%d] 0x%p deployableId=%d (already destroyed)\n",
				i, dep, dep->r_nDeployableId);
			continue;
		}

		Logger::Log("debug",
			"  [KillDeployables] DestroyIt [%d] 0x%p deployableId=%d\n",
			i, dep, dep->r_nDeployableId);

		// UC `Dep.eventDestroyIt(false)` — runs the full UC DestroyIt event
		// which: stops fire, sets LifeSpan, hides mesh, swaps to destroyed
		// mesh, flips m_bInDestroyedState, bumps r_nReplicateDestroyIt to
		// trigger the client-side repnotify.
		dep->eventDestroyIt(0 /* bSkipFx */);
	}

	// UC does `s_SelfDeployableList.Length = 0;` — reset the Count. Data
	// buffer stays allocated for reuse by future spawns on this pawn. The
	// destroyed actors themselves will be GC'd after LifeSpan expires.
	Pawn->s_SelfDeployableList.Count = 0;
}
