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

    // Recording toggles (INSTANCE_HELLO_ACK ← control-server.json, both
    // default true). Independent of the master enable above, which still
    // zeroes everything on home maps.
    //   device stats  — the server-side mirror of the client Device Stats
    //                   tab counters (damage/healing/kills via OnDeviceCredit)
    //   effectiveness — everything layered on top: cleanse/boost/power/
    //                   savior tracking, buff windows, uses, DEVICE_USED
    // The client's own tab is unaffected by either. EffectivenessEnabled()
    // is public so upstream recorders (BuffWindowTracking's per-bullet
    // scans) can skip their work entirely, not just discard results.
    static void SetDeviceStatsEnabled(bool enabled);
    static void SetEffectivenessEnabled(bool enabled);
    static bool EffectivenessEnabled();

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

    // Server-side mirror of every DeviceStats::Credit. `field` is the
    // FDeviceStatInfo::Stats index the PRI row is about to take (kDamage /
    // kHealing / kPlayerKills / kBotKills); the derived DPM/HPM slots are
    // display-only and ignored here.
    //
    // Accumulates per (character, task force, device) for the whole match and
    // upserts to ga_match_device_stats at leave + FlushAll. Deliberately NOT
    // read back out of r_DeviceStats: that array is per-connection (wiped on
    // reconnect) and caps at 9 rows, neither of which is acceptable for a
    // persisted record. Credits arrive as increments, so summing them needs
    // none of the r_Scores baseline/stint machinery.
    static void OnDeviceCredit(ATgPawn* CreditPawn, int device_id,
                               int field, int amount);

    // Effectiveness pass. Both land on the same per-device row as
    // OnDeviceCredit; cleanses additionally emit a CLEANSE event (actor =
    // cleanser, target = cleansed pawn, device_id, detail = category code
    // purged, flags = strip count) because "which category came off whom" is
    // context a bare counter cannot hold.
    //
    // OnDeviceCleanse ← CleanseTracking::OnRemoved, removed > 0 only.
    // OnDeviceOverheal ← TrackStats heal path: the magnitude the
    // missing-health clamp threw away (heal cast on a topped-up target).
    static void OnDeviceCleanse(ATgPawn* CreditPawn, ATgPawn* TargetPawn,
                                int device_id, int category, int removed);
    static void OnDeviceOverheal(ATgPawn* CreditPawn, int device_id,
                                 int amount);

    // One DeviceFiring activation (ProcessEvent DeviceFiringBeginState) —
    // the "used" denominator. Per hold for continuous fire.
    static void OnDeviceUsed(ATgPawn* UserPawn, int device_id);

    // Property-243 Power Pool restore, split at apply time like the heal
    // clamp: restored = what fit in the target's pool, wasted = the rest.
    // Triage Wave (5808) is exempt from the power columns: its +250 rider
    // always overflows the 160-max pool and is a designed rescue mechanic,
    // not wasteful play. Its conditional firing logs on `devusage` only.
    static void OnDevicePowerRestore(ATgPawn* CreditPawn, int device_id,
                                     int restored, int wasted);

    // Group Heal Savior (skill 852, eg 16587) fired: a group heal landed on
    // a teammate under 25% with the skill slotted. Emits a SAVIOR event
    // (actor = medic, target = rescued pawn, device_id = the delivering
    // group heal, detail = 852). The source-device field on skill instances
    // was verified reliable by single-cast tests (instances 207/208).
    // Triage Wave never procs this skill — its rescues are visible via its
    // own conditional group instead.
    static void OnSaviorTrigger(ATgPawn* CreditPawn, ATgPawn* TargetPawn,
                                int device_id);

    // A tracked boost effect group landed on a pawn (called from
    // CheckEffectBuffModifier for every effect of every application AND
    // every HoT tick — dedupe happens inside). Maintains the per-target
    // boost registry that drives three counters on the caster's device row:
    //   boost_targets    — distinct applications (reach; ticks and the
    //                      group's 2nd/3rd effects fold into their record)
    //   boost_overwrites — times THIS device's live boost was replaced by a
    //                      different caster before expiry (newest-wins)
    //   boost_wasted_secs— lifetime remaining at those overwrites, summed
    // Every fresh application emits a BOOST_APPLY event, and each overwrite
    // additionally a BOOST_OVERWRITE event (actor = the overwriter, owner =
    // the medic whose ticks were lost, target = the pawn, device_id = the
    // wasted device, detail = seconds remaining). A cast's APPLY rows minus
    // its OVERWRITE rows = targets that had nothing running, so "was the
    // second medic's cast justified" is per-incident data, not a judgment
    // baked into a counter.
    // Registry is (target pawnId, effect group) keyed with copied caster
    // identity — no pawn pointers survive in it — and entries clear on the
    // target's death so a post-respawn boost can't read as an overwrite.
    static void OnBoostApply(UTgEffectGroup* g);

    // Damage that happened inside a tracked buff window (BuffWindowTracking):
    // offensive → buffed_damage_dealt on the buff device's row, defensive →
    // protected_damage_taken. Credit is the buff's caster, not the shooter.
    static void OnBuffWindowDamage(ATgPawn* CreditPawn, int device_id,
                                   bool offensive, int amount);

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
