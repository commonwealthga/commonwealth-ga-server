#pragma once

// EffectDisplacementMarker — tracks "an entry with egId X was just removed
// from this manager's s_AppliedEffectGroups" so the immediately-following
// SetEffectRep call can tell that this is a Strongest-Wins / Newest-Wins
// re-emission rather than a first-time apply.
//
// Why this matters: SetEffectRep's queue-push gate suppresses the manual
// `r_EventQueue` push when `sameEgIdOthers == 0` (no pre-existing clones of
// the same egId), because Branch B will allocate a FRESH managed-list slot
// and the slot transition (0 → egId) drives the client to spawn a fresh
// TgEffectForm — making the queue push redundant and causing audible/visible
// doubling. That works for genuine first applies (Colony scope-in, Repair
// Arm tick 1).
//
// But for `application_value_id == 156/157` patterns (Newest/Strongest Wins),
// RemoveAllEffectGroups zeroes the prior slot AND the new apply re-allocates
// — both within the SAME server frame. UE3 replication coalesces intra-frame
// writes: the client only sees end-of-frame state, which is "slot[X] = egId"
// (same as the previous frame's end state). No diff → no replication → no
// client-side respawn. Concrete symptom: Power Station / Medical Station per-
// tick auras vanish on the deploying player's own pawn (self-cast path doesn't
// go through `suppressManaged`), while remaining visible on teammates (cross-
// pawn path takes `suppressManaged` and queue-pushes).
//
// This registry fills the gap: RemoveAllEffectGroups and RemoveEffectGroup
// call Mark() when they actually remove an entry. SetEffectRep calls
// ConsumeWasJustDisplaced() to check + clear the marker. If true, the queue
// push fires to give the client a fresh transient form, mimicking what would
// have happened if the slot transition were observable.
//
// Marks are TIMESTAMPED and expire after ~2ms. This is essential because
// RemoveEffectGroup is called from FOUR UC paths, only two of which are
// apply-driven displacement:
//
//   UC TgEffectManager.uc callers of RemoveEffectGroup:
//     line 206: ProcessEffect(bRemove=true)          — STANDALONE (scope-out)
//     line 481: GetStackingEffectGroup max rotation  — apply-driven (clone added next)
//     line 509: GetRefreshedEffectGroup (app=836)    — apply-driven (clone added next)
//     line 545: LifeOver (timer expiry)              — STANDALONE
//
// The apply-driven paths call GetClonedEffectGroup → SetEffectRep within the
// same synchronous C++ chain (microseconds). The STANDALONE paths return to
// UC without ever firing SetEffectRep — leaving the mark stale until the
// next apply with the same egId picks it up. Pre-timestamp, this caused
// Colony scope-in audio to double on the SECOND and later scope-ins (the
// first scope-out left a mark that the second scope-in consumed).
//
// The expiry threshold cleanly separates legitimate consumption (µs) from
// cross-frame stale marks (≥16ms at 60fps).

class ATgEffectManager;

namespace EffectDisplacementMarker {
	// Record that an entry with `egId` was just removed from `manager`'s
	// s_AppliedEffectGroups. Called by RemoveAllEffectGroups and RemoveEffectGroup.
	// Multiple Mark() calls for the same manager between SetEffectRep calls
	// are OR-ed together (a set is maintained per manager).
	void Mark(ATgEffectManager* manager, int egId);

	// Check whether `egId` was marked for `manager`, and clear the marker if so.
	// Returns true on first call after Mark, false thereafter (until the next
	// Mark). Called by SetEffectRep.
	bool ConsumeWasJustDisplaced(ATgEffectManager* manager, int egId);

	// Forget all markers for a manager. Call if the manager is being destroyed.
	// Optional — markers are small (one int per displaced egId) and only stale
	// markers would accumulate, which doesn't affect correctness (a stale marker
	// would just trigger a one-time spurious queue push on the next apply).
	void Forget(ATgEffectManager* manager);
}
