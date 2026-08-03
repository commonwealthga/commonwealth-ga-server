#include "src/ControlServer/MmrService/MmrService.hpp"
#include "src/ControlServer/MmrService/ClassProfiles.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <array>
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

namespace CP = ClassProfiles;
constexpr int kNumStats = CP::kStatCount;

// W/L engine. Left on its original constants and kept working as a fallback
// behind cs_settings.active_mmr_engine — it is not the engine we want, but it
// is a known quantity to fall back to if the perf engine misbehaves live.
constexpr double kWlK = 32.0;

using RatingKey = std::pair<int64_t, std::string>;  // (user_id, class_name)

struct Stint {
    std::string cls;
    uint32_t    profile_id = 0;
    int         task_force = 0;
    double      time = 0.0;              // seconds
    double      raw[kNumStats] = {};     // DB-backed slots only
};

struct Participant {
    int64_t     user_id = 0;
    int         task_force = 0;
    std::string cls;
    uint32_t    profile_id = 0;
    double      class_time = 0.0;        // seconds on the finishing class
    double      match_share = 1.0;       // class_time / match length
    double      rate[kNumStats] = {};    // per-minute, incl. derived rel_deaths
    double      perf = 0.0;              // filled during the chronological walk
    double      expected = 0.0;
    double      actual = 0.0;
    std::string archetype;
};

struct Match {
    int64_t id = 0;
    int64_t started_at = 0;
    int     winning_tf = 0;
    double  length = 0.0;                // seconds, MAX(game_time)
    std::vector<Participant> players;
};

// Running mean/variance for one stat within one class.
struct Accum {
    double  sum = 0.0;
    double  sumsq = 0.0;
    int64_t n = 0;

    void Add(double v) { sum += v; sumsq += v * v; n++; }
};
using ClassBaseline = std::array<Accum, kNumStats>;

// A player's running per-stat z-scores on one class. Archetype rules are
// evaluated against these means rather than a single match, so one unusual
// game cannot relabel someone.
struct ArchSignal {
    double  zsum[kNumStats] = {};
    int64_t zn[kNumStats] = {};
};

// Divisor is a claim about how decisive a rating gap is in THIS game, not a
// universal constant — see Tunables::elo_divisor. The wl engine keeps chess's
// 400 so its (retired) history stays on its original scale.
constexpr double kWlDivisor = 400.0;

double ExpectedScore(double rating, double opp_avg, double divisor) {
    return 1.0 / (1.0 + std::pow(10.0, (opp_avg - rating) / divisor));
}

// z of `value` against the class population, or false when the stat has too
// little history or no spread to say anything. A stat that does not vary
// within a class (healing for Assault) drops out rather than contributing a
// meaningless zero.
bool ZScore(const ClassBaseline& base, int stat, double value,
            double clamp, int64_t min_games, double* out) {
    const Accum& a = base[stat];
    if (a.n < min_games) return false;
    const double mean = a.sum / static_cast<double>(a.n);
    double var = a.sumsq / static_cast<double>(a.n) - mean * mean;
    if (var < 0.0) var = 0.0;
    const double sd = std::sqrt(var);
    if (sd == 0.0) return false;
    double z = (value - mean) / sd;
    if (z >  clamp) z =  clamp;
    if (z < -clamp) z = -clamp;
    *out = z;
    return true;
}

// Score one participant against the class population as it stood BEFORE this
// match, and label them with the archetype they play.
void ScoreParticipant(Participant& p, const ClassBaseline& base,
                      const CP::ClassDef& def, ArchSignal& signal,
                      const CP::Tunables& tun) {
    double z[kNumStats] = {};
    bool   have[kNumStats] = {};
    for (int s = 0; s < kNumStats; ++s) {
        have[s] = ZScore(base, s, p.rate[s], tun.z_clamp, tun.min_baseline_games, &z[s]);
        if (have[s]) {
            signal.zsum[s] += z[s];
            signal.zn[s]++;
        }
    }

    // Rule conditions read the player's whole history on the class, this match
    // included — a healer having one aggressive game stays a healer.
    auto mean_of = [&signal](int s, double* out) -> bool {
        if (signal.zn[s] == 0) return false;
        *out = signal.zsum[s] / static_cast<double>(signal.zn[s]);
        return true;
    };
    auto group_of = [&mean_of](const std::vector<int>& stats, double* out) -> bool {
        double total = 0.0;
        int count = 0;
        for (int s : stats) {
            double m = 0.0;
            if (mean_of(s, &m)) { total += m; count++; }
        }
        if (count == 0) return false;
        *out = total / count;
        return true;
    };

    const CP::Profile* profile = &def.profiles.front();
    for (const auto& rule : def.rules) {
        const CP::Profile* cand = def.ByName(rule.profile);
        if (!cand) continue;
        // An empty side contributes 0 — the class average — so a rule may use
        // either comparison, floors, or both.
        double pos = 0.0, neg = 0.0;
        if (!rule.pos.empty() && !group_of(rule.pos, &pos)) continue;
        if (!rule.neg.empty() && !group_of(rule.neg, &neg)) continue;
        if (pos - neg <= rule.threshold) continue;
        bool floors_met = true;
        for (const auto& req : rule.require) {
            double m = 0.0;
            if (!mean_of(req.first, &m) || m < req.second) { floors_met = false; break; }
        }
        if (!floors_met) continue;
        profile = cand;
        break;
    }
    p.archetype = profile->name;

    double num = 0.0, den = 0.0;
    for (int s = 0; s < kNumStats; ++s) {
        const double w = profile->weight[s];
        if (w == 0.0 || !have[s]) continue;
        num += w * z[s];
        den += std::fabs(w);
    }
    p.perf = (den > 0.0) ? profile->premium * (num / den) : 0.0;
}

void AccumulateBaseline(ClassBaseline& base, const Participant& p) {
    for (int s = 0; s < kNumStats; ++s) base[s].Add(p.rate[s]);
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
    // TEAM_CHANGE carries its destination team in `detail`, not in
    // target_task_force (that column is NULL on every row). Reading the wrong
    // one coalesced to 0, dropping the mover from BOTH headcounts for the rest
    // of the match.
    if (sqlite3_prepare_v2(db,
            "SELECT event_type, game_time, COALESCE(actor_user_id, 0), "
            "       COALESCE(actor_task_force, 0), "
            "       COALESCE(target_task_force, detail, 0) "
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
            "                i.started_at) AS concluded_at, "
            "       COALESCE((SELECT MAX(e.game_time) FROM ga_match_events e "
            "                 WHERE e.instance_id = i.id), 0.0) AS match_len "
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
        m.length     = sqlite3_column_double(stmt, 4);
        matches.push_back(std::move(m));
    }
    sqlite3_finalize(stmt);
    return matches;
}

// Per-(instance, task force) totals, used to derive rel_deaths: a player's
// death rate measured against their TEAMMATES' death rate, so someone who dies
// often on a team that dies often is not punished for the team's problem.
struct TeamTotals {
    double deaths = 0.0;
    double time = 0.0;
};

std::map<std::pair<int64_t, int>, TeamTotals> LoadTeamTotals(sqlite3* db) {
    std::map<std::pair<int64_t, int>, TeamTotals> out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT instance_id, task_force, SUM(deaths), SUM(time_played_seconds) "
            "FROM ga_match_player_stats GROUP BY instance_id, task_force",
            -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TeamTotals t;
            t.deaths = sqlite3_column_double(stmt, 2);
            t.time   = sqlite3_column_double(stmt, 3);
            out[{sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1)}] = t;
        }
    }
    sqlite3_finalize(stmt);
    return out;
}

// Participation rule: roster de-duped to one class per character, non-zero
// activity, finishing-class credit (longest stint), then BOTH a minimum time
// on that class AND a minimum share of the match. The share test is what keeps
// a 3-minute cameo in a 25-minute match from moving anybody's rating — such a
// stint says almost nothing about who was going to win.
void BuildParticipants(sqlite3* db, std::vector<Match>& matches) {
    std::map<int64_t, Match*> by_id;
    for (auto& m : matches) by_id[m.id] = &m;

    const CP::Tunables& tun = CP::Get();
    const auto team_totals = LoadTeamTotals(db);

    // Column list is driven by the profile stat table so adding a stat means
    // touching ClassProfiles only.
    std::string cols;
    std::vector<int> col_slot;
    for (int s = 0; s < kNumStats; ++s) {
        const char* c = CP::StatColumn(s);
        if (!c) continue;                       // derived, not selected
        cols += ", s.";
        cols += c;
        col_slot.push_back(s);
    }
    const std::string sql =
        "SELECT s.instance_id, s.user_id, s.task_force, r.profile_id, "
        "       s.time_played_seconds" + cols + " "
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
        "              WHERE m.map_name = i.map_name AND m.is_pvp = 1)";

    std::map<std::pair<int64_t, int64_t>, std::vector<Stint>> stints;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("mmr", "[MmrService] participant query prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int64_t inst = sqlite3_column_int64(stmt, 0);
        if (!by_id.count(inst)) continue;
        const uint32_t pid = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
        const CP::ClassDef* def = CP::ByProfileId(pid);
        if (!def) continue;
        Stint st;
        st.cls        = def->name;
        st.profile_id = pid;
        st.task_force = sqlite3_column_int(stmt, 2);
        st.time       = sqlite3_column_double(stmt, 4);
        for (size_t k = 0; k < col_slot.size(); ++k) {
            st.raw[col_slot[k]] = sqlite3_column_double(stmt, 5 + static_cast<int>(k));
        }
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
        p.profile_id = fin.profile_id;
        p.task_force = fin.task_force;
        double summed[kNumStats] = {};
        for (const auto& s : grp) {
            if (s.cls != p.cls) continue;
            p.class_time += s.time;
            for (int k = 0; k < kNumStats; ++k) summed[k] += s.raw[k];
        }
        if (p.class_time < tun.min_seconds) continue;

        Match* m = by_id[entry.first.first];
        p.match_share = (m->length > 0.0)
            ? std::min(1.0, p.class_time / m->length) : 1.0;
        if (p.match_share < tun.min_match_share) continue;

        const double minutes = p.class_time / 60.0;
        for (int k = 0; k < kNumStats; ++k) p.rate[k] = summed[k] / minutes;

        // rel_deaths: own death rate over the rest of the team's death rate.
        // 1.0 when the team's rate is unknown — neutral, neither credit nor
        // penalty.
        p.rate[CP::kRelDeaths] = 1.0;
        auto tt = team_totals.find({m->id, p.task_force});
        if (tt != team_totals.end()) {
            const double others_min = (tt->second.time - p.class_time) / 60.0;
            const double others_deaths = tt->second.deaths - summed[CP::kDeaths];
            if (others_min > 1.0 && others_deaths > 0.0) {
                const double others_rate = others_deaths / others_min;
                p.rate[CP::kRelDeaths] = p.rate[CP::kDeaths] / others_rate;
            }
        }
        m->players.push_back(std::move(p));
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
            "ORDER BY instance_id", -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* cls = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const RatingKey key{sqlite3_column_int64(stmt, 0), cls ? cls : ""};
            rating[key] = sqlite3_column_double(stmt, 2);
            games[key]  = sqlite3_column_int64(stmt, 3);
        }
    }
    sqlite3_finalize(stmt);
}

// A rating for a class the player has not played before. Starting everyone at
// the default throws away what we already know: general skill carries across
// classes more than it doesn't, so a strong Assault is a better-than-average
// bet on Medic. seed_weight controls how much of that carries — below 1.0 so
// there is still room to be genuinely bad at a class.
double SeedRating(const std::map<RatingKey, double>& rating,
                  const std::map<RatingKey, int64_t>& games,
                  int64_t user_id, const CP::Tunables& tun) {
    double sum = 0.0;
    int n = 0;
    for (const auto& e : rating) {
        if (e.first.first != user_id) continue;
        auto g = games.find(e.first);
        if (g == games.end() || g->second < tun.provisional_games) continue;
        sum += e.second;
        n++;
    }
    if (n == 0) return tun.default_mmr;
    return tun.default_mmr + tun.seed_weight * (sum / n - tun.default_mmr);
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

    const CP::Tunables& tun = CP::Get();

    std::vector<Match> matches = LoadMatches(db);
    BuildParticipants(db, matches);

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
        "INSERT OR IGNORE INTO ga_mmr_history "
        "(user_id, class_name, archetype, instance_id, played_at, minutes, "
        " match_share, perf, expected, actual, mmr_before, mmr_after, games_after) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
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

    // Class populations and per-player archetype signals are rebuilt from the
    // whole match history on every fold, always in conclusion order. They are
    // never read ahead of the match being scored, so a match scored live and
    // the same match scored during a full replay get identical numbers — that
    // is what makes a reseed reproduce the live chain.
    std::map<std::string, ClassBaseline> baselines;
    std::map<RatingKey, ArchSignal> arch;

    int rated = 0, skipped_one_sided = 0, skipped_small = 0, gap_adjusted = 0;
    int64_t wl_rows = 0, pf_rows = 0;

    for (auto& m : matches) {
        const bool wl_new = !wl_done.count(m.id);
        const bool pf_new = !perf_done.count(m.id);

        // Near-empty lobbies are watermarked so they are never revisited, but
        // otherwise ignored completely — they neither move ratings nor feed
        // the class baselines, because a 4-player match says nothing about how
        // a class is normally played.
        if (static_cast<int>(m.players.size()) < tun.min_rated_players) {
            if (wl_new) insert_proc(ins_wl_proc, m.id, m.started_at);
            if (pf_new) insert_proc(ins_pf_proc, m.id, m.started_at);
            if (wl_new || pf_new) skipped_small++;
            continue;
        }

        // Score against the population as it stood before this match, whether
        // or not we are writing it — the baselines have to advance in lockstep
        // with the walk even across already-folded matches.
        for (auto& p : m.players) {
            const CP::ClassDef* def = CP::ByProfileId(p.profile_id);
            if (!def) continue;
            ScoreParticipant(p, baselines[p.cls], *def, arch[{p.user_id, p.cls}], tun);
        }

        auto advance_baselines = [&]() {
            for (const auto& p : m.players) AccumulateBaseline(baselines[p.cls], p);
        };

        if (!wl_new && !pf_new) { advance_baselines(); continue; }

        // Watermark first, even when skipped below (mirrors the scripts).
        if (wl_new) insert_proc(ins_wl_proc, m.id, m.started_at);
        if (pf_new) insert_proc(ins_pf_proc, m.id, m.started_at);

        if (m.players.empty()) { advance_baselines(); continue; }

        std::set<int> tfs;
        for (const auto& p : m.players) tfs.insert(p.task_force);
        if (tfs.size() != 2) {
            skipped_one_sided++;
            advance_baselines();
            continue;
        }
        const int tf_a = *tfs.begin();
        const int tf_b = *std::next(tfs.begin());

        const std::pair<int, double> gap_res = GetTeamSizeGap(db, m.id);
        const int bigger_tf = gap_res.first;
        const double gap = std::min(gap_res.second, tun.gap_cap);
        const bool adjust = bigger_tf != 0
            && (bigger_tf == tf_a || bigger_tf == tf_b)
            && gap > 0.0;
        if (adjust) gap_adjusted++;

        auto team_avg = [&](const std::map<RatingKey, double>& ratings) {
            std::map<int, std::pair<double, int>> agg;
            for (const auto& p : m.players) {
                double r = tun.default_mmr;
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
                const double half = gap * tun.elo_per_player_gap / 2.0;
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
                const double before = (it != wl_rating.end()) ? it->second : tun.default_mmr;
                const double expected = ExpectedScore(before, adj.at(opp), kWlDivisor);
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
            const auto adj = adjusted(team_avg(perf_rating));
            for (auto& p : m.players) {
                const int opp = (p.task_force == tf_a) ? tf_b : tf_a;
                const RatingKey key{p.user_id, p.cls};
                auto it = perf_rating.find(key);
                const double before = (it != perf_rating.end())
                    ? it->second
                    : SeedRating(perf_rating, perf_games, p.user_id, tun);
                // Whether the match was won is a TEAM fact, so it is scored
                // team against team and every player shares the result. Scoring
                // an individual's rating against the opposing team instead
                // treats one player as if they decided a 9-a-side match: at
                // this divisor it expected the top Assault to win 98% of his
                // games, then punished him for winning 60%.
                //
                // What stops the rating drifting is not this term but the
                // anchored performance term below, which is per-player.
                const double expected =
                    ExpectedScore(adj.at(p.task_force), adj.at(opp), tun.elo_divisor);
                const double actual = (m.winning_tf == 0) ? 0.5
                    : (p.task_force == m.winning_tf) ? 1.0 : 0.0;
                const int64_t games = perf_games[key];
                const double k = (games < tun.provisional_games)
                    ? tun.k_provisional : tun.k_base;
                // Both halves of the update are now self-correcting. The
                // win/loss half asks "did you do better than your rating
                // predicted?"; the performance half asks the same question of
                // your stats. Scoring raw perf instead would be a constant
                // push with nothing pulling back, and the rating would climb
                // for as long as you kept playing.
                const double expected_perf =
                    (before - tun.default_mmr) / tun.perf_scale;
                const double after = before + k * ((actual - expected)
                    + tun.beta * (p.perf - expected_perf));
                perf_rating[key] = after;
                perf_games[key] = games + 1;
                p.expected = expected;
                p.actual = actual;

                sqlite3_reset(ins_pf_hist);
                sqlite3_clear_bindings(ins_pf_hist);
                sqlite3_bind_int64 (ins_pf_hist, 1,  p.user_id);
                sqlite3_bind_text  (ins_pf_hist, 2,  p.cls.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text  (ins_pf_hist, 3,  p.archetype.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64 (ins_pf_hist, 4,  m.id);
                sqlite3_bind_int64 (ins_pf_hist, 5,  m.started_at);
                sqlite3_bind_double(ins_pf_hist, 6,  p.class_time / 60.0);
                sqlite3_bind_double(ins_pf_hist, 7,  p.match_share);
                sqlite3_bind_double(ins_pf_hist, 8,  p.perf);
                sqlite3_bind_double(ins_pf_hist, 9,  expected);
                sqlite3_bind_double(ins_pf_hist, 10, actual);
                sqlite3_bind_double(ins_pf_hist, 11, before);
                sqlite3_bind_double(ins_pf_hist, 12, after);
                sqlite3_bind_int64 (ins_pf_hist, 13, games + 1);
                sqlite3_step(ins_pf_hist);
                pf_rows++;
            }
        }

        rated++;
        advance_baselines();
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(ins_wl_hist);
    sqlite3_finalize(ins_wl_proc);
    sqlite3_finalize(ins_pf_hist);
    sqlite3_finalize(ins_pf_proc);

    char buf[256];
    snprintf(buf, sizeof(buf),
        "folded %d match(es) (%d too small, %d one-sided, %d gap-adjusted); "
        "rows appended wl=%lld perf=%lld",
        rated, skipped_small, skipped_one_sided, gap_adjusted,
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
    const ClassProfiles::ClassDef* def = ClassProfiles::ByProfileId(profile_id);
    const double default_mmr = ClassProfiles::Get().default_mmr;
    if (!def) return default_mmr;
    sqlite3* db = Database::GetConnection();
    if (!db) return default_mmr;
    const bool perf = (GetActiveEngine() == "perf");
    // Both histories are keyed (user_id, class_name, instance_id), so ordering
    // by instance_id walks the primary key backwards instead of scanning.
    const std::string sql = perf
        ? "SELECT mmr_after FROM ga_mmr_history "
          "WHERE user_id = ? AND class_name = ? ORDER BY instance_id DESC LIMIT 1"
        : "SELECT mmr_after FROM ga_wl_mmr_history "
          "WHERE user_id = ? AND class_name = ? ORDER BY rowid DESC LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    double rating = default_mmr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        sqlite3_bind_int64(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, def->name.c_str(), -1, SQLITE_STATIC);
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
