#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Stats/DeviceStats.hpp"
#include "src/GameServer/TgGame/_effect_core/EffectCredit.hpp"
#include "src/Database/Database.hpp"
#include "sqlite3.h"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <map>
#include <set>
#include <string>

namespace {

bool g_enabled = false;

// Recording toggles (see MatchStats.hpp). Default true = record everything.
bool g_deviceStatsEnabled   = true;
bool g_effectivenessEnabled = true;

// user_id → per-match stint banking.
std::map<int64_t, Stats::UserMatchStats> g_users;

// Live-player identity, keyed by r_nPawnId (never by pointer —
// reference_pointer_keyed_maps_use_pawnid).
struct LivePlayer {
    int64_t user_id      = 0;
    int64_t character_id = 0;
    int     task_force   = 0;
};
std::map<int, LivePlayer> g_live;

// user_id → r_nPawnId of the CURRENT pawn. Guards against the stale-
// connection Cleanup that can fire AFTER the player already rejoined
// (NetConnection__Cleanup's has_other_connection case) — closing the
// fresh stint with a dead pawn's zeroed scores would bank negative deltas.
std::map<int64_t, int> g_userCurrentPawn;

// Per-device totals for the whole match, keyed by (character, task force,
// device). Not keyed by pawn or connection: a reconnect must keep adding to
// the same row, and the PRI's r_DeviceStats cannot express that.
struct DeviceKey {
    int64_t character_id = 0;
    int     task_force   = 0;
    int     device_id    = 0;
    bool operator<(const DeviceKey& o) const {
        if (character_id != o.character_id) return character_id < o.character_id;
        if (task_force   != o.task_force)   return task_force   < o.task_force;
        return device_id < o.device_id;
    }
};
struct DeviceTotals {
    int64_t user_id         = 0;
    int     damage          = 0;
    int     healing         = 0;
    int     player_kills    = 0;
    int     bot_kills       = 0;
    int     debuffs_removed = 0;  // cleanse strips (CleanseTracking)
    int     overheal        = 0;  // heal clamped away on full-health targets
    int     uses            = 0;  // DeviceFiring activations
    int     power_restored  = 0;  // prop-243 power that fit in the pool
    int     power_wasted    = 0;  // prop-243 power clamped away
    int     buffed_damage_dealt    = 0;  // dealt under this device's buff
    int     protected_damage_taken = 0;  // taken under this device's buff
    int     rescues         = 0;  // under-25% conditional deliveries:
                                  // Triage's own group + Savior triggers
    int     boost_targets     = 0;  // distinct boost applications (reach)
    int     boost_overwrites  = 0;  // live boosts replaced by another caster
    int     boost_wasted_secs = 0;  // lifetime lost to those overwrites
};
std::map<DeviceKey, DeviceTotals> g_deviceStats;

// Devices the server-side table must not carry. Two sources:
//   * every jetpack (asm slot 806) — with up to 20 players boosting
//     constantly, per-use rows would bloat the table with zero signal;
//   * an explicit list of self-maintenance devices the user ruled out of
//     performance tracking (with their tier variants): Bionics 2368,
//     Regeneration 2246/1950/2067/2068, Power Stim 3699/3696/3697/3698.
// Full exclusion — no row, no CLEANSE event. Extend the list here.
std::set<int> g_excludedDevices;
bool g_excludedLoaded = false;

bool DeviceExcluded(int device_id) {
    if (!g_excludedLoaded) {
        g_excludedLoaded = true;
        for (int id : {2368, 2246, 1950, 2067, 2068, 3699, 3696, 3697, 3698}) {
            g_excludedDevices.insert(id);
        }
        sqlite3* db = Database::GetConnection();
        if (db != nullptr) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                    "SELECT device_id FROM asm_data_set_devices "
                    "WHERE slot_used_value_id = 806",
                    -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    g_excludedDevices.insert(sqlite3_column_int(stmt, 0));
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    return g_excludedDevices.count(device_id) != 0;
}

// Devices whose activations additionally emit a timestamped DEVICE_USED
// event (the uses counter has no time axis; these events make per-cast
// effectiveness and match timelines reconstructable). Deliberately narrow —
// weapons would flood ga_match_events. Loaded once: the Group Heals family
// (asm skill 252: Healing/Protection/Frenzy/Power/Triage Wave, Healing
// Grenade, Purity) plus the three boosts. Extend by adding ids here.
std::set<int> g_castEventDevices;
bool g_castEventLoaded = false;

bool CastEventDevice(int device_id) {
    if (!g_castEventLoaded) {
        g_castEventLoaded = true;
        for (int id : {2838, 2773, 7559}) {  // Protection/Healing/Fashion Boost
            g_castEventDevices.insert(id);
        }
        sqlite3* db = Database::GetConnection();
        if (db != nullptr) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                    "SELECT i.item_id FROM asm_data_set_items i "
                    "JOIN asm_data_set_devices d ON d.device_id = i.item_id "
                    "WHERE i.skill_id = 252",
                    -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    g_castEventDevices.insert(sqlite3_column_int(stmt, 0));
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    return g_castEventDevices.count(device_id) != 0;
}

// Boost effect groups under overwrite tracking. Lifetime is carried here
// (verified against gaa.db) so the registry can tell "replaced early" from
// "expired then re-applied". Currently just the medic Healing Boost
// (200 HP/tick, 1s interval, 10s — an overwrite at t=3 costs 7 ticks);
// add Protection Boost {8964, 10.0f} / Fashion Boost {27646, 30.0f} here
// if they come into scope.
float BoostLifetime(int effect_group_id) {
    switch (effect_group_id) {
        case 8690: return 10.0f;  // Healing Boost (device 2773)
        default:   return 0.0f;   // not tracked
    }
}

// (caster character, device) → time of their last detected boost cast.
// Morale boosts never enter the DeviceFiring state (instance-210: two casts,
// uses=0, no DEVICE_USED), so cast detection lives here instead: a burst of
// applications shares one game_time, and a same-caster gap > 2s is a new
// cast (a real re-cast inside the window is morale-impossible anyway).
std::map<std::pair<int64_t, int>, float> g_lastBoostCast;

// (target r_nPawnId, effect group) → the live boost on that pawn. Caster
// identity is copied at apply time; no pawn pointers are retained.
struct BoostRec {
    int64_t user_id      = 0;
    int64_t character_id = 0;
    int     task_force   = 0;
    int     device_id    = 0;
    float   applied_at   = 0.0f;
    float   lifetime     = 0.0f;
};
std::map<std::pair<int, int>, BoostRec> g_boosts;

// Devices whose prop-243 power rider must not feed the power columns.
// Triage Wave's +250-on-rescue always overflows any pool (max reachable is
// 160), so it would read as pure waste when it is in fact the designed
// get-out-of-jail mechanic. Conditional-trigger tracking moves to the
// HP-gate / Group Heal Savior pass instead.
bool PowerStatExempt(int device_id) { return device_id == 5808; }

// Deaths waiting for a KILL to claim them; flushed as DEATH by Tick().
struct PendingDeath {
    int64_t user_id      = 0;
    int64_t character_id = 0;
    int     task_force   = 0;
    float   game_time    = 0.0f;
};
std::map<int, PendingDeath> g_pendingDeaths;  // keyed by victim r_nPawnId

std::set<int> g_suppressDeath;  // r_nPawnId — team-change suicides

// Objective capture-moment edge detection: objective id → last r_eStatus.
std::map<int, int> g_lastObjStatus;

float g_lastTick = 0.0f;
constexpr float kTickInterval       = 0.25f;  // 4 Hz
constexpr float kPendingDeathWindow = 1.0f;   // seconds before DEATH emits

float GameTime() {
    ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
    if (!Game || !Game->WorldInfo) return 0.0f;
    return Game->WorldInfo->TimeSeconds;
}

bool IsRealPlayer(AController* ctrl) {
    if (!ctrl) return false;
    return ObjectClassCache::ClassNameContains(ctrl, "PlayerController");
}

int TaskForceOf(ATgPawn* P) {
    if (!P || !P->PlayerReplicationInfo) return 0;
    ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)P->PlayerReplicationInfo;
    return PRI->r_TaskForce ? (int)PRI->r_TaskForce->r_nTaskForce : 0;
}

// Resolve a pawn to its live identity (players only; bots → false).
bool ResolveLive(ATgPawn* P, LivePlayer& out) {
    if (!P) return false;
    auto it = g_live.find(P->r_nPawnId);
    if (it == g_live.end()) return false;
    out = it->second;
    return true;
}

// Set player-or-bot identity fields on an event json.
// prefix: "actor" / "target" / "owner".
void FillIdentity(nlohmann::json& j, const char* prefix, ATgPawn* P) {
    if (!P) return;
    const std::string pfx(prefix);
    LivePlayer lp;
    if (ResolveLive(P, lp)) {
        j[pfx + "_user_id"]      = lp.user_id;
        j[pfx + "_character_id"] = lp.character_id;
        j[pfx + "_task_force"]   = TaskForceOf(P);
    } else if (!IsRealPlayer(P->Controller)) {
        // Bot: r_nProfileId carries asm_data_set_bots.bot_id (SpawnBotById).
        j[pfx + "_bot_id"]     = (int)P->r_nProfileId;
        j[pfx + "_task_force"] = TaskForceOf(P);
    }
}

void EmitEvent(nlohmann::json& j, const char* event_type) {
    j["type"]        = IpcProtocol::MSG_MATCH_EVENT;
    j["instance_id"] = IpcClient::GetInstanceId();
    j["game_time"]   = GameTime();
    j["event_type"]  = event_type;
    IpcClient::Send(j.dump());
    if (Logger::IsChannelEnabled("matchstats")) {
        Logger::Log("matchstats", "[Event] %s\n", j.dump().c_str());
    }
}

// Send absolute totals of every stint of one user (MSG_MATCH_STATS).
void UpsertUserStints(int64_t user_id) {
    auto uit = g_users.find(user_id);
    if (uit == g_users.end()) return;
    for (const auto& kv : uit->second.Stints()) {
        const Stats::StintKey& key = kv.first;
        const Stats::StintStats& s = kv.second;
        nlohmann::json m;
        m["type"]         = IpcProtocol::MSG_MATCH_STATS;
        m["instance_id"]  = IpcClient::GetInstanceId();
        m["user_id"]      = user_id;
        m["character_id"] = key.character_id;
        m["task_force"]   = key.task_force;
        nlohmann::json scores = nlohmann::json::array();
        for (int i = 0; i < Stats::kNumScores; i++) scores.push_back(s.scores[i]);
        m["scores"]                 = scores;
        m["capture_seconds"]        = s.capture_seconds;
        m["contest_seconds"]        = s.contest_seconds;
        m["objective_captures"]     = s.objective_captures;
        m["beacon_spawns_provided"] = s.beacon_spawns_provided;
        m["beacon_spawns_used"]     = s.beacon_spawns_used;
        m["beacons_destroyed"]      = s.beacons_destroyed;
        m["time_played_seconds"]    = s.seconds_played;
        IpcClient::Send(m.dump());
    }
}

// Send absolute per-device totals (MSG_MATCH_DEVICE_STATS). character_id 0
// sends every row; otherwise only that character's. Upsert is keyed
// (instance_id, character_id, task_force, device_id), so resending is safe.
void UpsertDeviceStats(int64_t character_id) {
    for (const auto& kv : g_deviceStats) {
        if (character_id != 0 && kv.first.character_id != character_id) continue;
        nlohmann::json m;
        m["type"]         = IpcProtocol::MSG_MATCH_DEVICE_STATS;
        m["instance_id"]  = IpcClient::GetInstanceId();
        m["user_id"]      = kv.second.user_id;
        m["character_id"] = kv.first.character_id;
        m["task_force"]   = kv.first.task_force;
        m["device_id"]    = kv.first.device_id;
        m["damage"]          = kv.second.damage;
        m["healing"]         = kv.second.healing;
        m["player_kills"]    = kv.second.player_kills;
        m["bot_kills"]       = kv.second.bot_kills;
        m["debuffs_removed"] = kv.second.debuffs_removed;
        m["overheal"]        = kv.second.overheal;
        m["uses"]            = kv.second.uses;
        m["power_restored"]  = kv.second.power_restored;
        m["power_wasted"]    = kv.second.power_wasted;
        m["buffed_damage_dealt"]    = kv.second.buffed_damage_dealt;
        m["protected_damage_taken"] = kv.second.protected_damage_taken;
        m["rescues"]                = kv.second.rescues;
        m["boost_targets"]          = kv.second.boost_targets;
        m["boost_overwrites"]       = kv.second.boost_overwrites;
        m["boost_wasted_secs"]      = kv.second.boost_wasted_secs;
        IpcClient::Send(m.dump());
    }
}

// Read the PRI's r_Scores into a plain array (zeros when PRI missing).
void ReadScores(ATgPawn* P, int out[Stats::kNumScores]) {
    for (int i = 0; i < Stats::kNumScores; i++) out[i] = 0;
    if (!P || !P->PlayerReplicationInfo) return;
    ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)P->PlayerReplicationInfo;
    for (int i = 0; i < Stats::kNumScores; i++) out[i] = PRI->r_Scores[i];
}

void BankPawn(ATgPawn* P, int64_t user_id) {
    int scores[Stats::kNumScores];
    ReadScores(P, scores);
    g_users[user_id].Bank(scores, GameTime());
}

Stats::StintStats* OpenStintOf(ATgPawn* P) {
    LivePlayer lp;
    if (!ResolveLive(P, lp)) return nullptr;
    auto uit = g_users.find(lp.user_id);
    if (uit == g_users.end()) return nullptr;
    return uit->second.OpenStint();
}

}  // namespace

void MatchStats::SetEnabled(bool enabled) {
    g_enabled = enabled;
    Logger::Log("matchstats", "[MatchStats] enabled=%d\n", (int)enabled);
}

bool MatchStats::Enabled() { return g_enabled; }

void MatchStats::SetDeviceStatsEnabled(bool enabled) {
    g_deviceStatsEnabled = enabled;
    Logger::Log("matchstats", "[MatchStats] device_stats_enabled=%d\n", (int)enabled);
}

void MatchStats::SetEffectivenessEnabled(bool enabled) {
    g_effectivenessEnabled = enabled;
    Logger::Log("matchstats", "[MatchStats] effectiveness_enabled=%d\n", (int)enabled);
}

bool MatchStats::EffectivenessEnabled() {
    return g_enabled && g_effectivenessEnabled;
}

void MatchStats::OnPlayerJoined(ATgPawn* Pawn, int64_t user_id,
                                int64_t character_id, int task_force) {
    if (!g_enabled || !Pawn || user_id == 0 || character_id == 0) return;

    Stats::UserMatchStats& u = g_users[user_id];

    // CLASS_CHANGE: user already played this match on another character.
    if (u.HasOpenStint() && u.OpenKey().character_id != character_id) {
        nlohmann::json ev;
        ev["actor_user_id"]      = user_id;
        ev["actor_character_id"] = u.OpenKey().character_id;
        ev["actor_task_force"]   = u.OpenKey().task_force;
        ev["detail"]             = character_id;  // the new character
        EmitEvent(ev, "CLASS_CHANGE");
    }
    const bool returning = u.HasAnyStint();

    int fresh[Stats::kNumScores];
    ReadScores(Pawn, fresh);
    u.BeginStint({character_id, task_force}, fresh, GameTime());

    // Reconnect restore: write banked sums into the fresh PRI so the
    // scoreboard keeps the running total; re-anchor the baseline so the
    // restored values don't double-bank.
    if (returning && Pawn->PlayerReplicationInfo) {
        int restored[Stats::kNumScores];
        u.SumScores(restored);
        ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
        for (int i = 0; i < Stats::kNumScores; i++) PRI->r_Scores[i] = restored[i];
        PRI->bNetDirty = 1;
        PRI->bForceNetUpdate = 1;
        u.RebaselineAfterRestore(restored);
        Logger::Log("matchstats",
            "[Join] restored scores user=%lld char=%lld (kills=%d deaths=%d)\n",
            (long long)user_id, (long long)character_id, restored[1], restored[8]);
    }

    g_live[Pawn->r_nPawnId] = {user_id, character_id, task_force};
    g_userCurrentPawn[user_id] = Pawn->r_nPawnId;

    nlohmann::json ev;
    ev["actor_user_id"]      = user_id;
    ev["actor_character_id"] = character_id;
    ev["actor_task_force"]   = task_force;
    EmitEvent(ev, "JOIN");
}

void MatchStats::OnPlayerLeft(ATgPawn* Pawn, int64_t user_id) {
    if (!g_enabled || user_id == 0) return;
    auto uit = g_users.find(user_id);
    if (uit == g_users.end()) return;

    // Stale-connection cleanup after a rejoin: the user's current pawn is
    // a different one. Drop the dead pawn's live entry, touch nothing else.
    auto cpit = g_userCurrentPawn.find(user_id);
    const bool isCurrent =
        Pawn && cpit != g_userCurrentPawn.end() && cpit->second == Pawn->r_nPawnId;
    if (!isCurrent) {
        if (Pawn) g_live.erase(Pawn->r_nPawnId);
        Logger::Log("matchstats",
            "[Leave] stale/pawnless cleanup user=%lld — no stint close\n",
            (long long)user_id);
        return;
    }

    LivePlayer lp;
    const bool live = ResolveLive(Pawn, lp);
    int scores[Stats::kNumScores];
    ReadScores(Pawn, scores);
    uit->second.CloseStint(scores, GameTime());
    UpsertUserStints(user_id);
    // Device rows survive the disconnect in g_deviceStats (keyed by character,
    // not connection) — this upsert just banks them early, like the stints.
    if (live) UpsertDeviceStats(lp.character_id);
    g_live.erase(Pawn->r_nPawnId);
    g_userCurrentPawn.erase(user_id);

    nlohmann::json ev;
    ev["actor_user_id"] = user_id;
    if (live) {
        ev["actor_character_id"] = lp.character_id;
        ev["actor_task_force"]   = lp.task_force;
    }
    EmitEvent(ev, "LEAVE");
}

void MatchStats::OnTeamChanged(ATgPawn* Pawn, int new_task_force) {
    if (!g_enabled || !Pawn) return;
    LivePlayer lp;
    if (!ResolveLive(Pawn, lp)) return;
    if (lp.task_force == new_task_force) return;

    BankPawn(Pawn, lp.user_id);
    int scores[Stats::kNumScores];
    ReadScores(Pawn, scores);
    g_users[lp.user_id].BeginStint({lp.character_id, new_task_force},
                                   scores, GameTime());
    g_live[Pawn->r_nPawnId].task_force = new_task_force;

    nlohmann::json ev;
    ev["actor_user_id"]      = lp.user_id;
    ev["actor_character_id"] = lp.character_id;
    ev["actor_task_force"]   = lp.task_force;   // the OLD team
    ev["detail"]             = new_task_force;  // the new team
    EmitEvent(ev, "TEAM_CHANGE");
}

void MatchStats::EmitMarker(const char* event_type, int64_t detail) {
    if (!g_enabled || !event_type || !*event_type) return;
    nlohmann::json ev;
    ev["detail"] = detail;
    EmitEvent(ev, event_type);
}

void MatchStats::OnChatCommand(ATgPawn* Actor, const char* event_type,
                               int64_t detail, ATgPawn* Target) {
    if (!g_enabled || !Actor || !event_type || !*event_type) return;
    LivePlayer lp;
    if (!ResolveLive(Actor, lp)) return;  // command sender is always a player
    nlohmann::json ev;
    ev["actor_user_id"]      = lp.user_id;
    ev["actor_character_id"] = lp.character_id;
    ev["actor_task_force"]   = lp.task_force;
    FillIdentity(ev, "target", Target);
    ev["detail"] = detail;
    EmitEvent(ev, event_type);
}

void MatchStats::OnDeath(ATgPawn* Victim) {
    if (!g_enabled || !Victim) return;
    // Death strips effects, so any boost the victim carried ended here —
    // clear its registry entries (bots included) so a boost applied after
    // respawn can't read as an overwrite of the pre-death one.
    for (auto it = g_boosts.begin(); it != g_boosts.end(); ) {
        if (it->first.first == Victim->r_nPawnId) it = g_boosts.erase(it);
        else ++it;
    }
    LivePlayer lp;
    if (!ResolveLive(Victim, lp)) return;  // bot deaths: no DEATH events
    PendingDeath pd;
    pd.user_id      = lp.user_id;
    pd.character_id = lp.character_id;
    pd.task_force   = lp.task_force;
    pd.game_time    = GameTime();
    g_pendingDeaths[Victim->r_nPawnId] = pd;
}

void MatchStats::OnKill(ATgPawn* CreditPawn, ATgPawn* PetPawn, ATgPawn* Victim,
                        int device_id, bool is_pet_kill, bool is_self_kill) {
    if (!g_enabled || !Victim) return;

    // This death is attributed — the pending DEATH must not also emit.
    g_pendingDeaths.erase(Victim->r_nPawnId);

    const bool victimIsPlayer = IsRealPlayer(Victim->Controller);
    const bool killerIsPlayer =
        CreditPawn != nullptr && IsRealPlayer(CreditPawn->Controller);
    if (!victimIsPlayer && !killerIsPlayer) return;  // bot-vs-bot: skip

    int flags = 0;
    if (is_self_kill) flags |= kFlagSelfKill;
    if (is_pet_kill)  flags |= kFlagPetKill;

    // Carrier kill: victim was holding the team beacon. r_BeaconHolder is
    // a PRI pointer, not a pawn pointer.
    if (victimIsPlayer && Victim->PlayerReplicationInfo) {
        ATgRepInfo_Player* VPRI = (ATgRepInfo_Player*)Victim->PlayerReplicationInfo;
        if (VPRI->r_TaskForce && VPRI->r_TaskForce->r_BeaconManager &&
            VPRI->r_TaskForce->r_BeaconManager->r_BeaconHolder == VPRI) {
            flags |= kFlagVictimCarriedBeacon;
        }
    }

    nlohmann::json ev;
    FillIdentity(ev, "actor", CreditPawn);
    FillIdentity(ev, "target", Victim);
    if (device_id != 0) ev["device_id"] = device_id;
    if (is_pet_kill && PetPawn) ev["detail"] = (int)PetPawn->r_nProfileId;
    if (flags != 0) ev["flags"] = flags;

    // Assists — same selection TrackKill / the alert block uses: up to two
    // damagers from the victim's rotated history + the killer's last
    // healer. Player victims only (matches scoreboard + alert policy).
    if (victimIsPlayer && !is_self_kill && CreditPawn != nullptr) {
        nlohmann::json assists = nlohmann::json::array();
        ATgPawn* damageAssists[2] = {
            Victim->m_LastDamager, Victim->m_SecondToLastDamager };
        for (int i = 0; i < 2; i++) {
            ATgPawn* a = damageAssists[i];
            if (!a || a == Victim || a == CreditPawn) continue;
            if (i == 1 && a == damageAssists[0]) continue;
            LivePlayer alp;
            if (!ResolveLive(a, alp)) continue;
            nlohmann::json aj;
            aj["user_id"]      = alp.user_id;
            aj["character_id"] = alp.character_id;
            aj["task_force"]   = alp.task_force;
            assists.push_back(aj);
        }
        ATgPawn* healer = CreditPawn->m_LastHealer;
        if (healer && healer != Victim && healer != CreditPawn) {
            LivePlayer hlp;
            if (ResolveLive(healer, hlp)) {
                nlohmann::json aj;
                aj["user_id"]      = hlp.user_id;
                aj["character_id"] = hlp.character_id;
                aj["task_force"]   = hlp.task_force;
                assists.push_back(aj);
            }
        }
        if (!assists.empty()) ev["assists"] = assists;
    }

    EmitEvent(ev, "KILL");
}

void MatchStats::OnDeployableDestroyed(ATgPawn* Destroyer,
                                       ATgDeployable* Deployable,
                                       bool is_beacon) {
    if (!g_enabled || !Destroyer || !Deployable) return;
    LivePlayer dlp;
    if (!ResolveLive(Destroyer, dlp)) return;  // bot wrecking gear: skip

    nlohmann::json ev;
    ev["actor_user_id"]      = dlp.user_id;
    ev["actor_character_id"] = dlp.character_id;
    ev["actor_task_force"]   = dlp.task_force;
    // Deployer attribution ("who owned the turret").
    ATgPawn* deployer = (ATgPawn*)Deployable->Instigator;
    LivePlayer olp;
    if (deployer && ResolveLive(deployer, olp)) {
        ev["owner_user_id"]      = olp.user_id;
        ev["owner_character_id"] = olp.character_id;
    }
    if (!is_beacon) ev["detail"] = (int64_t)Deployable->r_nDeployableId;

    if (is_beacon) {
        if (Stats::StintStats* s = OpenStintOf(Destroyer)) s->beacons_destroyed++;
        EmitEvent(ev, "BEACON_DESTROYED");
    } else {
        EmitEvent(ev, "DEPLOYABLE_DESTROYED");
    }
}

void MatchStats::OnDeviceCredit(ATgPawn* CreditPawn, int device_id,
                                int field, int amount) {
    if (!g_enabled || !g_deviceStatsEnabled) return;  // toggle A
    if (!CreditPawn || device_id <= 0 || amount <= 0) return;
    if (DeviceExcluded(device_id)) return;
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;  // bots have no persisted row

    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    switch (field) {
        case DeviceStats::kDamage:      t.damage       += amount; break;
        case DeviceStats::kHealing:     t.healing      += amount; break;
        case DeviceStats::kPlayerKills: t.player_kills += amount; break;
        case DeviceStats::kBotKills:    t.bot_kills    += amount; break;
        default: break;  // kId + the derived DPM/HPM slots aren't counters
    }
}

void MatchStats::OnDeviceCleanse(ATgPawn* CreditPawn, ATgPawn* TargetPawn,
                                 int device_id, int category, int removed) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!CreditPawn || removed <= 0) return;
    if (DeviceExcluded(device_id)) return;  // no row AND no event
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;  // bot cleanses: no record

    // device_id 0 (unresolved origin) still counts on the player via the
    // event below, but can't take a per-device row.
    if (device_id > 0) {
        DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
        t.user_id = lp.user_id;
        t.debuffs_removed += removed;
    }

    nlohmann::json ev;
    ev["actor_user_id"]      = lp.user_id;
    ev["actor_character_id"] = lp.character_id;
    ev["actor_task_force"]   = lp.task_force;
    FillIdentity(ev, "target", TargetPawn);
    ev["device_id"] = device_id;
    ev["detail"]    = (int64_t)category;  // which category came off
    ev["flags"]     = removed;            // how many groups of it
    EmitEvent(ev, "CLEANSE");
}

void MatchStats::OnDeviceOverheal(ATgPawn* CreditPawn, int device_id,
                                  int amount) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!CreditPawn || device_id <= 0 || amount <= 0) return;
    if (DeviceExcluded(device_id)) return;
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;

    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    t.overheal += amount;
}

void MatchStats::OnDeviceUsed(ATgPawn* UserPawn, int device_id) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!UserPawn || device_id <= 0) return;
    if (DeviceExcluded(device_id)) return;  // jetpacks land here constantly
    LivePlayer lp;
    if (!ResolveLive(UserPawn, lp)) return;
    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    t.uses++;

    // Allowlisted activatables also emit a timestamped cast event, so
    // per-cast effectiveness ("3 casts, which cured?") joins against the
    // CLEANSE / SAVIOR stream on game_time instead of being inferred.
    if (CastEventDevice(device_id)) {
        nlohmann::json ev;
        ev["actor_user_id"]      = lp.user_id;
        ev["actor_character_id"] = lp.character_id;
        ev["actor_task_force"]   = lp.task_force;
        ev["device_id"]          = device_id;
        EmitEvent(ev, "DEVICE_USED");
    }
}

void MatchStats::OnDevicePowerRestore(ATgPawn* CreditPawn, int device_id,
                                      int restored, int wasted) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!CreditPawn || device_id <= 0) return;
    if (restored <= 0 && wasted <= 0) return;
    if (DeviceExcluded(device_id)) return;
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;
    if (PowerStatExempt(device_id)) {
        // Triage: the power rider stays out of the power columns (see
        // PowerStatExempt), but its firing IS the under-25% conditional —
        // one power effect per rescue — so it drives the rescues counter.
        DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
        t.user_id = lp.user_id;
        t.rescues++;
        if (Logger::IsChannelEnabled("devusage")) {
            Logger::Log("devusage",
                "[TRIAGE-CONDITIONAL] credit=%p restored=%d wasted=%d (power stats exempt, rescue counted)\n",
                (void*)CreditPawn, restored, wasted);
        }
        return;
    }
    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    if (restored > 0) t.power_restored += restored;
    if (wasted > 0)   t.power_wasted   += wasted;
}

void MatchStats::OnSaviorTrigger(ATgPawn* CreditPawn, ATgPawn* TargetPawn,
                                 int device_id) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!CreditPawn) return;
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;  // bot-cast heals: no record
    // Savior firing = an under-25% rescue delivered by this device.
    // Self-rescue (a grenade at your own feet while low — observed live in
    // instance 210) emits the event but stays out of the counter, per the
    // skill's authored "hit a teammate" intent and the same self-exclusion
    // the heal credit applies. Filter events on actor==target to study them.
    if (device_id > 0 && !DeviceExcluded(device_id) && CreditPawn != TargetPawn) {
        DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
        t.user_id = lp.user_id;
        t.rescues++;
    }
    nlohmann::json ev;
    ev["actor_user_id"]      = lp.user_id;
    ev["actor_character_id"] = lp.character_id;
    ev["actor_task_force"]   = lp.task_force;
    FillIdentity(ev, "target", TargetPawn);
    if (device_id > 0) ev["device_id"] = device_id;
    ev["detail"] = (int64_t)852;  // the skill, for symmetry with CLEANSE's category
    EmitEvent(ev, "SAVIOR");
}

void MatchStats::OnBoostApply(UTgEffectGroup* g) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!g || !g->m_Target) return;
    const float lifetime = BoostLifetime(g->m_nEffectGroupId);
    if (lifetime <= 0.0f) return;
    if (!ObjectClassCache::ClassNameContains(g->m_Target, "TgPawn")) return;
    ATgPawn* target = static_cast<ATgPawn*>(g->m_Target);

    ATgPawn* credit = EffectCredit::ResolveCreditPawn(g->m_Instigator);
    if (!credit) return;
    LivePlayer lp;
    if (!ResolveLive(credit, lp)) return;  // bot-cast boosts: no record
    const int device_id = EffectCredit::ResolveDeviceId(g, credit);
    if (device_id <= 0 || DeviceExcluded(device_id)) return;

    const float now = GameTime();
    const std::pair<int, int> key{target->r_nPawnId, g->m_nEffectGroupId};
    auto it = g_boosts.find(key);
    if (it != g_boosts.end() && (now - it->second.applied_at) < it->second.lifetime) {
        // Same caster + device inside the window: a HoT tick or another
        // effect of the same application — same record, nothing to count.
        // A same-caster RE-cast inside the window is mechanically
        // impossible: boosts are morale-priced with no cooldown, and
        // morale cannot regenerate while the caster's own boost is live.
        if (it->second.character_id == lp.character_id &&
            it->second.device_id == device_id) {
            return;
        }
        // Different caster: newest-wins overwrite. The remaining lifetime
        // of the old instance is the wasted investment.
        const int remaining = (int)(it->second.lifetime - (now - it->second.applied_at));
        DeviceTotals& old_row = g_deviceStats[{it->second.character_id,
                                               it->second.task_force,
                                               it->second.device_id}];
        old_row.user_id = it->second.user_id;
        old_row.boost_overwrites++;
        old_row.boost_wasted_secs += remaining > 0 ? remaining : 0;

        nlohmann::json ev;
        ev["actor_user_id"]      = lp.user_id;             // the overwriter
        ev["actor_character_id"] = lp.character_id;
        ev["actor_task_force"]   = lp.task_force;
        FillIdentity(ev, "target", target);
        ev["owner_user_id"]      = it->second.user_id;     // the wronged medic
        ev["owner_character_id"] = it->second.character_id;
        ev["device_id"]          = it->second.device_id;   // the wasted boost
        ev["detail"]             = (int64_t)(remaining > 0 ? remaining : 0);
        EmitEvent(ev, "BOOST_OVERWRITE");
    }

    // Fresh application (first, post-expiry, or the overwriting one): the
    // new caster's reach grows and the registry now tracks their instance.
    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    t.boost_targets++;
    g_boosts[key] = BoostRec{lp.user_id, lp.character_id, lp.task_force,
                             device_id, now, lifetime};

    // Cast detection (see g_lastBoostCast): first application of a burst
    // counts the use and emits the DEVICE_USED row the DeviceFiring hook
    // can't provide for morale abilities.
    float& lastCast = g_lastBoostCast[{lp.character_id, device_id}];
    if (now - lastCast > 2.0f || lastCast == 0.0f) {
        lastCast = now;
        t.uses++;
        nlohmann::json used;
        used["actor_user_id"]      = lp.user_id;
        used["actor_character_id"] = lp.character_id;
        used["actor_task_force"]   = lp.task_force;
        used["device_id"]          = device_id;
        EmitEvent(used, "DEVICE_USED");
    }

    // Per-application event so a cast's fresh-vs-stomped split is exact:
    // a cast's BOOST_APPLY rows minus its BOOST_OVERWRITE rows = targets
    // that had nothing running — the "was this cast justified" numerator.
    // Morale-priced casts are rare, so the event volume is trivial.
    nlohmann::json ap;
    ap["actor_user_id"]      = lp.user_id;
    ap["actor_character_id"] = lp.character_id;
    ap["actor_task_force"]   = lp.task_force;
    FillIdentity(ap, "target", target);
    ap["device_id"] = device_id;
    EmitEvent(ap, "BOOST_APPLY");
}

void MatchStats::OnBuffWindowDamage(ATgPawn* CreditPawn, int device_id,
                                    bool offensive, int amount) {
    if (!g_enabled || !g_effectivenessEnabled) return;  // toggle B
    if (!CreditPawn || device_id <= 0 || amount <= 0) return;
    if (DeviceExcluded(device_id)) return;
    LivePlayer lp;
    if (!ResolveLive(CreditPawn, lp)) return;
    DeviceTotals& t = g_deviceStats[{lp.character_id, lp.task_force, device_id}];
    t.user_id = lp.user_id;
    if (offensive) t.buffed_damage_dealt    += amount;
    else           t.protected_damage_taken += amount;
}

void MatchStats::OnBeaconSpawnUsed(ATgPawn* User, ATgPawn* Deployer) {
    if (!g_enabled || !User || !Deployer) return;
    LivePlayer ulp;
    if (!ResolveLive(User, ulp)) return;  // bots don't count as usage

    if (Stats::StintStats* s = OpenStintOf(User)) s->beacon_spawns_used++;
    LivePlayer dlp;
    if (ResolveLive(Deployer, dlp)) {
        if (Stats::StintStats* s = OpenStintOf(Deployer)) s->beacon_spawns_provided++;
    }

    nlohmann::json ev;
    ev["actor_user_id"]      = ulp.user_id;
    ev["actor_character_id"] = ulp.character_id;
    ev["actor_task_force"]   = ulp.task_force;
    if (dlp.user_id != 0) {
        ev["owner_user_id"]      = dlp.user_id;
        ev["owner_character_id"] = dlp.character_id;
    }
    EmitEvent(ev, "BEACON_SPAWN");
}

void MatchStats::SuppressNextDeath(int pawn_id) {
    if (!g_enabled) return;
    g_suppressDeath.insert(pawn_id);
}

bool MatchStats::ConsumeDeathSuppression(int pawn_id) {
    auto it = g_suppressDeath.find(pawn_id);
    if (it == g_suppressDeath.end()) return false;
    g_suppressDeath.erase(it);
    return true;
}

void MatchStats::Tick() {
    if (!g_enabled) return;
    const float now = GameTime();
    if (now - g_lastTick < kTickInterval) return;
    const float dt = (g_lastTick > 0.0f) ? (now - g_lastTick) : 0.0f;
    g_lastTick = now;

    // 0. Periodic device-stats safety flush. Rows otherwise persist only at
    //    clean leave / mission end, and instances torn down without either
    //    (test sessions, crashes) lost every still-connected player's rows
    //    (observed instances 204 and 210). Upserts are idempotent, so a
    //    minute-cadence resend costs a handful of IPC messages and caps the
    //    loss window at 60s.
    static float s_lastDeviceFlush = 0.0f;
    if (now - s_lastDeviceFlush > 60.0f) {
        s_lastDeviceFlush = now;
        UpsertDeviceStats(0);
    }

    // 1. Flush pending deaths nothing claimed (environment / fall / team
    //    damage kills) as killer-less DEATH events.
    for (auto it = g_pendingDeaths.begin(); it != g_pendingDeaths.end();) {
        if (now - it->second.game_time >= kPendingDeathWindow) {
            nlohmann::json ev;
            ev["target_user_id"]      = it->second.user_id;
            ev["target_character_id"] = it->second.character_id;
            ev["target_task_force"]   = it->second.task_force;
            EmitEvent(ev, "DEATH");
            it = g_pendingDeaths.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Objective capture/contest time + capture-moment detection.
    //    Status (TgMissionObjective.uc enum): 2=PAUSED_CONTESTED,
    //    5/6=INPROGRESS (def/att), 7/8=CAPTURED (def/att).
    if (dt <= 0.0f) return;
    for (ATgMissionObjective* Obj : ActorCache::MissionObjectives) {
        if (!Obj) continue;
        const std::string& ocname = ObjectClassCache::GetClassName(Obj->Class);
        if (ocname.find("TgMissionObjective_Proximity") == std::string::npos &&
            ocname.find("TgMissionObjective_Escort")    == std::string::npos) continue;
        ATgMissionObjective_Proximity* PObj = (ATgMissionObjective_Proximity*)Obj;
        if (!PObj->s_CollisionProxy) continue;

        const int status = (int)Obj->r_eStatus;
        int prev = -1;
        {
            auto it = g_lastObjStatus.find(Obj->nObjectiveId);
            if (it != g_lastObjStatus.end()) prev = it->second;
        }
        g_lastObjStatus[Obj->nObjectiveId] = status;

        const bool advancing = (status == 5 || status == 6);
        const bool contested = (status == 2);
        const bool justCaptured =
            (status == 7 || status == 8) && prev != status && prev != -1;
        // 8 = ATTACKER_CAPTURED → coalition 1 took it; 7 → coalition 2.
        const int capturingCoalition = (status == 8) ? 1 : 2;

        if (!advancing && !contested && !justCaptured) continue;

        for (int i = 0; i < PObj->s_CollisionProxy->m_NearByPlayers.Count; i++) {
            ATgPawn* P = PObj->s_CollisionProxy->m_NearByPlayers.Data[i];
            if (!P || !IsRealPlayer(P->Controller)) continue;
            Stats::StintStats* s = OpenStintOf(P);
            if (!s) continue;

            // Advancing: only the capturing side is on the point (UC only
            // reaches INPROGRESS with no enemy contesting). Contested:
            // both sides earn contest credit — blocker and blocked alike.
            if (advancing) s->capture_seconds += dt;
            if (contested) s->contest_seconds += dt;

            if (justCaptured && P->PlayerReplicationInfo) {
                ATgRepInfo_Player* PRI =
                    (ATgRepInfo_Player*)P->PlayerReplicationInfo;
                if (PRI->r_TaskForce &&
                    (int)PRI->r_TaskForce->r_eCoalition == capturingCoalition) {
                    s->objective_captures++;
                    LivePlayer lp;
                    if (ResolveLive(P, lp)) {
                        nlohmann::json ev;
                        ev["actor_user_id"]      = lp.user_id;
                        ev["actor_character_id"] = lp.character_id;
                        ev["actor_task_force"]   = lp.task_force;
                        ev["detail"]             = (int64_t)Obj->nObjectiveId;
                        EmitEvent(ev, "OBJECTIVE_CAPTURED");
                    }
                }
            }
        }
    }
}

void MatchStats::FlushAll() {
    if (!g_enabled) return;
    Logger::Log("matchstats", "[FlushAll] %zu live player(s), %zu user(s)\n",
        g_live.size(), g_users.size());
    // Bank every live player's open stint at current PRI values, then
    // upsert every user's stints (already-left users were upserted at
    // leave; the resend is idempotent).
    for (const auto& kv : g_live) {
        for (const auto& ps : GPawnSessions) {
            ATgPawn* P = ps.first;
            if (P && P->r_nPawnId == kv.first) {
                BankPawn(P, kv.second.user_id);
                break;
            }
        }
    }
    for (const auto& kv : g_users) {
        UpsertUserStints(kv.first);
    }
    Logger::Log("matchstats", "[FlushAll] %zu device row(s)\n",
        g_deviceStats.size());
    UpsertDeviceStats(0);
}
