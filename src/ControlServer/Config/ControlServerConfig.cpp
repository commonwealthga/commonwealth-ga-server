#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Logger.hpp"
#include "lib/nlohmann/json.hpp"
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
    if (j.contains("startup_timeout_seconds"))
        cfg.startup_timeout_seconds = j["startup_timeout_seconds"].get<int>();
    if (j.contains("db_path"))            cfg.db_path            = j["db_path"].get<std::string>();
    if (j.contains("crash_dir"))          cfg.crash_dir          = j["crash_dir"].get<std::string>();
    if (j.contains("log_dir"))            cfg.log_dir            = j["log_dir"].get<std::string>();
    if (j.contains("fix_package_guids"))  cfg.fix_package_guids  = j["fix_package_guids"].get<bool>();

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

    Logger::Log("config", "[ControlServerConfig] Loaded config from '%s'\n", path.c_str());
    return cfg;
}
