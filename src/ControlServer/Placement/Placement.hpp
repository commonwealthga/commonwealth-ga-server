#pragma once

#include <string>
#include <vector>

struct ControlServerConfig;

namespace Placement {

struct Choice {
    bool        ok      = false;
    int         node_id = 0;   // 0 = the master's own spawner
    int         site    = 0;
    std::string error;
};

// Decides where new work runs.
//
// The players' region choices pick the first site; `fallback_order` in config
// decides what to try after that. Within a site the least-loaded node that
// reports it can take work wins, and the master's own spawner competes for its
// own site on the same rule rather than being a special case.
//
// `preferred_sites` is in priority order; empty means "no preference", which
// starts the ladder at the master's site. `trace` (optional) receives one line
// per rejected candidate, which is what makes a placement decision auditable
// after the fact.
Choice Choose(const ControlServerConfig& cfg,
              const std::vector<int>& preferred_sites,
              std::string* trace = nullptr);

}  // namespace Placement
