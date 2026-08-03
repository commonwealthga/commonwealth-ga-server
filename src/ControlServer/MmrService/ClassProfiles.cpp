#include "src/ControlServer/MmrService/ClassProfiles.hpp"

#include "src/ControlServer/Logger.hpp"
#include "lib/nlohmann/json.hpp"

#include <fstream>
#include <initializer_list>
#include <mutex>
#include <utility>

namespace ClassProfiles {

namespace {

struct StatMeta {
    const char* name;
    const char* column;   // nullptr = derived
};

// Order must match the Stat enum.
const StatMeta kStats[kStatCount] = {
    {"kills",         "kills"},
    {"assists",       "assists"},
    {"deaths",        "deaths"},
    {"damage_dealt",  "damage_dealt"},
    {"damage_taken",  "damage_taken"},
    {"healing",       "healing"},
    {"defense",       "defense"},
    {"buff_value",    "buff_value"},
    {"obj_points",    "obj_points"},
    {"bot_kills",     "bot_kills"},
    {"rel_deaths",    nullptr},
};

std::mutex             g_mutex;
std::vector<ClassDef>  g_classes;
Tunables               g_tunables;
bool                   g_initialised = false;

Profile MakeProfile(const char* name, double premium,
                    std::initializer_list<std::pair<int, double>> weights) {
    Profile p;
    p.name = name;
    p.premium = premium;
    for (const auto& w : weights) p.weight[w.first] = w.second;
    return p;
}

// Tuned against 159 Sunday matches on 2026-08-03. Every number here is a
// judgement backed by either the operator's description of strong play or an
// out-of-sample prediction test; none of them are derived constants.
void InstallDefaults() {
    g_classes.clear();
    g_tunables = Tunables{};

    {   // Roamers trade kills and damage; tanks hold the point and soak. Both
        // are viable, so a player is scored on whichever they actually play.
        // The tank premium is the operator's call -- prediction was flat across
        // 0.80..1.35, so the data neither supports nor refutes it.
        ClassDef c;
        c.name = "Assault";
        c.profile_id = 680;
        c.profiles.push_back(MakeProfile("roamer", 1.00, {
            {kDamageDealt, 1.0}, {kKills, 1.0}, {kObjPoints, 0.4},
            {kAssists, 0.3}, {kDeaths, -0.4}}));
        c.profiles.push_back(MakeProfile("tank", 1.10, {
            {kDamageTaken, 1.0}, {kObjPoints, 1.0}, {kDefense, 0.7},
            {kDamageDealt, 0.4}, {kKills, 0.3}, {kDeaths, -0.2}}));
        c.discriminant.pos = {kDamageTaken, kObjPoints, kDefense};
        c.discriminant.neg = {kKills, kDamageDealt};
        c.discriminant.threshold = 0.0;
        g_classes.push_back(std::move(c));
    }
    {
        ClassDef c;
        c.name = "Recon";
        c.profile_id = 681;
        c.profiles.push_back(MakeProfile("recon", 1.00, {
            {kKills, 1.0}, {kDamageDealt, 0.9}, {kObjPoints, 0.4},
            {kAssists, 0.3}, {kDeaths, -0.4}}));
        g_classes.push_back(std::move(c));
    }
    {   // Healing volume leads, but damage taken and dying more than your own
        // team both count against -- they separate medics who hold position
        // from ones who get caught out. Poison medics deal damage instead of
        // healing; they are real and useful, but win fewer games than healers,
        // hence the premium well below 1.0.
        ClassDef c;
        c.name = "Medic";
        c.profile_id = 567;
        c.profiles.push_back(MakeProfile("healer", 1.00, {
            {kHealing, 1.0}, {kAssists, 0.8}, {kBuffValue, 0.4},
            {kObjPoints, 0.2}, {kDeaths, -0.2}, {kDamageTaken, -0.4},
            {kRelDeaths, -0.3}}));
        c.profiles.push_back(MakeProfile("poison", 0.45, {
            {kKills, 0.8}, {kDamageDealt, 0.8}, {kAssists, 0.5},
            {kHealing, 0.4}, {kBuffValue, 0.2}, {kObjPoints, 0.2},
            {kDeaths, -0.3}}));
        // A poison medic is identified by dealing damage INSTEAD of healing,
        // never by scoring badly as a healer.
        c.discriminant.pos = {kKills, kDamageDealt};
        c.discriminant.neg = {kHealing};
        c.discriminant.threshold = 0.5;
        g_classes.push_back(std::move(c));
    }
    {   // Robotics heal too, but cannot reach medic volume -- per-class
        // z-scoring handles that automatically.
        ClassDef c;
        c.name = "Robotic";
        c.profile_id = 679;
        c.profiles.push_back(MakeProfile("robo", 1.00, {
            {kDamageDealt, 0.8}, {kDefense, 0.8}, {kKills, 0.7},
            {kObjPoints, 0.4}, {kHealing, 0.4}, {kAssists, 0.3},
            {kDeaths, -0.3}}));
        g_classes.push_back(std::move(c));
    }
    g_initialised = true;
}

void ReadStatList(const nlohmann::json& arr, std::vector<int>& out) {
    std::vector<int> parsed;
    for (const auto& e : arr) {
        if (!e.is_string()) continue;
        const int s = StatByName(e.get<std::string>());
        if (s < 0) {
            Logger::Log("mmr", "[ClassProfiles] unknown stat '%s' ignored\n",
                        e.get<std::string>().c_str());
            continue;
        }
        parsed.push_back(s);
    }
    if (!parsed.empty()) out = std::move(parsed);
}

void ApplyOverrides(const nlohmann::json& mmr) {
    if (mmr.contains("tunables") && mmr["tunables"].is_object()) {
        const auto& t = mmr["tunables"];
        auto num = [&](const char* k, auto& dst) {
            if (t.contains(k) && t[k].is_number()) dst = t[k].get<double>();
        };
        auto integer = [&](const char* k, int& dst) {
            if (t.contains(k) && t[k].is_number_integer()) dst = t[k].get<int>();
        };
        num("beta",                g_tunables.beta);
        num("k_base",              g_tunables.k_base);
        num("k_provisional",       g_tunables.k_provisional);
        num("elo_per_player_gap",  g_tunables.elo_per_player_gap);
        num("gap_cap",             g_tunables.gap_cap);
        num("seed_weight",         g_tunables.seed_weight);
        num("default_mmr",         g_tunables.default_mmr);
        num("min_match_share",     g_tunables.min_match_share);
        num("z_clamp",             g_tunables.z_clamp);
        integer("provisional_games",  g_tunables.provisional_games);
        integer("min_seconds",        g_tunables.min_seconds);
        integer("min_rated_players",  g_tunables.min_rated_players);
        integer("min_baseline_games", g_tunables.min_baseline_games);
    }

    if (!mmr.contains("classes") || !mmr["classes"].is_object()) return;
    for (auto& cls : g_classes) {
        if (!mmr["classes"].contains(cls.name)) continue;
        const auto& jc = mmr["classes"][cls.name];
        if (!jc.is_object()) continue;

        if (jc.contains("profiles") && jc["profiles"].is_object()) {
            for (auto& prof : cls.profiles) {
                if (!jc["profiles"].contains(prof.name)) continue;
                const auto& jp = jc["profiles"][prof.name];
                if (!jp.is_object()) continue;
                if (jp.contains("premium") && jp["premium"].is_number())
                    prof.premium = jp["premium"].get<double>();
                if (jp.contains("weights") && jp["weights"].is_object()) {
                    // Named weights only -- an absent stat keeps its default
                    // rather than being zeroed, so a partial edit is safe.
                    for (auto it = jp["weights"].begin(); it != jp["weights"].end(); ++it) {
                        const int s = StatByName(it.key());
                        if (s < 0) {
                            Logger::Log("mmr",
                                "[ClassProfiles] %s/%s: unknown stat '%s' ignored\n",
                                cls.name.c_str(), prof.name.c_str(), it.key().c_str());
                            continue;
                        }
                        if (it.value().is_number()) prof.weight[s] = it.value().get<double>();
                    }
                }
            }
        }
        if (jc.contains("discriminant") && jc["discriminant"].is_object()) {
            const auto& jd = jc["discriminant"];
            if (jd.contains("pos") && jd["pos"].is_array())
                ReadStatList(jd["pos"], cls.discriminant.pos);
            if (jd.contains("neg") && jd["neg"].is_array())
                ReadStatList(jd["neg"], cls.discriminant.neg);
            if (jd.contains("threshold") && jd["threshold"].is_number())
                cls.discriminant.threshold = jd["threshold"].get<double>();
        }
    }
}

}  // namespace

const char* StatColumn(int stat) {
    if (stat < 0 || stat >= kStatCount) return nullptr;
    return kStats[stat].column;
}

const char* StatName(int stat) {
    if (stat < 0 || stat >= kStatCount) return "?";
    return kStats[stat].name;
}

int StatByName(const std::string& name) {
    for (int i = 0; i < kStatCount; ++i) {
        if (name == kStats[i].name) return i;
    }
    return -1;
}

void Load(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    InstallDefaults();

    std::ifstream in(config_path);
    if (!in) {
        Logger::Log("mmr",
            "[ClassProfiles] no config at %s -- using tuned defaults\n",
            config_path.c_str());
        return;
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        // Keep the defaults rather than failing to start. A typo in the config
        // should cost the operator their overrides, not the engine.
        Logger::Log("mmr",
            "[ClassProfiles] %s is not valid JSON (%s) -- using tuned defaults\n",
            config_path.c_str(), e.what());
        return;
    }
    if (!j.contains("mmr") || !j["mmr"].is_object()) {
        Logger::Log("mmr", "[ClassProfiles] no \"mmr\" section -- using tuned defaults\n");
        return;
    }
    try {
        ApplyOverrides(j["mmr"]);
    } catch (const std::exception& e) {
        Logger::Log("mmr",
            "[ClassProfiles] failed to apply \"mmr\" overrides (%s) -- "
            "some defaults may have been replaced\n", e.what());
        return;
    }
    Logger::Log("mmr",
        "[ClassProfiles] loaded: beta=%.2f gap=%.0f seed=%.2f "
        "min=%ds/%.0f%% of match\n",
        g_tunables.beta, g_tunables.elo_per_player_gap, g_tunables.seed_weight,
        g_tunables.min_seconds, g_tunables.min_match_share * 100.0);
    for (const auto& c : g_classes) {
        for (const auto& p : c.profiles) {
            Logger::Log("mmr", "[ClassProfiles]   %s/%s premium=%.2f\n",
                        c.name.c_str(), p.name.c_str(), p.premium);
        }
    }
}

const std::vector<ClassDef>& Classes() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_initialised) InstallDefaults();
    return g_classes;
}

const ClassDef* ByProfileId(uint32_t profile_id) {
    const auto& classes = Classes();
    for (const auto& c : classes) {
        if (c.profile_id == profile_id) return &c;
    }
    return nullptr;
}

const Tunables& Get() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_initialised) InstallDefaults();
    return g_tunables;
}

}  // namespace ClassProfiles
