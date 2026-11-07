#pragma once
#include <cstdint>
#include <string>
#include "lib/nlohmann/json.hpp"

// NodeClaims -- economy events (NODE_CLAIM) reported by external instances
// (ExternalInstances) that run with their own local DB. Each claim is checked
// and, when accepted, applied to this server's DB:
//   kill    {bot_id}       -> the bot's xp_value / currency_value (asm_data_set_bots)
//   consume {inventory_id} -> the item is removed from the owner's inventory
// The ServiceHooks event "node_claim" {user_id, char_id, node_id, instance_id,
// map, kind, bot_id|inventory_id, seq, t, elapsed_s} may decide accept/reject;
// without a service the built-in rule applies: claim must match the instance
// owner, seq must increase, the bot must exist, and per-instance totals stay
// under kKillCap / kConsumeCap.
// Mission-end rewards of an external instance go through "mission_end"
// {user_id, char_id, node_id, instance_id, map, elapsed_s}; built-in rule:
// at least kMinMissionSecs since the instance started.
namespace NodeClaims {

constexpr int kKillCap        = 400;
constexpr int kConsumeCap     = 30;
constexpr int kMinMissionSecs = 120;

// Returns true if the claim was accepted and applied.
bool Handle(int64_t instance_id, const nlohmann::json& claim);
// Non-external instances always return true. A service reply
// {"accept": true, "defer_results": true} sets *defer_results: the caller
// grants the normal rewards but parks the rating fold + AvA territory result
// (DeferResults) until an admin "instance-results" commit / drop.
// `report` (the instance's own MISSION_ENDED fields, e.g. outcome) is passed
// to the service together with our side's player join/leave times and the
// accepted claim counts.
bool MissionEndAccepted(int64_t instance_id, bool* defer_results = nullptr,
                        const nlohmann::json& report = nlohmann::json::object());

// Deferred match results (table ga_instance_result_hold). Held instances are
// skipped by the MMR fold; commit = row removed, MMR fold + AvA resolve run;
// drop = row kept as 'dropped' (never folded, no territory change).
void DeferResults(int64_t instance_id, int winning_task_force);
bool CommitResults(int64_t instance_id, std::string& message);
bool DropResults(int64_t instance_id, std::string& message);
nlohmann::json ListResults();
void Forget(int64_t instance_id);

} // namespace NodeClaims
