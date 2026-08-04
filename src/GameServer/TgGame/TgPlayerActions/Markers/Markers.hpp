#pragma once

#include <string>

namespace TgPlayerActions::MarkersCmd {

// -markers: highlight players for the spectator who enabled it, and ONLY for
// that spectator. Two delivery routes, because they have different strengths
// and only one of them is proven.
//
// ── ROUTE "glow" (default) ──────────────────────────────────────────────────
// Effect group 1469 -> FX 131, a pure MATERIAL swap with no particles at all:
//     asm_data_set_special_fx_psc    (fx 131) -> 0 rows
//     asm_data_set_special_fx_sounds (fx 131) -> 0 rows
//     asm_data_set_special_fx_materials       -> 3 rows, all same_team_flag=0
//         EFX_Agent_Materials.ColorGlow.MIC_PCAgent_ColorGlow_Gold_hair
//                                              ..._Gold_head
//                                              ..._Gold_suit
// The whole player model turns gold. Because it is a skin swap and not a
// particle system, nothing can balloon into the camera at close range — which
// is exactly the failure the scanbot FX has. Silent, and 30s lifetime, so it
// needs re-pushing only every ~24s (vs 6s for the scanbot).
//
// Per-viewer delivery: for each marked pawn we spawn our own TgEffectManager
// with r_Owner = that pawn but network Owner = the spectator's
// PlayerController and bOnlyRelevantToOwner = 1, then push SetEffectRep on it.
// The manager's channel opens for that one connection, so no other client ever
// receives the effect entry.
//
// ONE MANAGER PER MARKED PAWN, not one shared manager: r_Owner is a single
// Actor ref, so a shared manager could only ever point at one pawn. With <=16
// players that is <=16 hidden, owner-only actors pushing one entry each per
// ~24s — negligible traffic.
//
// *** UNPROVEN ***  This route depends on the client's native
// UpdateEffectForms resolving the effect form's owner from the manager's
// r_Owner (TgEffectManager.uc:61 declares r_Owner as a replicated Actor;
// TgEffectForm carries c_Owner). That wiring is native and could not be
// confirmed by static reading. Verify with `-fx own` first — if that renders,
// this route works. If it does not, use the foreman route below.
//
// Second caveat specific to FX 131: all three material rows are
// same_team_flag=0 (enemy-side). The client picks rows by matching
// bApplyIfSameTeam against IsFriendlyWithLocalPawn(). A spectator has no pawn,
// so that predicate most likely returns false and the enemy rows apply — which
// is what we want — but it is an assumption until seen.
//
// ── ROUTE "foreman" ─────────────────────────────────────────────────────────
// The scanbot route: a hidden, controller-less TgPawn_SupportForeman owned by
// the spectator's PlayerController with bOnlyRelevantToOwner=1, round-robining
// its r_Target. Each change fires TgPawn_SupportForeman.CheckBeingTargeted ->
// r_Target.OnDetectedByAnAlarmBot() (TgPawn.uc:9908), attaching FX 1163 at
// SDPG_Foreground — the only through-wall highlight in the shipped script.
//
// PROVEN to reach a spectator, but: FX 1163 carries sound cue res 5843
// (SND_B_UI.A_CUE_UI_Foreman_Detected), played by the intact native
// UTgSpecialFx::Activate on an FX object the client builds inside
// OnDetectedByAnAlarmBot — unreachable from the server, so the beep cannot be
// muted. Owner-only relevancy at least confines it to the one spectator. Its
// world-scaled sprites also fill the screen at close range, hence the
// near-cull (see min_distance_uu).
//
// Trade: glow is silent and close-range-safe but occluded by geometry;
// foreman draws through walls but beeps. Nothing in the data does both.

// ── TEAM SELECTION AND -spectate ────────────────────────────────────────────
// `-spectate <id> attackers|defenders` is a cosmetic assignment: the DLL sets
// the SPECTATOR'S OWN PRI.r_TaskForce to that task force
// (UObject__ProcessEvent.cpp:1119-1128) without clearing bOnlySpectator, so no
// pawn ever spawns. A teamless `-spectate <id>` leaves r_TaskForce null.
//
// That gives two ways to pick who gets highlighted:
//
//   ABSOLUTE  -markers attackers | defenders
//             Always that side, regardless of how you joined. Works for a
//             teamless spectator.
//
//   RELATIVE  -markers friendly | enemy
//             Resolved against your own PRI.r_TaskForce each sweep, so it
//             follows the side you joined as and keeps working if that
//             assignment changes mid-session. Requires a team: a teamless
//             spectator has no r_TaskForce, so these fall back to All and say
//             so in the audit/log.
//
// CAVEAT specific to FX 131 (the glow route's default): all three of its
// material rows are same_team_flag=0, i.e. enemy-side only. The client picks
// rows by matching bApplyIfSameTeam against IsFriendlyWithLocalPawn(), which
// compares against the local PAWN — and a spectator has none. If that
// predicate falls back to the PRI team rather than the pawn, then
// `-markers friendly` will render NOTHING for a team-assigned spectator,
// because FX 131 has no same-team rows to apply. `-markers enemy` would still
// work. If you see that asymmetry, this is why — and it is a property of FX
// 131, not of the team filter.

// ── ACCESS ──────────────────────────────────────────────────────────────────
// SPECTATOR-ONLY. Execute() rejects any session whose PlayerInfo.is_spectator
// is false, and the auto-enable scan only ever considers spectator
// connections. A live player must never reach this: it is a see-through-team
// highlight on every enemy, i.e. a wallhack if self-served from chat.
// is_spectator is set by the control server from the ga_user_roles "spectator"
// role at PLAYER_REGISTER and is never supplied by the connecting client.

enum class Mode {
    Toggle,
    Off,
    All,        // every live player
    Attackers,  // task force 1 only
    Defenders,  // task force 2 only
    Friendly,   // same task force as the spectator's own PRI
    Enemy,      // the opposing task force
    RouteGlow,
    RouteForeman,
    SetDistance,
};

// Effect group / FX / lifetime for the glow route.
constexpr int   kGlowEffectGroupId = 1469;
constexpr int   kGlowFxId          = 131;
constexpr float kGlowLifetimeSec   = 30.0f;

// Default near-cull for the foreman route, in UE3 world units (cm). Ignored by
// the glow route, which has no close-range scaling problem.
constexpr float kDefaultMinDistanceUU = 1500.0f;

// True for the effect managers the glow route spawns.
//
// Actor__GetOptimizedRepListV2 uses this to suppress replication of AActor::Owner
// for these actors ONLY. The client's effect-form updater (FUN_10a70a70)
// resolves its target as `Owner ?: r_Owner`:
//
//     piVar9 = *(int **)(param_1 + 0x98);          // AActor::Owner
//     if ((piVar9 != 0) || (piVar9 = *(int **)(param_1 + 0x480), piVar9 != 0))
//                                                  // ^ r_Owner, only if Owner null
//
// We must keep Owner = the spectator's PlayerController on the SERVER, because
// that is what drives bOnlyRelevantToOwner and bNetOwner. But if that value
// reaches the client, the native resolves the target as the PlayerController —
// which has no mesh — and never consults r_Owner. Replicating Owner as null
// makes the client fall through to r_Owner (the pawn we want painted) while
// the server keeps its relevancy gate. Verified failure mode: `-fx own`
// rendered nothing while `-fx pawn` rendered correctly.
bool IsMarkerEffectManager(const void* actor);

// Register/unregister an effect manager for the Owner-suppression above.
// Shared with the -fx browser's "own" mode so both routes behave identically;
// the registry lives here because Markers is the primary consumer.
void RegisterMarkerEffectManager(const void* actor);
void UnregisterMarkerEffectManager(const void* actor);

// distance_uu is only read for Mode::SetDistance.
void Execute(const std::string& session_guid, Mode mode, float distance_uu);

// Per-frame entry point (GameEngine__Tick). Returns immediately when no
// session has markers enabled.
void Tick();

// Tear down a session's actors. Called from connection cleanup.
void ForgetSession(const std::string& session_guid);

} // namespace TgPlayerActions::MarkersCmd
