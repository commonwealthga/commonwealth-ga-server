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
    if (j.contains("dll_overrides"))      cfg.dll_overrides      = j["dll_overrides"].get<std::string>();
    if (j.contains("home_map_name"))      cfg.home_map_name      = j["home_map_name"].get<std::string>();
    if (j.contains("home_map_game_mode")) cfg.home_map_game_mode = j["home_map_game_mode"].get<std::string>();
    if (j.contains("tcp_port"))           cfg.tcp_port           = j["tcp_port"].get<uint16_t>();
    if (j.contains("chat_port"))          cfg.chat_port          = j["chat_port"].get<uint16_t>();
    if (j.contains("ipc_port"))           cfg.ipc_port           = j["ipc_port"].get<uint16_t>();
    if (j.contains("admin_token"))        cfg.admin_token        = j["admin_token"].get<std::string>();
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
    if (j.contains("require_password_verification"))
        cfg.require_password_verification = j["require_password_verification"].get<bool>();
    if (j.contains("afk_kick_seconds"))
        cfg.afk_kick_seconds = j["afk_kick_seconds"].get<int>();

    if (j.contains("enabled_channels") && j["enabled_channels"].is_array()) {
        for (const auto& item : j["enabled_channels"]) {
            if (item.is_string()) cfg.enabled_channels.push_back(item.get<std::string>());
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
