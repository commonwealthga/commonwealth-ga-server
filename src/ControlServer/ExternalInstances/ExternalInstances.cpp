#include "src/ControlServer/ExternalInstances/ExternalInstances.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

#include <chrono>
#include <thread>

#include <mutex>
#include <unordered_map>

namespace ExternalInstances {

namespace {
std::mutex g_mu;
std::unordered_map<int64_t, Owner> g_attached;
Fallback g_fallback;

bool IsIpv4(const std::string& s) {
    int parts = 0, digits = 0, val = 0;
    for (char c : s) {
        if (c == '.') {
            if (digits == 0) return false;
            ++parts; digits = 0; val = 0;
        } else if (c >= '0' && c <= '9') {
            val = val * 10 + (c - '0');
            if (++digits > 3 || val > 255) return false;
        } else {
            return false;
        }
    }
    return parts == 3 && digits > 0;
}
}

bool Attach(int64_t user_id, int64_t instance_id, int64_t character_id,
            const nlohmann::json& extra, std::string* address) {
    if (user_id <= 0 || instance_id <= 0 || !ServiceHooks::Enabled("external_instance")) return false;
    nlohmann::json req = extra.is_object() ? extra : nlohmann::json::object();
    req["user_id"] = user_id;
    req["instance_id"] = instance_id;
    req["character_id"] = character_id;
    auto d = ServiceHooks::Call("external_instance", req);
    const bool ok = d && d->is_object() && d->value("accept", false) == true;
    if (ok && address) {
        const auto& a = (*d)["address"];
        *address = (a.is_string() && IsIpv4(a.get<std::string>())) ? a.get<std::string>() : "127.0.0.1";
    }
    if (ok) {
        std::lock_guard<std::mutex> lk(g_mu);
        Owner o;
        o.user_id = user_id;
        o.character_id = character_id;
        const auto& n = (*d)["node_id"];
        if (n.is_number_integer()) o.node_id = n.get<int64_t>();
        g_attached[instance_id] = o;
    }
    Logger::Log("ipc", "[External] attach instance=%lld user=%lld -> %s\n",
        (long long)instance_id, (long long)user_id, ok ? "accepted" : "refused");
    return ok;
}

bool Probe(int64_t user_id, int64_t character_id, const nlohmann::json& extra) {
    if (user_id <= 0 || !ServiceHooks::Enabled("external_instance_probe")) return false;
    nlohmann::json req = extra.is_object() ? extra : nlohmann::json::object();
    req["user_id"] = user_id;
    req["character_id"] = character_id;
    auto d = ServiceHooks::Call("external_instance_probe", req);
    return d && d->is_object() && d->value("accept", false) == true;
}

bool AttachGroup(int64_t instance_id, const std::vector<Member>& players,
                 const nlohmann::json& extra, std::string* address) {
    if (instance_id <= 0 || players.empty() || !ServiceHooks::Enabled("external_instance_group")) return false;
    nlohmann::json req = extra.is_object() ? extra : nlohmann::json::object();
    nlohmann::json list = nlohmann::json::array();
    for (const auto& m : players) {
        if (m.user_id <= 0) return false;
        list.push_back({{"user_id", m.user_id}, {"character_id", m.character_id}});
    }
    req["instance_id"] = instance_id;
    req["players"] = list;
    auto d = ServiceHooks::Call("external_instance_group", req);
    bool ok = d && d->is_object() && d->value("accept", false) == true;
    Owner o;
    if (ok) {
        const auto& h = (*d)["host_user_id"];
        const int64_t host = h.is_number_integer() ? h.get<int64_t>() : 0;
        ok = false;
        for (const auto& m : players) {
            if (host > 0 && m.user_id == host) { o.user_id = m.user_id; o.character_id = m.character_id; ok = true; break; }
        }
        // Accepted with a host outside the match: refuse and let the service clean up.
        if (!ok) ServiceHooks::Notify("external_instance_closed", {{"instance_id", instance_id}});
    }
    if (ok) {
        const auto& n = (*d)["node_id"];
        if (n.is_number_integer()) o.node_id = n.get<int64_t>();
        if (address) {
            const auto& a = (*d)["address"];
            *address = (a.is_string() && IsIpv4(a.get<std::string>())) ? a.get<std::string>() : "127.0.0.1";
        }
        std::lock_guard<std::mutex> lk(g_mu);
        g_attached[instance_id] = o;
    }
    Logger::Log("ipc", "[External] attach group instance=%lld players=%d host=%lld -> %s\n",
        (long long)instance_id, (int)players.size(), (long long)o.user_id, ok ? "accepted" : "refused");
    return ok;
}

int Count() {
    std::lock_guard<std::mutex> lk(g_mu);
    return (int)g_attached.size();
}

bool IsExternal(int64_t instance_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_attached.count(instance_id) != 0;
}

bool GetOwner(int64_t instance_id, Owner& out) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_attached.find(instance_id);
    if (it == g_attached.end()) return false;
    out = it->second;
    return true;
}

void Detach(int64_t instance_id) {
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (g_attached.erase(instance_id) == 0) return;
    }
    ServiceHooks::Notify("external_instance_closed", {{"instance_id", instance_id}});
    Logger::Log("ipc", "[External] detach instance=%lld\n", (long long)instance_id);
}

void RequestStop(int64_t instance_id, const char* reason) {
    if (!IsExternal(instance_id)) return;
    ServiceHooks::Notify("external_instance_stop",
        {{"instance_id", instance_id}, {"reason", reason ? reason : ""}});
    Logger::Log("ipc", "[External] stop requested instance=%lld reason=%s\n",
        (long long)instance_id, reason ? reason : "");
}

void SetFallback(Fallback cb) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_fallback = std::move(cb);
}

bool FallBackToLocal(int64_t instance_id, const char* why) {
    Fallback cb;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (g_attached.erase(instance_id) == 0) return false;  // already handled
        cb = g_fallback;
    }
    ServiceHooks::Notify("external_instance_closed", {{"instance_id", instance_id}});
    const bool ok = cb && cb(instance_id);
    Logger::Log("ipc", "[External] instance=%lld given up (%s) -> local %s\n",
        (long long)instance_id, why ? why : "", ok ? "spawned" : "FAILED");
    return ok;
}

void ArmReadyTimer(int64_t instance_id, int seconds) {
    std::thread([instance_id, seconds]() {
        std::this_thread::sleep_for(std::chrono::seconds(seconds > 0 ? seconds : 1));
        if (!IsExternal(instance_id)) return;
        auto inst = InstanceRegistry::GetInstanceById(instance_id);
        if (!inst || inst->state != "STARTING") return;
        if (!FallBackToLocal(instance_id, "not ready in time")) {
            // Nothing started locally: unstick the players and stop the row.
            MatchmakingService::DiscardPendingMatchForDeadInstance(instance_id, "external instance not ready");
            InstanceRegistry::MarkStopped(instance_id);
        }
    }).detach();
}

} // namespace ExternalInstances
