#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"

// NodeAgent.hpp -- the node (volunteer game-host) role.
//
// A node does four things and nothing else:
//   1. keeps an authenticated link to its master,
//   2. spawns the game instances the master asks for,
//   3. relays each instance's IPC to the master and its replies back, and
//   4. reports lifecycle events (spawning / failed / exited / heartbeat).
//
// It makes no gameplay decisions and owns no authoritative data. Everything a
// player sees is still decided by the master; the node is a supervised process
// host with a byte relay attached.

class NodeAgent {
public:
    // Runs until SIGINT/SIGTERM. Returns a process exit code.
    static int Run(const ControlServerConfig& cfg);
};
