#include "src/ControlServer/SpectatorOverlay/OverlayHttpServer.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>

#include "lib/nlohmann/json.hpp"

#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/SpectatorOverlay/SpectatorOverlayState.hpp"
#include "src/ControlServer/SpectatorOverlay/SkillTreeCatalog.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

namespace {

// Opaque per-player correlation key for the overlay JSON. The pages join
// /overlay and /overlay/builds rows per player; the session GUID itself is a
// live credential elsewhere (chat bind, TCP session lookup) and must never
// leave this process on a public-facing endpoint. FNV-1a is stable for the
// lifetime of the session, which is all the join needs.
std::string OpaquePlayerKey(const std::string& session_guid) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : session_guid) {
        h ^= c;
        h *= 1099511628211ull;
    }
    char out[17];
    snprintf(out, sizeof(out), "%016llx", (unsigned long long)h);
    return std::string(out);
}

std::map<std::string, std::string> ParseQuery(const std::string& query) {
    std::map<std::string, std::string> out;
    size_t pos = 0;
    while (pos < query.size()) {
        size_t amp = query.find('&', pos);
        std::string kv = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
        size_t eq = kv.find('=');
        if (eq != std::string::npos) {
            out[kv.substr(0, eq)] = kv.substr(eq + 1);
        }
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return out;
}

// One-shot per-connection handler: read the request line, respond, close.
// No keep-alive — a ~300ms poll interval from the browser-source page makes
// connection setup cost irrelevant, and it keeps this genuinely minimal.
class OverlayHttpSession : public std::enable_shared_from_this<OverlayHttpSession> {
public:
    OverlayHttpSession(asio::ip::tcp::socket socket, std::string token)
        : socket_(std::move(socket)), token_(std::move(token)),
          deadline_(socket_.get_executor()) {}

    void start() {
        // Read/response deadline: without it an idle or byte-dribbling client
        // holds this session (and its fd) open forever, and nothing bounds
        // how many such sessions accumulate on the shared io thread.
        auto self(shared_from_this());
        deadline_.expires_after(std::chrono::seconds(10));
        deadline_.async_wait([this, self](std::error_code ec) {
            if (ec) return;  // cancelled -- request completed in time
            std::error_code ignored;
            socket_.close(ignored);
        });
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(buf_),
            [this, self](std::error_code ec, std::size_t len) {
                if (ec) return;
                request_.append(buf_.data(), len);
                // GET requests have no body we care about — the request line
                // alone (up to the first CRLF) is enough to route + respond.
                // Cap total buffered bytes so a slow/garbage client can't hold
                // the connection reading forever.
                if (request_.find("\r\n") != std::string::npos || request_.size() > 8192) {
                    handle_request();
                    return;
                }
                do_read();
            });
    }

    void handle_request() {
        std::string method, target;
        {
            std::istringstream iss(request_);
            iss >> method >> target;
        }

        if (method != "GET") {
            send_response(405, "Method Not Allowed", "{}");
            return;
        }

        std::string path = target;
        std::string query;
        const size_t qpos = target.find('?');
        if (qpos != std::string::npos) {
            path = target.substr(0, qpos);
            query = target.substr(qpos + 1);
        }

        if (path != "/overlay" && path != "/overlay/instances" &&
            path != "/overlay/skilltree" && path != "/overlay/builds") {
            send_response(404, "Not Found", "{}");
            return;
        }

        const auto params = ParseQuery(query);

        if (!token_.empty()) {
            auto it = params.find("token");
            if (it == params.end() || it->second != token_) {
                send_response(403, "Forbidden", R"({"error":"invalid token"})");
                return;
            }
        }

        if (path == "/overlay/instances") {
            handle_instances_request();
            return;
        }

        if (path == "/overlay/skilltree") {
            handle_skilltree_request(params);
            return;
        }

        if (path == "/overlay/builds") {
            handle_builds_request(params);
            return;
        }

        int64_t instance_id = 0;
        if (auto it = params.find("instance_id"); it != params.end()) {
            instance_id = std::strtoll(it->second.c_str(), nullptr, 10);
        }
        if (instance_id == 0) {
            send_response(400, "Bad Request", R"({"error":"missing instance_id"})");
            return;
        }

        const auto snaps = SpectatorOverlayState::GetForInstance(instance_id);

        nlohmann::json players = nlohmann::json::array();
        for (const auto& s : snaps) {
            nlohmann::json p;
            p["player_key"] = OpaquePlayerKey(s.session_guid);
            p["task_force"]   = s.task_force;
            p["health"]       = s.health;
            p["health_max"]   = s.health_max;
            p["power"]        = s.power;
            p["power_max"]    = s.power_max;
            p["effect_ids"]   = s.effect_ids;
            p["skill_ids"]    = s.skill_ids;
            auto info = PlayerSessionStore::GetByGuid(s.session_guid);
            p["player_name"] = info ? info->player_name : "";
            // The player's class -- distinct from effect_ids/skill_ids
            // (UTgEffectGroup ids for buffs/skill-tree effects). Already
            // tracked per session (SessionInfo::selected_profile_id, set
            // from PLAYER_REGISTER's profile_id field) for unrelated
            // reasons (loadout/spawn logic) -- reused here as-is for the
            // class icon lookup, no new plumbing needed.
            p["profile_id"] = info ? info->selected_profile_id : 0;
            // "P/T1/T2" build notation is deliberately NOT computed here --
            // this endpoint is polled every ~300ms (health bars), and a
            // ga_character_skills read per player on every tick would mean
            // 2 synchronous SQLite queries x N players every 300ms, all on
            // the single ASIO thread this control server shares across
            // every listener (chat, matchmaking, TCP sessions -- not just
            // this HTTP server). See GET /overlay/builds, which is meant to
            // be polled far less often and reads through
            // SkillTreeCatalog::GetCachedBuildSummary instead of hitting
            // the DB on every call.
            players.push_back(p);
        }

        nlohmann::json body;
        body["instance_id"] = instance_id;
        body["players"]     = players;

        send_response(200, "OK", body.dump());
    }

    // GET /overlay/skilltree -- per-player list of invested skill_ids +
    // points, for the dedicated skill-tree grid page (skill-tree-overlay.html).
    // Shares the same instance roster as the main overlay (SpectatorOverlayState
    // is already gated on an active spectator -- see ActiveSpectatorCount),
    // just adds a live ga_character_skills read per player instead of the
    // effect/health snapshot fields.
    void handle_skilltree_request(const std::map<std::string, std::string>& params) {
        int64_t instance_id = 0;
        if (auto it = params.find("instance_id"); it != params.end()) {
            instance_id = std::strtoll(it->second.c_str(), nullptr, 10);
        }
        if (instance_id == 0) {
            send_response(400, "Bad Request", R"({"error":"missing instance_id"})");
            return;
        }

        const auto snaps = SpectatorOverlayState::GetForInstance(instance_id);
        nlohmann::json players = nlohmann::json::array();
        for (const auto& s : snaps) {
            auto info = PlayerSessionStore::GetByGuid(s.session_guid);
            if (!info) continue;

            nlohmann::json p;
            p["player_key"] = OpaquePlayerKey(s.session_guid);
            p["player_name"]  = info->player_name;
            p["task_force"]   = s.task_force;
            p["profile_id"]   = info->selected_profile_id;

            const int item_profile_id =
                PlayerSessionStore::GetCurrentItemProfile(info->selected_character_id);
            nlohmann::json points = nlohmann::json::array();
            for (const auto& row : PlayerSessionStore::GetSkillsForCharacter(
                     info->selected_character_id, item_profile_id)) {
                nlohmann::json r;
                r["skill_id"] = row.skill_id;
                r["points"]   = row.points;
                points.push_back(r);
            }
            p["points"] = points;
            players.push_back(p);
        }

        nlohmann::json body;
        body["instance_id"] = instance_id;
        body["players"]     = players;
        send_response(200, "OK", body.dump());
    }

    // GET /overlay/builds -- "P/T1/T2" build notation per player, meant to
    // be polled far less often than the main /overlay endpoint (health
    // bars poll every ~300ms; builds don't change mid-match outside a
    // loadout-profile swap -- see SkillTreeCatalog.hpp). Reads through
    // SkillTreeCatalog::GetCachedBuildSummary rather than hitting
    // ga_character_skills on every call.
    void handle_builds_request(const std::map<std::string, std::string>& params) {
        int64_t instance_id = 0;
        if (auto it = params.find("instance_id"); it != params.end()) {
            instance_id = std::strtoll(it->second.c_str(), nullptr, 10);
        }
        if (instance_id == 0) {
            send_response(400, "Bad Request", R"({"error":"missing instance_id"})");
            return;
        }

        const auto snaps = SpectatorOverlayState::GetForInstance(instance_id);
        nlohmann::json players = nlohmann::json::array();
        for (const auto& s : snaps) {
            auto info = PlayerSessionStore::GetByGuid(s.session_guid);
            if (!info) continue;

            const auto summary = SkillTreeCatalog::GetCachedBuildSummary(
                info->selected_character_id, info->selected_profile_id);

            nlohmann::json p;
            p["player_key"] = OpaquePlayerKey(s.session_guid);
            p["balanced"]      = summary.balanced;
            p["tree1"]         = summary.tree1;
            p["tree2"]         = summary.tree2;
            p["tree1_name"]    = summary.tree1_name;
            p["tree2_name"]    = summary.tree2_name;
            players.push_back(p);
        }

        nlohmann::json body;
        body["instance_id"] = instance_id;
        body["players"]     = players;
        send_response(200, "OK", body.dump());
    }

    // GET /overlay/instances -- lists instance ids currently worth offering
    // in the HTML's instance picker (see SpectatorOverlayState::
    // ListActiveInstances for why this is "actively spectated", not "every
    // running match" -- a hosted page the operator can't edit still needs
    // this filtered server-side, not just documented as a convention).
    void handle_instances_request() {
        nlohmann::json instances = nlohmann::json::array();
        for (int64_t id : SpectatorOverlayState::ListActiveInstances()) {
            nlohmann::json entry;
            entry["instance_id"] = id;
            auto info = InstanceRegistry::GetInstanceById(id);
            entry["map_name"]     = info ? info->map_name : "";
            entry["game_mode"]    = info ? info->game_mode : "";
            entry["player_count"] = (int)SpectatorOverlayState::GetForInstance(id).size();
            instances.push_back(entry);
        }

        nlohmann::json body;
        body["instances"] = instances;
        send_response(200, "OK", body.dump());
    }

    void send_response(int code, const char* status, const std::string& body) {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << code << " " << status << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n"
            << "\r\n"
            << body;
        response_ = oss.str();

        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(response_),
            [this, self](std::error_code, std::size_t) {
                deadline_.cancel();
                std::error_code ignored;
                socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
            });
    }

    asio::ip::tcp::socket socket_;
    std::string token_;
    asio::steady_timer deadline_;
    std::array<char, 4096> buf_{};
    std::string request_;
    std::string response_;
};

} // namespace

OverlayHttpServer::OverlayHttpServer(asio::io_context& io, const std::string& bind_address,
                                     uint16_t port, std::string token)
    : io_(io), acceptor_(io), token_(std::move(token)), retry_timer_(io) {
    if (port == 0) {
        Logger::Log("main", "[OverlayHttpServer] port=0 -- overlay HTTP endpoint disabled\n");
        return;
    }

    asio::error_code ec;
    const auto addr = asio::ip::make_address(bind_address, ec);
    if (ec) {
        Logger::Log("main", "[OverlayHttpServer] invalid overlay_bind '%s': %s\n",
            bind_address.c_str(), ec.message().c_str());
        return;
    }
    asio::ip::tcp::endpoint endpoint(addr, port);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        Logger::Log("main", "[OverlayHttpServer] Open failed on port %d: %s\n", port, ec.message().c_str());
        return;
    }
    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        Logger::Log("main", "[OverlayHttpServer] set_option(reuse_address) failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_.close();
        return;
    }
    acceptor_.bind(endpoint, ec);
    if (ec) {
        Logger::Log("main", "[OverlayHttpServer] Bind failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_.close();
        return;
    }
    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::Log("main", "[OverlayHttpServer] Listen failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_.close();
        return;
    }
    listening_ = true;
    Logger::Log("main", "[OverlayHttpServer] Listening on %s:%d (token %s)\n",
        bind_address.c_str(), port, token_.empty() ? "EMPTY -- open access" : "required");
    do_accept();
}

void OverlayHttpServer::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
        if (!listening_) return;
        if (!ec) {
            accept_errors_ = 0;
            std::make_shared<OverlayHttpSession>(std::move(sock), token_)->start();
            do_accept();
            return;
        }
        if (ec == asio::error::operation_aborted) return;
        // Under fd exhaustion async_accept fails immediately -- re-arming
        // right away would hot-spin the io thread shared with every other
        // listener. Back off, and give up entirely if the error persists:
        // the overlay is a side feature, matchmaking/chat are not.
        ++accept_errors_;
        if (accept_errors_ >= 10) {
            Logger::Log("main", "[OverlayHttpServer] %d consecutive accept failures (%s) -- stopping listener\n",
                accept_errors_, ec.message().c_str());
            listening_ = false;
            asio::error_code ignored;
            acceptor_.close(ignored);
            return;
        }
        Logger::Log("main", "[OverlayHttpServer] accept failed (%s), retry %d/10\n",
            ec.message().c_str(), accept_errors_);
        retry_timer_.expires_after(std::chrono::milliseconds(500 * accept_errors_));
        retry_timer_.async_wait([this](std::error_code tec) {
            if (!tec && listening_) do_accept();
        });
    });
}
