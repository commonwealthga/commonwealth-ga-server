#include "src/ControlServer/MatchmakingService/RuleFactory.hpp"
#include "src/ControlServer/MatchmakingService/Rules/DataDrivenMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"
#include "src/ControlServer/Logger.hpp"

std::unique_ptr<MatchRule> RuleFactory::Create(const QueueConfig* cfg) {
    if (!cfg) return nullptr;

    // Default: DataDrivenMatchRule. Reached for empty rule_class (the
    // common case) and for the explicit "DataDriven" string.
    if (cfg->rule_class.empty() || cfg->rule_class == "DataDriven") {
        return std::make_unique<DataDrivenMatchRule>(cfg);
    }

    // Double Agent: asymmetric coop, shape-table driven, late-join lockout.
    if (cfg->rule_class == "DoubleAgent") {
        return std::make_unique<DoubleAgentRule>(cfg);
    }

    // Unknown rule_class — fall back to DataDriven with a loud log so a
    // typo in ga_queues.rule_class doesn't silently disable a queue.
    Logger::Log("matchmaking",
        "[Matchmaking] RuleFactory: unknown rule_class '%s' for queue %u — falling back to DataDriven\n",
        cfg->rule_class.c_str(), cfg->queue_id);
    return std::make_unique<DataDrivenMatchRule>(cfg);
}
