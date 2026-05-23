#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include <memory>

// RuleFactory — turn a QueueConfig's rule_class string into a concrete
// MatchRule. Empty/unknown rule_class falls back to DataDrivenMatchRule,
// which is the generic policy-parameterised rule covering every queue we
// currently ship. Add a case here for any genuinely custom future rule.
class RuleFactory {
public:
    // The returned rule holds a borrowed pointer to `cfg`; the caller (the
    // Queue inside MatchmakingService) must keep that QueueConfig alive for
    // the rule's lifetime.
    static std::unique_ptr<MatchRule> Create(const QueueConfig* cfg);
};
