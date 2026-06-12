#include "src/GameServer/Stats/MatchStats.hpp"
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

void MatchStats::OnDeath(ATgPawn* Victim) {
    if (!g_enabled || !Victim) return;
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
}
