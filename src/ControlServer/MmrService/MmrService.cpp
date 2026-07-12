#include "src/ControlServer/MmrService/MmrService.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

std::mutex MmrService::mutex_;

namespace {

// Constants mirror the analyst's compute_mmr.py / mmr_tracker.py. Changing
// any of them invalidates the sequential rating chain — change + reseed
// together (both engines).
constexpr double kDefaultMmr       = 1000.0;
constexpr int    kMinPlaySeconds   = 180;
// W/L engine
constexpr double kWlK              = 32.0;
// Perf engine
constexpr double kPerfKBase        = 24.0;
constexpr double kPerfKProvisional = 48.0;
constexpr int    kPerfMinGames     = 5;
constexpr double kPerfBeta         = 0.35;
constexpr double kPerfZClamp       = 3.0;
// Team-size imbalance correction, calibrated on this server's data
// (>=1.0 avg player gap -> larger side wins ~79%).
constexpr double kEloPerPlayerGap  = 220.0;
constexpr double kGapDeadzone      = 0.25;
constexpr double kMaxGapCap        = 3.0;

constexpr int kNumStats = 7;
// stat order: kills, assists, deaths, damage_dealt, healing, obj_points, bot_kills
constexpr double kStatSign[kNumStats] = {1, 1, -1, 1, 1, 1, 1};

using RatingKey = std::pair<int64_t, std::string>;  // (user_id, class_name)

const char* ClassNameForProfile(int profile_id) {
    switch (profile_id) {
        case 680: return "Assault";
        case 567: return "Medic";
        case 681: return "Recon";
        case 679: return "Robotic";
        default:  return nullptr;
    }
}

struct Stint {
    std::string cls;
    int         task_force = 0;
    double      time = 0.0;
    double      stats[kNumStats] = {};
};

struct Participant {
    int64_t     user_id = 0;
    int         task_force = 0;
    std::string cls;
    double      class_time = 0.0;
    double      stats[kNumStats] = {};
    double      perf = 0.0;  // z-score performance index, filled after baselines
};

struct Match {
    int64_t id = 0;
    int64_t started_at = 0;
    int     winning_tf = 0;
    std::vector<Participant> players;
};

double ExpectedScore(double rating, double opp_avg) {
    return 1.0 / (1.0 + std::pow(10.0, (opp_avg - rating) / 400.0));
}

struct GapEvent {
    std::string type;
    double      game_time = 0.0;
    int64_t     actor = 0;
    int         actor_tf = 0;
    int         target_tf = 0;
};

// Time-weighted average concurrent headcount difference during the match's
// stable window (post join-ramp, pre teardown). Returns (bigger_tf, gap);
// bigger_tf 0 = event log can't support it, treat as no adjustment.
std::pair<int, double> GetTeamSizeGap(sqlite3* db, int64_t instance_id) {
    std::vector<GapEvent> evs;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT event_type, game_time, COALESCE(actor_user_id, 0), "
            "       COALESCE(actor_task_force, 0), COALESCE(target_task_force, 0) "
            "FROM ga_match_events "
            "WHERE instance_id = ? AND event_type IN ('JOIN', 'LEAVE', 'TEAM_CHANGE') "
            "  AND game_time IS NOT NULL "
            "ORDER BY ts", -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return {0, 0.0};
    }
    sqlite3_bind_int64(stmt, 1, instance_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GapEvent e;
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        e.type      = t ? t : "";
        e.game_time = sqlite3_column_double(stmt, 1);
        e.actor     = sqlite3_column_int64(stmt, 2);
        e.actor_tf  = sqlite3_column_int(stmt, 3);
        e.target_tf = sqlite3_column_int(stmt, 4);
        evs.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);

    if (evs.size() < 2) return {0, 0.0};

    size_t i = 0;
    while (i < evs.size() && evs[i].type == "JOIN") i++;
    const double settle_t = evs[i > 0 ? i - 1 : 0].game_time;

    int j = static_cast<int>(evs.size()) - 1;
    while (j >= 0 && evs[j].type == "LEAVE") j--;
    const double teardown_t = (j + 1 < static_cast<int>(evs.size()))
        ? evs[j + 1].game_time : evs.back().game_time;

    if (teardown_t <= settle_t) return {0, 0.0};

    std::map<int64_t, int> state;  // user -> current task force
    double weighted1 = 0.0, weighted2 = 0.0;
    double last_t = settle_t;

    auto apply = [&state](const GapEvent& e) {
        if (e.type == "JOIN")       state[e.actor] = e.actor_tf;
        else if (e.type == "LEAVE") state.erase(e.actor);
        else                        state[e.actor] = e.target_tf;  // TEAM_CHANGE
    };

    for (const auto& e : evs) {
        if (e.game_time > settle_t) break;
        apply(e);
    }
    for (const auto& e : evs) {
        if (e.game_time <= settle_t) continue;
        if (e.game_time > teardown_t) break;
        const double dt = e.game_time - last_t;
        for (const auto& entry : state) {
            if (entry.second == 1) weighted1 += dt;
            else if (entry.second == 2) weighted2 += dt;
        }
        apply(e);
        last_t = e.game_time;
    }

    const double dur = teardown_t - settle_t;
    if (dur <= 0) return {0, 0.0};

    const double avg1 = weighted1 / dur, avg2 = weighted2 / dur;
    return {avg1 > avg2 ? 1 : 2, std::fabs(avg1 - avg2)};
}

// Concluded PvP matches (wins and stalemates) in conclusion-time order
// (deviation from the analyst's started_at order: live folds happen at
// conclusion, and a reseed must reproduce the live chain).
std::vector<Match> LoadMatches(sqlite3* db) {
    std::vector<Match> matches;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT i.id, i.started_at, COALESCE(i.winning_task_force, 0), "
            "       COALESCE(NULLIF(i.end_mission_at, 0), NULLIF(i.sealed_at, 0), "
            "                i.started_at) AS concluded_at "
            "FROM ga_instances i "
            "WHERE i.outcome IN ('ATTACKERS_WIN', 'DEFENDERS_WIN', 'STALEMATE') "
            "  AND EXISTS (SELECT 1 FROM map_game_info m "
            "              WHERE m.map_name = i.map_name AND m.is_pvp = 1) "
            "ORDER BY concluded_at ASC, i.id ASC",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("mmr", "[MmrService] match query prepare failed: %s\n",
            sqlite3_errmsg(db));
        return matches;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Match m;
        m.id         = sqlite3_column_int64(stmt, 0);
        m.started_at = sqlite3_column_int64(stmt, 1);
        m.winning_tf = sqlite3_column_int(stmt, 2);
        matches.push_back(std::move(m));
    }
    sqlite3_finalize(stmt);
    return matches;
}

// Shared participation rule (identical to both scripts): roster de-duped to
// one class per character, non-zero activity, finishing-class credit (longest
// stint), >= kMinPlaySeconds summed in that class.
void BuildParticipants(sqlite3* db, std::vector<Match>& matches) {
    std::map<int64_t, Match*> by_id;
    for (auto& m : matches) by_id[m.id] = &m;

    std::map<std::pair<int64_t, int64_t>, std::vector<Stint>> stints;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT s.instance_id, s.user_id, s.task_force, r.profile_id, "
            "       s.time_played_seconds, "
            "       s.kills, s.assists, s.deaths, s.damage_dealt, s.healing, "
            "       s.obj_points, s.bot_kills "
            "FROM ga_match_player_stats s "
            "JOIN ga_instances i ON i.id = s.instance_id "
            "JOIN (SELECT instance_id, character_id, MIN(profile_id) AS profile_id "
            "      FROM ga_instance_players "
            "      WHERE profile_id IN (680, 567, 681, 679) "
            "      GROUP BY instance_id, character_id) r "
            "  ON r.instance_id = s.instance_id AND r.character_id = s.character_id "
            "WHERE i.outcome IN ('ATTACKERS_WIN', 'DEFENDERS_WIN', 'STALEMATE') "
            "  AND (s.kills + s.assists + s.deaths + s.damage_dealt "
            "       + s.healing + s.obj_points + s.bot_kills) > 0 "
            "  AND EXISTS (SELECT 1 FROM map_game_info m "
            "              WHERE m.map_name = i.map_name AND m.is_pvp = 1)",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("mmr", "[MmrService] participant query prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int64_t inst = sqlite3_column_int64(stmt, 0);
        if (!by_id.count(inst)) continue;
        const char* cls = ClassNameForProfile(sqlite3_column_int(stmt, 3));
        if (!cls) continue;
        Stint st;
        st.cls        = cls;
        st.task_force = sqlite3_column_int(stmt, 2);
        st.time       = sqlite3_column_double(stmt, 4);
        for (int k = 0; k < kNumStats; k++)
            st.stats[k] = sqlite3_column_double(stmt, 5 + k);
        stints[{inst, sqlite3_column_int64(stmt, 1)}].push_back(std::move(st));
    }
    sqlite3_finalize(stmt);

    for (auto& entry : stints) {
        std::vector<Stint>& grp = entry.second;
        std::stable_sort(grp.begin(), grp.end(),
            [](const Stint& a, const Stint& b) { return a.time > b.time; });
        const Stint& fin = grp.front();
        Participant p;
        p.user_id    = entry.first.second;
        p.cls        = fin.cls;
        p.task_force = fin.task_force;
        for (const auto& s : grp) {
            if (s.cls != p.cls) continue;
            p.class_time += s.time;
            for (int k = 0; k < kNumStats; k++) p.stats[k] += s.stats[k];
        }
        if (p.class_time < kMinPlaySeconds) continue;
        by_id[entry.first.first]->players.push_back(std::move(p));
    }
}

// Perf engine: class-population baselines of per-minute stat rates over ALL
// qualifying records (recomputed each fold, same as mmr_tracker.py — baselines
// drift as data accumulates; only the wl engine is exactly replayable).
void ComputePerfIndices(std::vector<Match>& matches) {
    struct Acc { double sum = 0.0, sumsq = 0.0; int64_t n = 0; };
    std::map<std::string, Acc> acc[kNumStats];

    auto rate = [](const Participant& p, int k) {
        return p.stats[k] / (p.class_time / 60.0);
    };

    for (const auto& m : matches)
        for (const auto& p : m.players)
            for (int k = 0; k < kNumStats; k++) {
                Acc& a = acc[k][p.cls];
                const double r = rate(p, k);
                a.sum += r;
                a.sumsq += r * r;
                a.n++;
            }

    for (auto& m : matches)
        for (auto& p : m.players) {
            double zsum = 0.0;
            int zn = 0;
            for (int k = 0; k < kNumStats; k++) {
                const Acc& a = acc[k][p.cls];
                if (a.n < 2) continue;
                const double mean = a.sum / a.n;
                double var = a.sumsq / a.n - mean * mean;
                if (var < 0) var = 0;
                const double sd = std::sqrt(var);
                if (sd == 0) continue;
                double z = (rate(p, k) - mean) / sd;
                z = std::max(-kPerfZClamp, std::min(kPerfZClamp, z));
                zsum += kStatSign[k] * z;
                zn++;
            }
            p.perf = zn ? zsum / zn : 0.0;
        }
}

std::set<int64_t> LoadProcessed(sqlite3* db, const char* table) {
    std::set<int64_t> out;
    const std::string sql = std::string("SELECT instance_id FROM ") + table;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK && stmt)
        while (sqlite3_step(stmt) == SQLITE_ROW)
            out.insert(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);
    return out;
}

// rowid order == fold/insert order, so the last row per key is the current
// rating (more precise than the scripts' (played_at, instance_id) ordering
// under conclusion-ordered folding).
void LoadWlState(sqlite3* db, std::map<RatingKey, double>& rating) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT user_id, class_name, mmr_after FROM ga_wl_mmr_history "
            "ORDER BY rowid", -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* cls = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rating[{sqlite3_column_int64(stmt, 0), cls ? cls : ""}] =
                sqlite3_column_double(stmt, 2);
        }
    }
    sqlite3_finalize(stmt);
}

void LoadPerfState(sqlite3* db, std::map<RatingKey, double>& rating,
                   std::map<RatingKey, int64_t>& games) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT user_id, class_name, mmr_after, games_after FROM ga_mmr_history "
            "ORDER BY rowid", -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* cls = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const RatingKey key{sqlite3_column_int64(stmt, 0), cls ? cls : ""};
            rating[key] = sqlite3_column_double(stmt, 2);
            games[key]  = sqlite3_column_int64(stmt, 3);
        }
    }
    sqlite3_finalize(stmt);
}

std::string FoldLocked() {
    sqlite3* db = Database::GetConnection();
    if (!db) return "db unavailable";

    // Fast path: nothing new for either engine (the common per-tick case).
    {
        sqlite3_stmt* stmt = nullptr;
        int64_t pending = 0;
        if (sqlite3_prepare_v2(db,
                "SELECT COUNT(*) FROM ga_instances i "
                "WHERE i.outcome IN ('ATTACKERS_WIN', 'DEFENDERS_WIN', 'STALEMATE') "
                "  AND EXISTS (SELECT 1 FROM map_game_info m "
                "              WHERE m.map_name = i.map_name AND m.is_pvp = 1) "
                "  AND (i.id NOT IN (SELECT instance_id FROM ga_wl_mmr_processed) "
                "       OR i.id NOT IN (SELECT instance_id FROM ga_mmr_processed))",
                -1, &stmt, nullptr) == SQLITE_OK && stmt) {
            if (sqlite3_step(stmt) == SQLITE_ROW)
                pending = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        if (pending == 0) return "up to date";
    }

    std::vector<Match> matches = LoadMatches(db);
    BuildParticipants(db, matches);
    ComputePerfIndices(matches);

    std::map<RatingKey, double> wl_rating, perf_rating;
    std::map<RatingKey, int64_t> perf_games;
    LoadWlState(db, wl_rating);
    LoadPerfState(db, perf_rating, perf_games);
    std::set<int64_t> wl_done   = LoadProcessed(db, "ga_wl_mmr_processed");
    std::set<int64_t> perf_done = LoadProcessed(db, "ga_mmr_processed");

    sqlite3_stmt* ins_wl_hist = nullptr;
    sqlite3_stmt* ins_wl_proc = nullptr;
    sqlite3_stmt* ins_pf_hist = nullptr;
    sqlite3_stmt* ins_pf_proc = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO ga_wl_mmr_history VALUES (?, ?, ?, ?, ?, ?)",
        -1, &ins_wl_hist, nullptr);
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO ga_wl_mmr_processed VALUES (?, ?)",
        -1, &ins_wl_proc, nullptr);
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO ga_mmr_history VALUES (?, ?, ?, ?, ?, ?, ?)",
        -1, &ins_pf_hist, nullptr);
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO ga_mmr_processed VALUES (?, ?)",
        -1, &ins_pf_proc, nullptr);
    if (!ins_wl_hist || !ins_wl_proc || !ins_pf_hist || !ins_pf_proc) {
        sqlite3_finalize(ins_wl_hist);
        sqlite3_finalize(ins_wl_proc);
        sqlite3_finalize(ins_pf_hist);
        sqlite3_finalize(ins_pf_proc);
        return "insert prepare failed";
    }

    auto insert_proc = [](sqlite3_stmt* stmt, int64_t id, int64_t played_at) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_int64(stmt, 2, played_at);
        sqlite3_step(stmt);
    };

    sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);

    int rated = 0, skipped_one_sided = 0, gap_adjusted = 0;
    int64_t wl_rows = 0, pf_rows = 0;

    for (auto& m : matches) {
        const bool wl_new = !wl_done.count(m.id);
        const bool pf_new = !perf_done.count(m.id);
        if (!wl_new && !pf_new) continue;

        // Watermark first, even when skipped below (mirrors the scripts).
        if (wl_new) insert_proc(ins_wl_proc, m.id, m.started_at);
        if (pf_new) insert_proc(ins_pf_proc, m.id, m.started_at);

        if (m.players.empty()) continue;

        std::set<int> tfs;
        for (const auto& p : m.players) tfs.insert(p.task_force);
        if (tfs.size() != 2) {
            skipped_one_sided++;
            continue;
        }
        const int tf_a = *tfs.begin();
        const int tf_b = *std::next(tfs.begin());

        // Plain locals (not structured bindings): the lambdas below capture
        // them, which C++17 forbids for structured bindings.
        const std::pair<int, double> gap_res = GetTeamSizeGap(db, m.id);
        const int bigger_tf = gap_res.first;
        const double gap = std::min(gap_res.second, kMaxGapCap);
        const bool adjust = bigger_tf != 0
            && (bigger_tf == tf_a || bigger_tf == tf_b)
            && gap > kGapDeadzone;
        if (adjust) gap_adjusted++;

        auto team_avg = [&](const std::map<RatingKey, double>& ratings) {
            std::map<int, std::pair<double, int>> agg;
            for (const auto& p : m.players) {
                double r = kDefaultMmr;
                auto it = ratings.find({p.user_id, p.cls});
                if (it != ratings.end()) r = it->second;
                agg[p.task_force].first += r;
                agg[p.task_force].second++;
            }
            std::map<int, double> avg;
            for (const auto& e : agg) avg[e.first] = e.second.first / e.second.second;
            return avg;
        };
        auto adjusted = [&](std::map<int, double> avg) {
            if (adjust) {
                const int smaller = (bigger_tf == tf_a) ? tf_b : tf_a;
                const double half = gap * kEloPerPlayerGap / 2.0;
                avg[bigger_tf] += half;
                avg[smaller]  -= half;
            }
            return avg;
        };

        if (wl_new) {
            const auto adj = adjusted(team_avg(wl_rating));
            for (const auto& p : m.players) {
                const int opp = (p.task_force == tf_a) ? tf_b : tf_a;
                const RatingKey key{p.user_id, p.cls};
                auto it = wl_rating.find(key);
                const double before = (it != wl_rating.end()) ? it->second : kDefaultMmr;
                const double expected = ExpectedScore(before, adj.at(opp));
                // winning_tf 0 = stalemate (NULL winner) -> both sides draw.
                const double actual = (m.winning_tf == 0) ? 0.5
                    : (p.task_force == m.winning_tf) ? 1.0 : 0.0;
                const double after = before + kWlK * (actual - expected);
                wl_rating[key] = after;

                sqlite3_reset(ins_wl_hist);
                sqlite3_clear_bindings(ins_wl_hist);
                sqlite3_bind_int64(ins_wl_hist, 1, p.user_id);
                sqlite3_bind_text(ins_wl_hist, 2, p.cls.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(ins_wl_hist, 3, m.id);
                sqlite3_bind_int64(ins_wl_hist, 4, m.started_at);
                sqlite3_bind_double(ins_wl_hist, 5, before);
                sqlite3_bind_double(ins_wl_hist, 6, after);
                sqlite3_step(ins_wl_hist);
                wl_rows++;
            }
        }

        if (pf_new) {
            const auto avg = team_avg(perf_rating);
            const auto adj = adjusted(avg);
            for (const auto& p : m.players) {
                const int opp = (p.task_force == tf_a) ? tf_b : tf_a;
                const RatingKey key{p.user_id, p.cls};
                auto it = perf_rating.find(key);
                const double before = (it != perf_rating.end()) ? it->second : kDefaultMmr;
                // Team-average expected score (mmr_tracker.py semantics).
                const double expected = ExpectedScore(avg.at(p.task_force), adj.at(opp));
                // winning_tf 0 = stalemate (NULL winner) -> both sides draw.
                const double actual = (m.winning_tf == 0) ? 0.5
                    : (p.task_force == m.winning_tf) ? 1.0 : 0.0;
                const int64_t games = perf_games[key];
                const double k = (games < kPerfMinGames) ? kPerfKProvisional : kPerfKBase;
                const double after = before + k * ((actual - expected) + kPerfBeta * p.perf);
                perf_rating[key] = after;
                perf_games[key] = games + 1;

                sqlite3_reset(ins_pf_hist);
                sqlite3_clear_bindings(ins_pf_hist);
                sqlite3_bind_int64(ins_pf_hist, 1, p.user_id);
                sqlite3_bind_text(ins_pf_hist, 2, p.cls.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(ins_pf_hist, 3, m.id);
                sqlite3_bind_int64(ins_pf_hist, 4, m.started_at);
                sqlite3_bind_double(ins_pf_hist, 5, before);
                sqlite3_bind_double(ins_pf_hist, 6, after);
                sqlite3_bind_int64(ins_pf_hist, 7, games + 1);
                sqlite3_step(ins_pf_hist);
                pf_rows++;
            }
        }

        rated++;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(ins_wl_hist);
    sqlite3_finalize(ins_wl_proc);
    sqlite3_finalize(ins_pf_hist);
    sqlite3_finalize(ins_pf_proc);

    char buf[256];
    snprintf(buf, sizeof(buf),
        "folded %d match(es) (%d one-sided skipped, %d gap-adjusted); "
        "rows appended wl=%lld perf=%lld",
        rated, skipped_one_sided, gap_adjusted,
        (long long)wl_rows, (long long)pf_rows);
    Logger::Log("mmr", "[MmrService] %s\n", buf);
    return buf;
}

}  // namespace

void MmrService::FoldUnprocessed() {
    std::lock_guard<std::mutex> lock(mutex_);
    FoldLocked();
}

std::string MmrService::Reseed() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return "db unavailable";
    char* err = nullptr;
    if (sqlite3_exec(db,
            "DELETE FROM ga_wl_mmr_history; DELETE FROM ga_wl_mmr_processed; "
            "DELETE FROM ga_mmr_history; DELETE FROM ga_mmr_processed;",
            nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        return "reseed wipe failed: " + msg;
    }
    Logger::Log("mmr", "[MmrService] RESEED: cleared both engines, replaying all matches\n");
    return "reseed: " + FoldLocked();
}

std::string MmrService::GetActiveEngine() {
    sqlite3* db = Database::GetConnection();
    if (!db) return "wl";
    std::string engine = "wl";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT value FROM cs_settings WHERE key = 'active_mmr_engine'",
            -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* v = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (v && *v) engine = v;
        }
    }
    sqlite3_finalize(stmt);
    return engine;
}

double MmrService::GetCurrentRating(int64_t user_id, uint32_t profile_id) {
    const char* cls = ClassNameForProfile(static_cast<int>(profile_id));
    if (!cls) return kDefaultMmr;
    sqlite3* db = Database::GetConnection();
    if (!db) return kDefaultMmr;
    const std::string table =
        (GetActiveEngine() == "perf") ? "ga_mmr_history" : "ga_wl_mmr_history";
    // rowid order == fold order; the last row per (user, class) is current.
    const std::string sql =
        "SELECT mmr_after FROM " + table +
        " WHERE user_id = ? AND class_name = ? ORDER BY rowid DESC LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    double rating = kDefaultMmr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        sqlite3_bind_int64(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, cls, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            rating = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return rating;
}

bool MmrService::SetActiveEngine(const std::string& engine) {
    if (engine != "wl" && engine != "perf") return false;
    sqlite3* db = Database::GetConnection();
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "INSERT OR REPLACE INTO cs_settings (key, value) "
            "VALUES ('active_mmr_engine', ?)",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) return false;
    sqlite3_bind_text(stmt, 1, engine.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (ok) Logger::Log("mmr", "[MmrService] active engine set to %s\n", engine.c_str());
    return ok;
}
