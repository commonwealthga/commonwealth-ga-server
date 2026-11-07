#include <algorithm>
#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>

// ControlServerConfig.cpp -- JSON config file loader.
// All fields are optional in the JSON; missing fields retain their defaults.

namespace {
// Replace any `,` that is followed (after whitespace/comments) by `]` or `}` with a space.
// String- and comment-aware so commas inside "..." or // ... or /* ... */ are left alone.
// Spaces are used instead of erasure to preserve byte offsets for parser error messages.
void StripTrailingCommas(std::string& s) {
    size_t pending_comma = std::string::npos;
    bool in_string = false;
    bool in_line_comment = false;
    bool in_block_comment = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (in_line_comment) {
            if (c == '\n') in_line_comment = false;
            continue;
        }
        if (in_block_comment) {
            if (c == '*' && i + 1 < s.size() && s[i + 1] == '/') {
                in_block_comment = false;
                ++i;
            }
            continue;
        }
        if (in_string) {
            if (c == '\\' && i + 1 < s.size()) { ++i; continue; }
            if (c == '"') in_string = false;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        if (c == '/' && i + 1 < s.size()) {
            if (s[i + 1] == '/') { in_line_comment = true; ++i; continue; }
            if (s[i + 1] == '*') { in_block_comment = true; ++i; continue; }
        }
        if (c == '"') { in_string = true; pending_comma = std::string::npos; continue; }
        if (c == ',') { pending_comma = i; continue; }
        if (c == ']' || c == '}') {
            if (pending_comma != std::string::npos) s[pending_comma] = ' ';
        }
        pending_comma = std::string::npos;
    }
}

bool EnvFlagEnabled(const char* value) {
    if (!value) return false;
    const std::string v(value);
    return v == "1" || v == "true" || v == "TRUE" ||
           v == "yes" || v == "YES" || v == "on" || v == "ON";
}
} // namespace

ControlServerConfig ControlServerConfig::Load(const std::string& path) {
    ControlServerConfig cfg;

    std::ifstream f(path);
    if (!f.is_open()) {
        Logger::Log("config", "[ControlServerConfig] Config file not found at '%s', using defaults\n",
            path.c_str());
        return cfg;
    }

    std::stringstream buf;
    buf << f.rdbuf();
    std::string raw = buf.str();
    StripTrailingCommas(raw);

    nlohmann::json j = nlohmann::json::parse(raw, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
    if (j.is_discarded()) {
        Logger::Log("config", "[ControlServerConfig] Config file '%s' is invalid JSON, using defaults\n",
            path.c_str());
        return cfg;
    }

    if (j.contains("wine_binary"))        cfg.wine_binary        = j["wine_binary"].get<std::string>();
    if (j.contains("wine_prefix"))        cfg.wine_prefix        = j["wine_prefix"].get<std::string>();
    if (j.contains("xvfb_run_path"))      cfg.xvfb_run_path      = j["xvfb_run_path"].get<std::string>();
    if (j.contains("game_binary"))        cfg.game_binary        = j["game_binary"].get<std::string>();
    if (j.contains("host"))               cfg.host               = j["host"].get<std::string>();
    if (j.contains("hostdns"))            cfg.hostdns            = j["hostdns"].get<std::string>();
    if (j.contains("public_host"))        cfg.public_host        = j["public_host"].get<std::string>();
    if (j.contains("dll_overrides"))      cfg.dll_overrides      = j["dll_overrides"].get<std::string>();
    if (j.contains("home_map_name"))      cfg.home_map_name      = j["home_map_name"].get<std::string>();
    if (j.contains("home_map_game_mode")) cfg.home_map_game_mode = j["home_map_game_mode"].get<std::string>();
    if (j.contains("tcp_port"))           cfg.tcp_port           = j["tcp_port"].get<uint16_t>();
    if (j.contains("chat_port"))          cfg.chat_port          = j["chat_port"].get<uint16_t>();
    if (j.contains("ipc_port"))           cfg.ipc_port           = j["ipc_port"].get<uint16_t>();
    if (j.contains("admin_token"))        cfg.admin_token        = j["admin_token"].get<std::string>();
    if (j.contains("dev_controls"))       cfg.dev_controls       = j["dev_controls"].get<bool>();
    if (j.contains("starter_kit_only"))   cfg.starter_kit_only   = j["starter_kit_only"].get<bool>();
    if (j.contains("starter_kit_max_level")) cfg.starter_kit_max_level = j["starter_kit_max_level"].get<int>();
    if (j.contains("overlay_http_port"))  cfg.overlay_http_port  = j["overlay_http_port"].get<uint16_t>();
    if (j.contains("overlay_token"))      cfg.overlay_token      = j["overlay_token"].get<std::string>();
    if (j.contains("overlay_bind"))       cfg.overlay_bind       = j["overlay_bind"].get<std::string>();
    if (j.contains("startup_timeout_seconds"))
        cfg.startup_timeout_seconds = j["startup_timeout_seconds"].get<int>();
    if (j.contains("db_path"))            cfg.db_path            = j["db_path"].get<std::string>();
    if (j.contains("crash_dir"))          cfg.crash_dir          = j["crash_dir"].get<std::string>();
    if (j.contains("log_dir"))            cfg.log_dir            = j["log_dir"].get<std::string>();
    if (j.contains("fix_package_guids"))  cfg.fix_package_guids  = j["fix_package_guids"].get<bool>();
    if (j.contains("clear_logs"))         cfg.clear_logs         = j["clear_logs"].get<bool>();
    if (j.contains("show_game_console"))  cfg.show_game_console  = j["show_game_console"].get<bool>();
    if (j.contains("allow_duplicate_account_logins"))
        cfg.allow_duplicate_account_logins = j["allow_duplicate_account_logins"].get<bool>();
    if (j.contains("device_stats_enabled"))
        cfg.device_stats_enabled = j["device_stats_enabled"].get<bool>();
    if (j.contains("effectiveness_enabled"))
        cfg.effectiveness_enabled = j["effectiveness_enabled"].get<bool>();
    if (j.contains("require_provisioned_accounts"))
        cfg.require_provisioned_accounts = j["require_provisioned_accounts"].get<bool>();
    if (j.contains("require_password_verification"))
        cfg.require_password_verification = j["require_password_verification"].get<bool>();
    if (j.contains("intro_mission") && j["intro_mission"].is_object()) {
        const auto& im = j["intro_mission"];
        if (im.contains("enabled"))   cfg.intro_mission_enabled   = im["enabled"].get<bool>();
        if (im.contains("map_name"))  cfg.intro_mission_map_name  = im["map_name"].get<std::string>();
        if (im.contains("game_mode")) cfg.intro_mission_game_mode = im["game_mode"].get<std::string>();
        // Tutorial chapters
        if (im.contains("chain") && im["chain"].is_array())
            for (const auto& m : im["chain"]) if (m.is_string()) cfg.intro_chain_maps.push_back(m.get<std::string>());
        if (im.contains("chain_user_ids") && im["chain_user_ids"].is_array())
            for (const auto& u : im["chain_user_ids"]) if (u.is_number_integer()) cfg.intro_chain_user_ids.push_back(u.get<int64_t>());
    }

    if (j.contains("home_arrival_min_level"))
        cfg.home_arrival_min_level = j["home_arrival_min_level"].get<uint32_t>();
    if (j.contains("token_limits") && j["token_limits"].is_object()) {
        const auto& tl = j["token_limits"];
        if (tl.contains("weekly_earn")) cfg.token_weekly_earn = tl["weekly_earn"].get<int64_t>();
        if (tl.contains("bank"))        cfg.token_bank        = tl["bank"].get<int64_t>();
    }

    if (j.contains("quest_reward_choice_tables") && j["quest_reward_choice_tables"].is_array()) {
        for (const auto& t : j["quest_reward_choice_tables"])
            if (t.is_number_integer()) cfg.quest_reward_choice_tables.push_back(t.get<int>());
    }

    if (j.contains("enabled_channels") && j["enabled_channels"].is_array()) {
        for (const auto& item : j["enabled_channels"]) {
            if (item.is_string()) cfg.enabled_channels.push_back(item.get<std::string>());
        }
    }
    if (j.contains("login_notice"))       cfg.login_notice       = j["login_notice"].get<std::string>();
    if (j.contains("news_file"))          cfg.news_file          = j["news_file"].get<std::string>();
    if (j.contains("max_online_players")) cfg.max_online_players = j["max_online_players"].get<int>();
    if (j.contains("max_instances"))      cfg.max_instances      = j["max_instances"].get<int>();
    if (j.contains("dome_cap"))           cfg.dome_cap           = j["dome_cap"].get<int>();
    if (j.contains("zone_cap"))           cfg.zone_cap           = j["zone_cap"].get<int>();
    if (j.contains("calendar") && j["calendar"].is_object()) {
        for (auto it = j["calendar"].begin(); it != j["calendar"].end(); ++it) {
            if (it.key().empty() || it.key()[0] == '_' || !it.value().is_object()) continue;
            const auto& o = it.value();
            EventCalendar::Window w;
            if (o.contains("start") && o["start"].is_string()) w.start_mmdd = EventCalendar::ParseMmdd(o["start"].get<std::string>());
            if (o.contains("end") && o["end"].is_string())     w.end_mmdd   = EventCalendar::ParseMmdd(o["end"].get<std::string>());
            if (o.contains("win_loot_table") && o["win_loot_table"].is_number_integer()) w.win_loot_table = o["win_loot_table"].get<int>();
            if (o.contains("force") && o["force"].is_string()) {
                const std::string f = o["force"].get<std::string>();
                w.force = f == "on" ? 1 : f == "off" ? -1 : 0;
            }
            cfg.calendar[it.key()] = w;
        }
    }
    if (j.contains("external_ready_timeout_s")) cfg.external_ready_timeout_s = j["external_ready_timeout_s"].get<int>();
    if (j.contains("market_bots") && j["market_bots"].is_object()) {
        const auto& m = j["market_bots"];
        auto& b = cfg.market_bots;
        b.enabled             = m.value("enabled", b.enabled);
        b.tick_seconds        = std::max(60, m.value("tick_seconds", b.tick_seconds));
        b.target_listings     = m.value("target_listings", b.target_listings);
        b.restock_per_tick    = m.value("restock_per_tick", b.restock_per_tick);
        b.max_per_item        = m.value("max_per_item", b.max_per_item);
        b.markup              = std::clamp(m.value("markup", b.markup), 50, 300);
        b.buy_enabled         = m.value("buy_enabled", b.buy_enabled);
        b.buy_percent         = std::min(95, m.value("buy_percent", b.buy_percent));
        b.buy_percent_fallback = std::min(b.buy_percent, m.value("buy_percent_fallback", b.buy_percent_fallback));
        b.buy_min_age_minutes = m.value("buy_min_age_minutes", b.buy_min_age_minutes);
        b.buy_per_tick        = m.value("buy_per_tick", b.buy_per_tick);
        b.cancel_per_tick     = std::clamp(m.value("cancel_per_tick", b.cancel_per_tick), 0, 50);
        b.bid_per_tick        = std::clamp(m.value("bid_per_tick", b.bid_per_tick), 0, 20);
        b.buy_daily_item_cap  = m.value("buy_daily_item_cap", b.buy_daily_item_cap);
        b.buy_daily_seller_credits = m.value("buy_daily_seller_credits", b.buy_daily_seller_credits);
    }
    if (j.contains("ap_rewards") && j["ap_rewards"].is_object()) {
        const auto& m = j["ap_rewards"];
        auto& a = cfg.ap_rewards;
        a.pvp_finish = m.value("pvp_finish", a.pvp_finish);
        a.pvp_win = m.value("pvp_win", a.pvp_win);
        a.pvp_rank1 = m.value("pvp_rank1", a.pvp_rank1);
        a.pvp_rank2 = m.value("pvp_rank2", a.pvp_rank2);
        a.pvp_rank3 = m.value("pvp_rank3", a.pvp_rank3);
        a.pvp_min_presence_percent = m.value("pvp_min_presence_percent", a.pvp_min_presence_percent);
        a.pvp_min_real_per_side = m.value("pvp_min_real_per_side", a.pvp_min_real_per_side);
        a.ava_win = m.value("ava_win", a.ava_win);
        a.ava_capture = m.value("ava_capture", a.ava_capture);
        a.ava_defend = m.value("ava_defend", a.ava_defend);
        a.season_rank1 = m.value("season_rank1", a.season_rank1);
        a.season_rank2 = m.value("season_rank2", a.season_rank2);
        a.season_rank3 = m.value("season_rank3", a.season_rank3);
        a.raid_min = m.value("raid_min", a.raid_min);
        a.raid_max = m.value("raid_max", a.raid_max);
        a.daily_cap = m.value("daily_cap", a.daily_cap);
        a.bot_match_percent = m.value("bot_match_percent", a.bot_match_percent);
    }
    if (j.contains("services")) cfg.services = ServiceHooks::ParseConfig(j["services"]);
    if (j.contains("announcers") && j["announcers"].is_array()) {
        for (const auto& item : j["announcers"]) {
            if (item.is_string()) cfg.announcers.push_back(item.get<std::string>());
        }
    }
    if (j.contains("enabled_crash_channels") && j["enabled_crash_channels"].is_array()) {
        for (const auto& item : j["enabled_crash_channels"]) {
            if (item.is_string()) cfg.enabled_crash_channels.push_back(item.get<std::string>());
        }
    }

    if (j.contains("udp_port_range")) {
        const auto& pr = j["udp_port_range"];
        cfg.udp_port_range.lo = pr.value("lo", cfg.udp_port_range.lo);
        cfg.udp_port_range.hi = pr.value("hi", cfg.udp_port_range.hi);
    }

    if (j.contains("game_cpu_range")) {
        const auto& cr = j["game_cpu_range"];
        cfg.game_cpu_range.lo = cr.value("lo", cfg.game_cpu_range.lo);
        cfg.game_cpu_range.hi = cr.value("hi", cfg.game_cpu_range.hi);
    }
    if (j.contains("cores_per_instance")) cfg.cores_per_instance = j["cores_per_instance"].get<int>();
    if (j.contains("game_cpu_relative"))  cfg.game_cpu_relative  = j["game_cpu_relative"].get<bool>();
    if (j.contains("pin_wineserver"))     cfg.pin_wineserver     = j["pin_wineserver"].get<bool>();
    if (j.contains("per_slot_prefix"))    cfg.per_slot_prefix    = j["per_slot_prefix"].get<bool>();

    if (j.contains("use_docker"))         cfg.use_docker         = j["use_docker"].get<bool>();
    if (j.contains("docker_image"))       cfg.docker_image       = j["docker_image"].get<std::string>();
    if (j.contains("docker_memory"))      cfg.docker_memory      = j["docker_memory"].get<std::string>();
    if (j.contains("docker_debug"))       cfg.docker_debug       = j["docker_debug"].get<bool>();
    if (j.contains("wine_install_dir"))   cfg.wine_install_dir   = j["wine_install_dir"].get<std::string>();
    if (j.contains("docker_extra_mounts") && j["docker_extra_mounts"].is_array()) {
        for (const auto& item : j["docker_extra_mounts"]) {
            if (item.is_string()) cfg.docker_extra_mounts.push_back(item.get<std::string>());
        }
    }

    if (const char* show_game_console = std::getenv("GA_SHOW_GAME_CONSOLE")) {
        cfg.show_game_console = EnvFlagEnabled(show_game_console);
    }

    if (j.contains("ban_spoof") && j["ban_spoof"].is_object()) {
        const auto& bs = j["ban_spoof"];
        if (bs.contains("mode") && bs["mode"].is_string()) {
            cfg.ban_spoof.mode = bs["mode"].get<std::string>();
        }
        if (bs.contains("fallback_close_sec") && bs["fallback_close_sec"].is_number_integer()) {
            cfg.ban_spoof.fallback_close_sec = bs["fallback_close_sec"].get<int>();
        }
    }
    if (j.contains("kick") && j["kick"].is_object()) {
        const auto& k = j["kick"];
        if (k.contains("fallback_close_sec") && k["fallback_close_sec"].is_number_integer()) {
            cfg.kick.fallback_close_sec = k["fallback_close_sec"].get<int>();
        }
    }

    Logger::Log("config", "[ControlServerConfig] Loaded config from '%s'\n", path.c_str());
    return cfg;
}
