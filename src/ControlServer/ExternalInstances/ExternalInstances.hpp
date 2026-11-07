#pragma once
#include "lib/nlohmann/json.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ExternalInstances -- instance processes that run outside this server and
// reach the IPC port through a separate local service. Attaching asks the
// ServiceHooks event "external_instance" {user_id, instance_id, character_id};
// a reply {"accept": true} marks the instance external. Without a configured
// service nothing is attached and instances spawn locally as usual.
// When an attached instance disconnects, "external_instance_closed"
// {instance_id} is sent once.
namespace ExternalInstances {

// extra: more fields merged into the request (map, port, ...). address: set to
// the reply's "address" (IPv4 the clients connect to), default "127.0.0.1".
bool Attach(int64_t user_id, int64_t instance_id, int64_t character_id,
            const nlohmann::json& extra = nlohmann::json::object(),
            std::string* address = nullptr);

// Non-binding question "could this player's match run outside?" -- ServiceHooks
// "external_instance_probe" {user_id, character_id, ...extra} -> {"accept": bool}.
// No service / no reply = false. Nothing is attached; Attach() still decides.
bool Probe(int64_t user_id, int64_t character_id,
           const nlohmann::json& extra = nlohmann::json::object());

// Same for a match with several players: ServiceHooks "external_instance_group"
// {instance_id, players: [{user_id, character_id}], ...extra}. The reply names
// the hosting player ("host_user_id", must be one of `players`) and optionally
// "address" + "node_id"; that player becomes the instance owner.
struct Member { int64_t user_id = 0, character_id = 0; };
bool AttachGroup(int64_t instance_id, const std::vector<Member>& players,
                 const nlohmann::json& extra = nlohmann::json::object(),
                 std::string* address = nullptr);
int Count();
bool IsExternal(int64_t instance_id);
// Owner of an attached instance (the ids passed to Attach + the service's
// "node_id" reply, 0 if none). False if the instance is not attached.
struct Owner { int64_t user_id = 0, character_id = 0, node_id = 0; };
bool GetOwner(int64_t instance_id, Owner& out);
void Detach(int64_t instance_id);

// Ask the service to stop an attached instance ("external_instance_stop").
void RequestStop(int64_t instance_id, const char* reason);

// Local fallback: called with the instance id when an attached instance is
// given up before INSTANCE_READY; returns true if it started a local process.
using Fallback = std::function<bool(int64_t instance_id)>;
void SetFallback(Fallback cb);
// Detach (if attached) and run the fallback. False if not attached or no fallback.
bool FallBackToLocal(int64_t instance_id, const char* why);
// After `seconds`, fall back if the instance is still attached and STARTING.
void ArmReadyTimer(int64_t instance_id, int seconds);

} // namespace ExternalInstances
