#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/Logger.hpp"

#include <asio.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <mutex>
#include <thread>

namespace ServiceHooks {

namespace {

constexpr size_t kMaxFrame = 64 * 1024;
constexpr size_t kMaxQueue = 256;

std::mutex g_mu;
std::vector<Service> g_services;
std::atomic<uint64_t> g_next_id{1};

// Notify worker. Heap-allocated and never freed: the detached worker may
// still be waiting on the condition variable when static destructors run.
struct NotifyQueue {
    std::mutex mu;
    std::condition_variable cv;
    std::deque<std::pair<std::string, nlohmann::json>> items;
    bool worker_started = false;
};
NotifyQueue& Q() {
    static NotifyQueue* q = new NotifyQueue();
    return *q;
}

const Service* FindLocked(const std::string& event) {
    for (const auto& s : g_services)
        if (std::find(s.events.begin(), s.events.end(), event) != s.events.end())
            return &s;
    return nullptr;
}

// One request/reply exchange bounded by timeout_ms. Returns the reply JSON
// or nullopt with `why` set.
std::optional<nlohmann::json> Exchange(const Service& s, const std::string& payload,
                                       std::string& why) {
    if (payload.size() > kMaxFrame) { why = "request too large"; return std::nullopt; }
    asio::io_context io;
    asio::ip::tcp::socket sock(io);
    asio::error_code addr_ec;
    const auto addr = asio::ip::make_address(s.host, addr_ec);
    if (addr_ec) { why = "bad addr"; return std::nullopt; }
    const asio::ip::tcp::endpoint ep(addr, s.port);

    std::array<uint8_t, 4> hdr_out{};
    const uint32_t n = static_cast<uint32_t>(payload.size());
    for (int i = 0; i < 4; ++i) hdr_out[i] = static_cast<uint8_t>((n >> (8 * i)) & 0xFF);
    std::array<uint8_t, 4> hdr_in{};
    std::string body;
    bool done = false;

    auto fail = [&](const char* stage, const asio::error_code& ec) {
        why = std::string(stage) + ": " + ec.message();
        done = true;
    };
    sock.async_connect(ep, [&](const asio::error_code& ec) {
        if (ec) return fail("connect", ec);
        const std::array<asio::const_buffer, 2> out{asio::buffer(hdr_out), asio::buffer(payload)};
        asio::async_write(sock, out, [&](const asio::error_code& ec, size_t) {
            if (ec) return fail("write", ec);
            asio::async_read(sock, asio::buffer(hdr_in), [&](const asio::error_code& ec, size_t) {
                if (ec) return fail("read", ec);
                const uint32_t len = hdr_in[0] | (hdr_in[1] << 8) | (hdr_in[2] << 16) |
                                     (static_cast<uint32_t>(hdr_in[3]) << 24);
                if (len == 0 || len > kMaxFrame) { why = "bad reply length"; done = true; return; }
                body.resize(len);
                asio::async_read(sock, asio::buffer(body), [&](const asio::error_code& ec, size_t) {
                    if (ec) return fail("read", ec);
                    done = true;
                });
            });
        });
    });
    io.run_for(std::chrono::milliseconds(std::max(1, s.timeout_ms)));
    if (!done) { why = "timeout"; return std::nullopt; }
    if (!why.empty()) return std::nullopt;
    auto j = nlohmann::json::parse(body, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) { why = "bad json"; return std::nullopt; }
    return j;
}

std::optional<nlohmann::json> Send(const Service& s, const std::string& event,
                                   const nlohmann::json& data) {
    const std::string id = std::to_string(g_next_id.fetch_add(1));
    nlohmann::json req;
    req["v"] = 1;
    req["event"] = event;
    req["id"] = id;
    req["ts"] = static_cast<int64_t>(std::time(nullptr));
    req["data"] = data.is_object() ? data : nlohmann::json::object();
    std::string why;
    auto reply = Exchange(s, req.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace), why);
    if (!reply) {
        Logger::Log("hooks", "[HOOK] %s %s fallback: %s\n", s.name.c_str(), event.c_str(), why.c_str());
        return std::nullopt;
    }
    if (reply->value("id", std::string()) != id) {
        Logger::Log("hooks", "[HOOK] %s %s fallback: wrong id\n", s.name.c_str(), event.c_str());
        return std::nullopt;
    }
    if (reply->value("pass", false)) return std::nullopt;
    if (!reply->contains("decision") || !(*reply)["decision"].is_object()) {
        Logger::Log("hooks", "[HOOK] %s %s fallback: no decision\n", s.name.c_str(), event.c_str());
        return std::nullopt;
    }
    return (*reply)["decision"];
}

void WorkerLoop() {
    for (;;) {
        std::pair<std::string, nlohmann::json> item;
        {
            auto& q = Q();
            std::unique_lock<std::mutex> lk(q.mu);
            q.cv.wait(lk, [&q] { return !q.items.empty(); });
            item = std::move(q.items.front());
            q.items.pop_front();
        }
        Service s;
        {
            std::lock_guard<std::mutex> lk(g_mu);
            const Service* p = FindLocked(item.first);
            if (!p) continue;
            s = *p;
        }
        Send(s, item.first, item.second);
    }
}

}  // namespace

std::vector<Service> ParseConfig(const nlohmann::json& services) {
    std::vector<Service> out;
    if (!services.is_object()) return out;
    for (auto it = services.begin(); it != services.end(); ++it) {
        const auto& v = it.value();
        if (!v.is_object()) continue;
        Service s;
        s.name = it.key();
        const std::string addr = v.value("addr", std::string());
        const auto colon = addr.rfind(':');
        int port = 0;
        if (colon != std::string::npos) {
            s.host = addr.substr(0, colon);
            port = std::atoi(addr.c_str() + colon + 1);
        }
        if (port <= 0 || port > 65535 || s.host.empty()) {
            Logger::Log("hooks", "[HOOK] service '%s' skipped: bad addr '%s'\n", s.name.c_str(), addr.c_str());
            continue;
        }
        s.port = static_cast<uint16_t>(port);
        s.timeout_ms = std::clamp(v.value("timeout_ms", 200), 10, 5000);
        if (v.contains("events") && v["events"].is_array())
            for (const auto& e : v["events"])
                if (e.is_string()) s.events.push_back(e.get<std::string>());
        out.push_back(std::move(s));
    }
    return out;
}

void Configure(const std::vector<Service>& services) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_services = services;
    for (const auto& s : g_services) {
        std::string ev;
        for (const auto& e : s.events) ev += (ev.empty() ? "" : ",") + e;
        Logger::Log("hooks", "[HOOK] service %s -> %s:%u (%d ms) events=%s\n",
                    s.name.c_str(), s.host.c_str(), s.port, s.timeout_ms, ev.c_str());
    }
}

bool Enabled(const std::string& event) {
    std::lock_guard<std::mutex> lk(g_mu);
    return FindLocked(event) != nullptr;
}

std::optional<nlohmann::json> Call(const std::string& event, const nlohmann::json& data) {
    Service s;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        const Service* p = FindLocked(event);
        if (!p) return std::nullopt;
        s = *p;
    }
    return Send(s, event, data);
}

void Notify(const std::string& event, nlohmann::json data) {
    if (!Enabled(event)) return;
    auto& q = Q();
    std::lock_guard<std::mutex> lk(q.mu);
    if (!q.worker_started) {
        std::thread(WorkerLoop).detach();
        q.worker_started = true;
    }
    if (q.items.size() >= kMaxQueue) {
        Logger::Log("hooks", "[HOOK] %s dropped: queue full\n", event.c_str());
        return;
    }
    q.items.emplace_back(event, std::move(data));
    q.cv.notify_one();
}

int64_t ClampInt(const nlohmann::json& obj, const char* key, int64_t fallback,
                 int64_t lo, int64_t hi) {
    if (!obj.is_object() || !obj.contains(key)) return fallback;
    const auto& v = obj[key];
    if (!v.is_number()) return fallback;
    const double d = v.get<double>();
    if (d != d) return fallback;
    if (d <= static_cast<double>(lo)) return lo;
    if (d >= static_cast<double>(hi)) return hi;
    return static_cast<int64_t>(d);
}

std::string ClampString(const nlohmann::json& obj, const char* key, size_t max_len) {
    if (!obj.is_object() || !obj.contains(key) || !obj[key].is_string()) return std::string();
    std::string s = obj[key].get<std::string>();
    if (s.size() > max_len) s.resize(max_len);
    return s;
}

}  // namespace ServiceHooks
