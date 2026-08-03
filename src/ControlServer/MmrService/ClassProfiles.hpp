#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ClassProfiles.hpp -- per-class performance indices for the perf MMR engine.
//
// A player's per-match performance is a weighted average of z-scores taken
// against the population of the class they played, so a medic is measured
// against medics and never against assaults.
//
// Two classes have more than one way to play them well -- Assault (roamer vs
// point-holding tank) and Medic (healer vs poison) -- so each class carries one
// or more Profiles plus a Discriminant that decides which one a given player is
// scored against.
//
// Every weight here is a tuned value, not a derived constant. They are
// overridable from control-server.json so they can be adjusted between play
// sessions without a rebuild -- see Load().
namespace ClassProfiles {

// Stat slots. The first ten map 1:1 onto ga_match_player_stats columns; they
// are summed per (match, player, class) and converted to per-minute rates
// before scoring. kRelDeaths is derived: the player's death rate divided by
// their TEAMMATES' death rate, so someone who dies a lot on a team that dies a
// lot is not punished for it.
enum Stat {
    kKills = 0,
    kAssists,
    kDeaths,
    kDamageDealt,
    kDamageTaken,
    kHealing,
    kDefense,
    kBuffValue,
    kObjPoints,
    kBotKills,
    kRelDeaths,
    kStatCount
};

// ga_match_player_stats column, or nullptr when the stat is derived.
const char* StatColumn(int stat);
// Name used in config and logs; covers derived stats too.
const char* StatName(int stat);
// -1 when the name is not recognised.
int StatByName(const std::string& name);

struct Profile {
    std::string name;                     // "roamer", "tank", "healer", "poison"
    double      premium = 1.0;            // scales the finished score
    double      weight[kStatCount] = {};  // 0 = stat unused by this profile
};

// Picks which profile a player is scored against, from their accumulated
// per-stat z-scores on that class.
//
// A rule fires when BOTH hold:
//   - mean z of `pos` minus mean z of `neg` exceeds `threshold`
//     (either list may be empty; an empty side contributes 0, which is the
//      class average, so `pos` alone means "well above average at these")
//   - every (stat, min_z) in `require` is met
//
// Rules are tried in order and the first to fire wins; no match leaves the
// player on profiles[0]. Two conditions are needed because the archetypes are
// not all separable the same way: "poison" is a medic who deals damage INSTEAD
// of healing (a comparison), while "buff" is one who heals a lot AND buffs a
// lot (two floors) -- expressing the latter as a comparison would catch medics
// who simply buff more than they heal, which is a different player.
//
// Assignment is per PLAYER, not per match. Scoring someone on whichever
// profile happened to flatter them in a given game inflates the whole
// population, and it mislabels people -- a medic with weak survivability would
// get pushed onto "poison" despite dealing no damage.
struct Rule {
    std::string      profile;                        // profile name it selects
    std::vector<int> pos;
    std::vector<int> neg;
    double           threshold = 0.0;
    std::vector<std::pair<int, double>> require;     // stat -> minimum z
};

struct ClassDef {
    std::string          name;            // "Assault"
    uint32_t             profile_id = 0;  // engine PROFILE_* id
    std::vector<Profile> profiles;        // [0] is the default
    std::vector<Rule>    rules;           // tried in order
    const Profile* ByName(const std::string& n) const {
        for (const auto& p : profiles) if (p.name == n) return &p;
        return nullptr;
    }
};

struct Tunables {
    double beta               = 1.0;      // weight on the performance term
    // Rating points one unit of performance is worth at rest. A player is
    // scored on (perf - perf_expected_for_their_rating), where the expectation
    // is (rating - default) / perf_scale -- so you only gain by beating your
    // own rating, not by being good in absolute terms.
    //
    // Without this the performance term is a constant upward push with nothing
    // pulling back: the win/loss half can only ever pull as hard as
    // (win_rate - 1), so any player with beta*perf above that climbs forever.
    // At beta 1.0 that meant everyone winning more than ~44% drifted without
    // limit -- a +0.56 perf player passed 5000 by their thousandth game.
    //
    // This term is also the ONLY per-player feedback in the update: the
    // win/loss half is scored team against team and shared by everyone on a
    // side, because who won is a team fact. Convergence rests entirely here.
    double perf_scale         = 600.0;
    // Rating difference that corresponds to a given win probability. 400 is the
    // chess convention and is far too flat for this game: teams it called 62%
    // favourites won 86% of the time. Calibrated against 128 out-of-sample
    // matches. Re-fit this when the population or the balancer changes.
    double elo_divisor        = 120.0;
    double k_base             = 24.0;
    double k_provisional      = 48.0;
    int    provisional_games  = 5;
    double elo_per_player_gap = 110.0;    // headcount advantage, per player
    double gap_cap            = 3.0;      // ... capped at this many players
    double seed_weight        = 0.6;      // new class rating from other classes
    double default_mmr        = 1000.0;
    int    min_seconds        = 180;      // minimum time played on the class
    double min_match_share    = 0.50;     // ... and minimum share of the match
    // Matches with fewer rated players than this are ignored entirely: they
    // neither move ratings nor feed the class baselines. Near-empty lobbies
    // are not representative of how a class plays, and on this server they are
    // almost all midweek — every match with 14+ rated players was a Sunday.
    int    min_rated_players  = 10;
    double z_clamp            = 3.0;
    int    min_baseline_games = 20;       // before a stat's z-score is trusted
};

// Applies the "mmr" object from `config_path` on top of the compiled defaults;
// absent keys keep their default. Safe to call more than once. A malformed
// file is logged and leaves the defaults untouched, so a bad edit degrades to
// the tuned values rather than taking the engine down.
void Load(const std::string& config_path);

const std::vector<ClassDef>& Classes();
const ClassDef*              ByProfileId(uint32_t profile_id);
const Tunables&              Get();

}  // namespace ClassProfiles
