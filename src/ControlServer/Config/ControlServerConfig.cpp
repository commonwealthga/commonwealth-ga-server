#include "src/ControlServer/Config/ControlServerConfig.hpp"

#include <algorithm>
#include <cstdlib>
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
    if (j.contains("role") && j["role"].is_string()) {
        const std::string role = j["role"].get<std::string>();
        if (role == "master" || role == "node") {
            cfg.role = role;
        } else {
            Logger::Log("config",
                "[ControlServerConfig] Unknown role '%s' (expected 'master' or 'node') — using 'master'\n",
                role.c_str());
        }
    }
    if (j.contains("wine_prefix"))        cfg.wine_prefix        = j["wine_prefix"].get<std::string>();
    if (j.contains("xvfb_run_path"))      cfg.xvfb_run_path      = j["xvfb_run_path"].get<std::string>();
    if (j.contains("game_binary"))        cfg.game_binary        = j["game_binary"].get<std::string>();
    if (j.contains("host"))               cfg.host               = j["host"].get<std::string>();
    //if (j.contains("hostdns"))            cfg.hostdns            = j["hostdns"].get<std::string>(); // skal deprecated
    if (j.contains("nat_networks"))       cfg.nat_networks       = j["nat_networks"].get<std::string>();
    if (j.contains("nat_ip"))             cfg.nat_ip             = j["nat_ip"].get<std::string>();
    //if (j.contains("dll_overrides"))      cfg.dll_overrides      = j["dll_overrides"].get<std::string>();
    if (j.contains("home_map_name"))      cfg.home_map_name      = j["home_map_name"].get<std::string>();
    if (j.contains("home_map_game_mode")) cfg.home_map_game_mode = j["home_map_game_mode"].get<std::string>();
    if (j.contains("tcp_port"))           cfg.tcp_port           = j["tcp_port"].get<uint16_t>();
    if (j.contains("chat_port"))          cfg.chat_port          = j["chat_port"].get<uint16_t>();
    if (j.contains("ipc_port"))           cfg.ipc_port           = j["ipc_port"].get<uint16_t>();
    if (j.contains("ipc_bind"))           cfg.ipc_bind           = j["ipc_bind"].get<std::string>();
    if (j.contains("node_port"))          cfg.node_port          = j["node_port"].get<uint16_t>();
    if (j.contains("node_bind"))          cfg.node_bind          = j["node_bind"].get<std::string>();
    if (j.contains("content_dir"))        cfg.content_dir        = j["content_dir"].get<std::string>();
    if (j.contains("node_key_secret"))    cfg.node_key_secret    = j["node_key_secret"].get<std::string>();
    if (j.contains("node_dll"))           cfg.node_dll           = j["node_dll"].get<std::string>();
    if (j.contains("node_auto_update"))   cfg.node_auto_update   = j["node_auto_update"].get<bool>();
    if (j.contains("master_site"))        cfg.master_site        = j["master_site"].get<int>();
    if (j.contains("sites") && j["sites"].is_array()) {
        cfg.sites.clear();
        for (const auto& s : j["sites"]) {
            SiteEntryConfig site;
            if (s.contains("id"))           site.id          = s["id"].get<int>();
            if (s.contains("name_msg_id"))  site.name_msg_id = s["name_msg_id"].get<int>();
            if (s.contains("label"))        site.label       = s["label"].get<std::string>();
            if (s.contains("enabled"))      site.enabled     = s["enabled"].get<bool>();
            if (site.id > 0) cfg.sites.push_back(site);
        }
    }
    if (j.contains("fallback_order") && j["fallback_order"].is_object()) {
        cfg.fallback_order.clear();
        for (auto it = j["fallback_order"].begin(); it != j["fallback_order"].end(); ++it) {
            const int from = std::atoi(it.key().c_str());
            if (from <= 0) continue;
            std::vector<int> ladder;
            if (it.value().is_array()) {
                for (const auto& v : it.value()) {
                    if (v.is_number_integer()) ladder.push_back(v.get<int>());
                }
            }
            if (!ladder.empty()) cfg.fallback_order[from] = std::move(ladder);
        }
    }
    if (j.contains("base_db"))            cfg.base_db            = j["base_db"].get<std::string>();
    if (j.contains("apply_content"))      cfg.apply_content      = j["apply_content"].get<bool>();
    if (j.contains("admin_token"))        cfg.admin_token        = j["admin_token"].get<std::string>();
    if (j.contains("overlay_http_port"))  cfg.overlay_http_port  = j["overlay_http_port"].get<uint16_t>();
    if (j.contains("overlay_token"))      cfg.overlay_token      = j["overlay_token"].get<std::string>();
    if (j.contains("overlay_bind"))       cfg.overlay_bind       = j["overlay_bind"].get<std::string>();
    if (j.contains("settings_api_port"))  cfg.settings_api_port  = j["settings_api_port"].get<uint16_t>();
    if (j.contains("settings_api_bind"))  cfg.settings_api_bind  = j["settings_api_bind"].get<std::string>();
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
    if (j.contains("require_password_verification"))
        cfg.require_password_verification = j["require_password_verification"].get<bool>();

    if (j.contains("enabled_channels") && j["enabled_channels"].is_array()) {
        for (const auto& item : j["enabled_channels"]) {
            if (item.is_string()) cfg.enabled_channels.push_back(item.get<std::string>());
        }
    }
    if (j.contains("node") && j["node"].is_object()) {
        const auto& n = j["node"];
        try {
            if (n.contains("id") && n["id"].is_string())
                cfg.node.id = n["id"].get<std::string>();
            if (n.contains("key") && n["key"].is_string())
                cfg.node.key = n["key"].get<std::string>();
            if (n.contains("master_addr") && n["master_addr"].is_string())
                cfg.node.master_addr = n["master_addr"].get<std::string>();
            if (n.contains("update_mode") && n["update_mode"].is_string()) {
                const std::string mode = n["update_mode"].get<std::string>();
                if (mode == "apply" || mode == "stage" || mode == "off") {
                    cfg.node.update_mode = mode;
                } else {
                    Logger::Log("config",
                        "[ControlServerConfig] node.update_mode '%s' is not "
                        "apply/stage/off -- keeping '%s'\n",
                        mode.c_str(), cfg.node.update_mode.c_str());
                }
            }
        } catch (const std::exception& e) {
            Logger::Log("config", "[ControlServerConfig] node block ignored: %s\n", e.what());
        }
    }

    if (j.contains("nodes") && j["nodes"].is_array()) {
        // Guarded per-entry: a typo in one volunteer's block must not stop the
        // server from starting; it just leaves that node out (and logged).
        for (const auto& item : j["nodes"]) {
            if (!item.is_object()) {
                Logger::Log("config", "[ControlServerConfig] nodes[]: non-object entry ignored\n");
                continue;
            }
            try {
                NodeEntryConfig n;
                n.id       = item.value("id", 0);
                n.name     = item.value("name", std::string());
                n.key      = item.value("key", std::string());
                n.site     = item.value("site", 0);
                n.address  = item.value("address", std::string());
                n.capacity = item.value("capacity", 0);
                n.home_map = item.value("home_map", false);
                n.enabled  = item.value("enabled", true);
                cfg.nodes.push_back(std::move(n));
            } catch (const std::exception& e) {
                Logger::Log("config",
                    "[ControlServerConfig] nodes[]: entry ignored (bad field type: %s)\n",
                    e.what());
            }
        }
    }

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

// ─── Site resolution ─────────────────────────────────────────────────────────
//
// One place decides which site a client is on and what it may fall back to, so
// placement and the login reply can never disagree.

const std::vector<SiteEntryConfig>& ControlServerConfig::Sites() const {
    static const std::vector<SiteEntryConfig> kDefault = {
        // Retail's name for site 2; the id the client has always been served.
        SiteEntryConfig{2, 41483, "Europe", true},
    };
    return sites.empty() ? kDefault : sites;
}

int ControlServerConfig::ResolveSite(int requested, std::string* why) const {
    const auto& all = Sites();
    for (const auto& s : all) {
        if (s.enabled && s.id == requested) {
            if (why) *why = "requested";
            return s.id;
        }
    }
    // Unknown or disabled: first enabled site in config order. A client that
    // asks for a region we do not serve still has to land somewhere.
    for (const auto& s : all) {
        if (s.enabled) {
            if (why) {
                *why = (requested == 0)
                    ? "no preference"
                    : "requested site " + std::to_string(requested) + " not served";
            }
            return s.id;
        }
    }
    if (why) *why = "no enabled site configured";
    return 0;
}

std::vector<int> ControlServerConfig::FallbackLadder(int preferred) const {
    std::vector<int> ladder;
    auto it = fallback_order.find(preferred);
    if (it != fallback_order.end()) {
        ladder = it->second;
    } else {
        // Default: the preferred site first, then every other enabled site in
        // config order. Deterministic, and "never lands in the wrong region" is
        // achieved by writing an explicit ladder in config.
        ladder.push_back(preferred);
        for (const auto& s : Sites()) {
            if (s.enabled && s.id != preferred) ladder.push_back(s.id);
        }
    }
    // Only sites we can actually place on, and a site appears once.
    std::vector<int> filtered;
    for (int id : ladder) {
        bool enabled = false;
        for (const auto& s : Sites()) {
            if (s.id == id && s.enabled) { enabled = true; break; }
        }
        if (!enabled) continue;
        if (std::find(filtered.begin(), filtered.end(), id) != filtered.end()) continue;
        filtered.push_back(id);
    }
    return filtered;
}
