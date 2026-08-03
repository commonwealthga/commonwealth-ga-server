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

// Picks which profile a player is scored against. Compares the mean z of the
// `pos` stats against the mean z of the `neg` stats, accumulated over that
// player's history on the class; above `threshold` selects profiles[1],
// otherwise profiles[0].
//
// Assignment is per PLAYER, not per match. Scoring someone on whichever
// profile happened to flatter them in a given game inflates the whole
// population, and it also mislabels players -- a medic with weak survivability
// would get pushed onto "poison" despite dealing no damage.
struct Discriminant {
    std::vector<int> pos;
    std::vector<int> neg;
    double           threshold = 0.0;
    bool active() const { return !pos.empty() && !neg.empty(); }
};

struct ClassDef {
    std::string          name;            // "Assault"
    uint32_t             profile_id = 0;  // engine PROFILE_* id
    std::vector<Profile> profiles;        // [0] is the default
    Discriminant         discriminant;
};

struct Tunables {
    double beta               = 1.0;      // weight on the performance term
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
