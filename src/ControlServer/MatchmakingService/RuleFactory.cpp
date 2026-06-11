#include "src/ControlServer/MatchmakingService/RuleFactory.hpp"
#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"
#include "src/ControlServer/MatchmakingService/Rules/VersusSidesRule.hpp"
#include "src/ControlServer/Logger.hpp"

std::unique_ptr<MatchRule> RuleFactory::Create(const QueueConfig* cfg) {
    if (!cfg) return nullptr;

    // Default pool rule (PvE coop / mercenary / DDR). Reached for empty
    // rule_class and the explicit "DataDriven"/"Coop" strings.
    if (cfg->rule_class.empty() || cfg->rule_class == "DataDriven" || cfg->rule_class == "Coop") {
        return std::make_unique<CoopMatchRule>(cfg);
    }

    // Double Agent: asymmetric coop, shape-table driven, late-join lockout.
    if (cfg->rule_class == "DoubleAgent") {
        return std::make_unique<DoubleAgentRule>(cfg);
    }

    // 1v1 / private: each side is a whole party, matched against another.
    if (cfg->rule_class == "VersusSides" || cfg->rule_class == "1v1") {
        return std::make_unique<VersusSidesRule>(cfg);
    }

    Logger::Log("matchmaking",
        "[Matchmaking] RuleFactory: unknown rule_class '%s' for queue %u — falling back to Coop\n",
        cfg->rule_class.c_str(), cfg->queue_id);
    return std::make_unique<CoopMatchRule>(cfg);
}
