#pragma once

#include <string>
#include <cstdint>

// ControlServerConfig.hpp -- Configuration for the control server.
// Loaded from a JSON file at startup; all fields have sensible defaults.

struct PortRange {
    uint16_t lo;
    uint16_t hi;
};

struct ControlServerConfig {
    std::string wine_binary        = "/home/zax/lutrisproton/lutrisprotonge/bin/wine";
    std::string wine_prefix        = "/home/zax/games/gawineprefixserver";
    std::string xvfb_run_path      = "xvfb-run";
    std::string game_binary        = "/home/zax/games/ga/Binaries/GlobalAgenda.exe";
    std::string host               = "77.237.240.162";
    std::string hostdns            = "77.237.240.162";
    std::string dll_overrides      = "version=n,b";
    std::string home_map_name      = "Rot_Redistribution05";
    std::string home_map_game_mode = "TgGame.TgGame";
    PortRange   udp_port_range     = {9002, 9020};
    uint16_t    tcp_port           = 9000;
    uint16_t    ipc_port           = 9010;
    int         startup_timeout_seconds = 120;
    std::string db_path            = "server.db";

    // Load config from JSON file at path. Returns defaults if file is absent or invalid.
    static ControlServerConfig Load(const std::string& path);
};
