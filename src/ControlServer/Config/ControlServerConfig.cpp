#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <fstream>

// ControlServerConfig.cpp -- JSON config file loader.
// All fields are optional in the JSON; missing fields retain their defaults.

ControlServerConfig ControlServerConfig::Load(const std::string& path) {
    ControlServerConfig cfg;

    std::ifstream f(path);
    if (!f.is_open()) {
        Logger::Log("config", "[ControlServerConfig] Config file not found at '%s', using defaults\n",
            path.c_str());
        return cfg;
    }

    nlohmann::json j = nlohmann::json::parse(f, nullptr, /*allow_exceptions=*/false);
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
    if (j.contains("fix_package_guids"))  cfg.fix_package_guids  = j["fix_package_guids"].get<bool>();

    if (j.contains("udp_port_range")) {
        const auto& pr = j["udp_port_range"];
        cfg.udp_port_range.lo = pr.value("lo", cfg.udp_port_range.lo);
        cfg.udp_port_range.hi = pr.value("hi", cfg.udp_port_range.hi);
    }

    Logger::Log("config", "[ControlServerConfig] Loaded config from '%s'\n", path.c_str());
    return cfg;
}
