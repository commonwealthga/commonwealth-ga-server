#pragma once
#include "lib/nlohmann/json.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ServiceHooks -- optional external services the control server can ask for
// a decision or notify about an event. Transport: TCP to a local address,
// one request frame + one reply frame, 4-byte little-endian length + UTF-8
// JSON (same framing as the IPC port), max 64 KB.
//   request: {"v":1,"event":"<name>","id":"<n>","ts":<unix>,"data":{...}}
//   reply:   {"v":1,"id":"<n>","decision":{...}}  or  {"v":1,"id":"<n>","pass":true}
// No services configured, timeout, connect error, bad reply or "pass" ->
// Call() returns nullopt and the caller applies its built-in rule.
// Config: "services": {"<name>": {"addr":"127.0.0.1:7700","timeout_ms":200,
//                                  "events":["mission_end", ...]}}
namespace ServiceHooks {

struct Service {
    std::string name;
    std::string host = "127.0.0.1";
    uint16_t    port = 0;
    int         timeout_ms = 200;
    std::vector<std::string> events;
};

void Configure(const std::vector<Service>& services);
// Parses the "services" config object; invalid entries are skipped with a log line.
std::vector<Service> ParseConfig(const nlohmann::json& services);

// True if some service subscribes to `event`.
bool Enabled(const std::string& event);

// Blocking request (bounded by the service timeout). Returns the "decision"
// object, or nullopt = use the built-in rule.
std::optional<nlohmann::json> Call(const std::string& event, const nlohmann::json& data);

// Fire-and-forget on a worker thread; the reply is read and ignored.
void Notify(const std::string& event, nlohmann::json data);

// Helpers for reading a decision with server-side limits.
int64_t ClampInt(const nlohmann::json& obj, const char* key, int64_t fallback,
                 int64_t lo, int64_t hi);
std::string ClampString(const nlohmann::json& obj, const char* key, size_t max_len);

}  // namespace ServiceHooks
