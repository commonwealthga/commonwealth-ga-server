#include "src/ControlServer/Placement/Placement.hpp"

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeRegistry/NodeRegistry.hpp"

#include <algorithm>
#include <climits>

namespace Placement {

Choice Choose(const ControlServerConfig& cfg, const std::vector<int>& preferred_sites,
              std::string* trace) {
    Choice choice;
    std::string log;

    const int master_site = cfg.ResolveSite(cfg.master_site, nullptr);
    const int preferred   = preferred_sites.empty() ? master_site : preferred_sites.front();
    const std::vector<int> ladder = cfg.FallbackLadder(preferred);

    if (!preferred_sites.empty() && preferred != preferred_sites.front()) {
        log += "  preferred site " + std::to_string(preferred_sites.front()) +
               " is not served; starting at " + std::to_string(preferred) + "\n";
    }

    for (int site : ladder) {
        struct Candidate {
            int node_id;
            int load;
        };
        std::vector<Candidate> candidates;

        for (const auto& rt : NodeRegistry::Snapshot()) {
            if (rt.id == 0 || rt.site != site) continue;
            std::string why;
            if (!NodeRegistry::CanAcceptWork(rt.id, why)) {
                log += "  site " + std::to_string(site) + ": node '" + rt.name +
                       "' (id " + std::to_string(rt.id) + ") refused: " + why + "\n";
                continue;
            }
            candidates.push_back({rt.id, InstanceRegistry::CountActiveInstancesForNode(rt.id)});
        }

        // The master's own spawner belongs to its own site and always accepts
        // work, so it competes on load like any other candidate for that site.
        if (site == master_site) {
            candidates.push_back({0, InstanceRegistry::CountActiveInstancesForNode(0)});
        }

        if (candidates.empty()) {
            log += "  site " + std::to_string(site) + ": no node can take work\n";
            continue;
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b) { return a.load < b.load; });
        const Candidate& best = candidates.front();

        choice.ok      = true;
        choice.node_id = best.node_id;
        choice.site    = site;
        log += "  site " + std::to_string(site) + ": chose " +
               (best.node_id == 0 ? std::string("the master's own spawner")
                                  : ("node '" + NodeRegistry::NameOf(best.node_id) +
                                     "' (id " + std::to_string(best.node_id) + ")")) +
               " at load " + std::to_string(best.load) + "\n";
        if (trace) *trace = log;
        return choice;
    }

    // Nothing in the ladder accepted work (or no site is configured). The
    // master's spawner is the last resort: refusing to place is worse than
    // running one more instance locally.
    choice.ok      = true;
    choice.node_id = 0;
    choice.site    = master_site;
    log += "  no site accepted the work; using the master's own spawner (site " +
           std::to_string(master_site) + ")\n";
    if (trace) *trace = log;

    Logger::Log("matchmaking", "[Placement] fell through the ladder to the master\\n%s",
        log.c_str());
    return choice;
}

}  // namespace Placement
