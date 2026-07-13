#pragma once

#include "src/pch.hpp"
#include "src/GameServer/Stats/StatsCore.hpp"
#include <cstdint>

// Match stats recording + event emission over IPC. Design:
// .planning/2026-06-12-match-stats-tracking-design.md. Every entry point
// no-ops unless SetEnabled(true) arrived in INSTANCE_HELLO_ACK (home maps
// get stats_enabled=false — stock in-PRI behavior there is untouched).
class MatchStats {
public:
    // KILL event flags (ga_match_events.flags).
    static constexpr int kFlagSelfKill            = 1;
    static constexpr int kFlagPetKill             = 2;
    static constexpr int kFlagVictimCarriedBeacon = 4;

    static void SetEnabled(bool enabled);
    static bool Enabled();

    // JOIN site (NotifyControlMessage, after GPawnSessions is populated).
    // Emits JOIN (+CLASS_CHANGE on character switch), restores banked
    // score sums into the fresh PRI on rejoin.
    static void OnPlayerJoined(ATgPawn* Pawn, int64_t user_id,
                               int64_t character_id, int task_force);

    // LEAVE site (NetConnection__Cleanup, before the pawn is torn down).
    static void OnPlayerLeft(ATgPawn* Pawn, int64_t user_id);

    // Team flip (ChangeTeam cmd / autobalance / ServerChangeTaskForce).
    // Call BEFORE the task force is written so the old stint banks clean.
    static void OnTeamChanged(ATgPawn* Pawn, int new_task_force);

    // TrackDeath: remember a pending death; Tick() flushes it as DEATH
    // unless a KILL consumes it first.
    static void OnDeath(ATgPawn* Victim);

    // TrackStats kill block. CreditPawn = scoreboard-credited killer
    // (pet→owner resolved); PetPawn = the pet when is_pet_kill.
    static void OnKill(ATgPawn* CreditPawn, ATgPawn* PetPawn, ATgPawn* Victim,
                       int device_id, bool is_pet_kill, bool is_self_kill);

    // TrackStats deployable-destroyed branch.
    static void OnDeployableDestroyed(ATgPawn* Destroyer,
                                      ATgDeployable* Deployable,
                                      bool is_beacon);

    // ProcessEvent intercept on TgPawn.TriggerBeaconEntrance.
    static void OnBeaconSpawnUsed(ATgPawn* User, ATgPawn* Deployer);

    // MSG_EMIT_MATCH_EVENT (control server → DLL): emit an identity-less
    // event row (e.g. AUTOBALANCE_START / AUTOBALANCE_END batch markers).
    static void EmitMarker(const char* event_type, int64_t detail);

    // Chat-command success sites (TgPlayerActions modules). Emits a CMD_*
    // event tied to the invoking player. Call only for manual invocations
    // (ChangeTeam passes is_autobalance=false). detail = the command's
    // numeric argument (bot_id / deployable_id / new tf / ...); Target =
    // affected pawn when there is one (spawned bot, possessed pawn).
    static void OnChatCommand(ATgPawn* Actor, const char* event_type,
                              int64_t detail = 0, ATgPawn* Target = nullptr);

    // -changeteam / autobalance: the imminent eventSuicide() must not
    // count a death. Keyed by r_nPawnId (pointer keys are forbidden).
    static void SuppressNextDeath(int pawn_id);
    static bool ConsumeDeathSuppression(int pawn_id);

    // GameEngine__Tick: pending-death flush, objective capture/contest
    // accumulation, capture-moment detection. Self-throttled to 4 Hz.
    static void Tick();

    // BeginEndMission: bank + upsert every user's stints.
    static void FlushAll();
};
