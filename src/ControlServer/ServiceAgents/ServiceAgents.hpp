#pragma once
// Service agents - generic server-side accounts/agents that are driven by
// an operator tool over admin IPC instead of a game client. They can never log in
// (random verifier + explicit refusal at login) and only service agents may take part
// in an externally resolved territory match.
#include <cstdint>
#include <string>
#include "lib/nlohmann/json.hpp"

namespace ServiceAgents {

void EnsureSchema();
bool IsServiceUser(int64_t user_id);
bool IsServiceCharacter(int64_t character_id);

// Admin IPC "service-agents": {"op": ...}
//   agent   {name, profile_id, xp?}                     -> {user_id, character_id, created}
//   agency  {name, color:[r,g,b], leader, members:[name]} -> {agency_id, created, added}
//   bid     {character_id, position, amount, attack_type?, cancel?} -> {ok, text}
//   match   {position, attackers:[cid], defenders:[cid], winning_task_force, map_name?}
//           -> {instance_id, owner_agency_id, owner_name}
//   assign  {character_id, position, item}  /  facility {character_id, position, id}
//           -> {agency_id, text}   (same rules/costs as the client requests)
//   list    {}                                           -> {agents:[...]}
bool Admin(const nlohmann::json& payload, nlohmann::json& out, std::string& message);

}  // namespace ServiceAgents
