#include "src/ControlServer/PlayerSettingsApi/PlayerSettingsHttpServer.hpp"

#include <chrono>
#include <memory>
#include <sstream>
#include <string>

#include "lib/nlohmann/json.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"

namespace {

// Case-insensitive prefix/suffix helpers for the header block. Endpoint names
// here are fixed and few; a full HTTP parser would be more code than the
// problem deserves, same reasoning as the overlay server.
std::string ToLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string Trim(const std::string& s) {
    const size_t from = s.find_first_not_of(" \t\r\n");
    if (from == std::string::npos) return "";
    const size_t to = s.find_last_not_of(" \t\r\n");
    return s.substr(from, to - from + 1);
}

// Keys the website may write. Anything else is rejected rather than stored:
// this table is shared with the game DLL, so an arbitrary key here would be an
// arbitrary write into the game's configuration.
bool IsWritablePreference(const std::string& key) {
    return key == "show_broken_suits" || key == "show_all_suits" || key == "solo_mode";
}

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::string admin_token;
};

class PlayerSettingsHttpSession : public std::enable_shared_from_this<PlayerSettingsHttpSession> {
public:
    PlayerSettingsHttpSession(asio::ip::tcp::socket socket, std::string token)
        : socket_(std::move(socket)), token_(std::move(token)),
          deadline_(socket_.get_executor()) {}

    void start() {
        auto self(shared_from_this());
        deadline_.expires_after(std::chrono::seconds(10));
        deadline_.async_wait([this, self](std::error_code ec) {
            if (ec) return;  // cancelled: the request finished in time
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
                request_.append(buf_, len);

                // Buffer bound: a write endpoint should never let a client
                // park memory on the shared io thread.
                if (request_.size() > 16384) {
                    send_response(413, "Payload Too Large", R"({"error":"request too large"})");
                    return;
                }

                if (!headers_done_ && request_.find("\r\n\r\n") != std::string::npos) {
                    headers_done_ = true;
                    content_length_ = ParseContentLength();
                    if (content_length_ > 8192) {
                        send_response(413, "Payload Too Large", R"({"error":"body too large"})");
                        return;
                    }
                }

                if (headers_done_) {
                    const size_t header_end = request_.find("\r\n\r\n") + 4;
                    if (request_.size() >= header_end + content_length_) {
                        handle_request();
                        return;
                    }
                }
                do_read();
            });
    }

    size_t ParseContentLength() const {
        const size_t header_end = request_.find("\r\n\r\n");
        std::istringstream stream(request_.substr(0, header_end));
        std::string line;
        while (std::getline(stream, line)) {
            const size_t colon = line.find(':');
            if (colon == std::string::npos) continue;
            if (ToLower(Trim(line.substr(0, colon))) == "content-length") {
                return static_cast<size_t>(std::strtoull(line.c_str() + colon + 1, nullptr, 10));
            }
        }
        return 0;
    }

    void handle_request() {
        Request req;
        {
            std::istringstream stream(request_);
            stream >> req.method >> req.path;
            std::string line;
            std::getline(stream, line);  // rest of the request line
            while (std::getline(stream, line)) {
                if (line == "\r" || line.empty()) break;
                const size_t colon = line.find(':');
                if (colon == std::string::npos) continue;
                const std::string name = ToLower(Trim(line.substr(0, colon)));
                const std::string value = Trim(line.substr(colon + 1));
                if (name == "x-admin-token") req.admin_token = value;
            }
        }
        const size_t header_end = request_.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            req.body = request_.substr(header_end + 4, content_length_);
        }

        if (req.method != "POST") {
            send_response(405, "Method Not Allowed", R"({"error":"POST only"})");
            return;
        }

        // Constant-time-ish comparison is overkill for a header token on a
        // loopback socket, but the length check happens first so a wrong-length
        // guess cannot be distinguished by timing alone at any useful speed.
        if (token_.empty() || req.admin_token.size() != token_.size() ||
            !std::equal(req.admin_token.begin(), req.admin_token.end(), token_.begin())) {
            Logger::Log("settings-api", "[PlayerSettings] rejected request: bad or missing token\n");
            send_response(403, "Forbidden", R"({"error":"invalid token"})");
            return;
        }

        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (const std::exception&) {
            send_response(400, "Bad Request", R"({"error":"body must be JSON"})");
            return;
        }

        const int64_t user_id = body.value("user_id", static_cast<int64_t>(0));
        if (user_id <= 0) {
            send_response(400, "Bad Request", R"({"error":"missing user_id"})");
            return;
        }

        if (req.path == "/player/preferences") {
            handle_preference(user_id, body);
            return;
        }
        if (req.path == "/player/dlc") {
            handle_dlc(user_id, body);
            return;
        }
        send_response(404, "Not Found", R"({"error":"unknown path"})");
    }

    // A preference is two things: what the account keeps (the database), and
    // what this session is currently doing (the DLL's in-memory cache). The
    // chat command does both, so this does too.
    void handle_preference(int64_t user_id, const nlohmann::json& body) {
        const std::string key = body.value("key", std::string());
        const std::string value = body.value("value", std::string());

        if (!IsWritablePreference(key)) {
            send_response(400, "Bad Request", R"({"error":"unknown preference key"})");
            return;
        }
        if (value != "0" && value != "1") {
            send_response(400, "Bad Request", R"({"error":"value must be '0' or '1'"})");
            return;
        }

        if (!Database::SetUserPreference(user_id, key, value)) {
            send_response(500, "Internal Server Error", R"({"error":"could not persist"})");
            return;
        }

        const bool enabled = value == "1";
        bool applied_live = false;
        if (const auto session = PlayerSessionStore::GetByUserId(user_id)) {
            if (key == "solo_mode") {
                // Solo mode is entirely a control-server decision: it stamps
                // the ticket, so flipping the queued player is enough.
                MatchmakingService::SetSoloLockForQueuedPlayer(session->session_guid, enabled);
                applied_live = true;
            } else {
                ChatCommand::ToggleBrokenSuitsArgs args;
                args.mode = enabled ? 1 : 0;
                args.all = key == "show_all_suits";
                ChatCommand::DispatchToggleBrokenSuits(args, session->session_guid);
                // The dispatch is fire-and-forget over TCP; report it as
                // attempted rather than confirmed, and say which.
                applied_live = true;
            }
        }

        Logger::Log("settings-api",
            "[PlayerSettings] user=%lld key=%s value=%s persisted=1 live=%d\n",
            (long long)user_id, key.c_str(), value.c_str(), applied_live ? 1 : 0);

        nlohmann::json out;
        out["ok"] = true;
        out["key"] = key;
        out["value"] = value;
        out["applied_live"] = applied_live;
        send_response(200, "OK", out.dump());
    }

    void handle_dlc(int64_t user_id, const nlohmann::json& body) {
        const std::string identifier = body.value("identifier", std::string());
        const bool installed = body.value("installed", false);

        if (identifier.empty()) {
            send_response(400, "Bad Request", R"({"error":"missing identifier"})");
            return;
        }

        if (!Database::SetUserDlc(user_id, identifier, installed)) {
            send_response(404, "Not Found", R"({"error":"unknown dlc identifier"})");
            return;
        }

        // DLC is read at login (and by the launcher), so there is nothing live
        // to push: the flag takes effect the next time the pack is needed.
        Logger::Log("settings-api",
            "[PlayerSettings] user=%lld dlc=%s installed=%d\n",
            (long long)user_id, identifier.c_str(), installed ? 1 : 0);

        nlohmann::json out;
        out["ok"] = true;
        out["identifier"] = identifier;
        out["installed"] = installed;
        out["applied_live"] = false;
        send_response(200, "OK", out.dump());
    }

    void send_response(int status, const char* reason, const std::string& body) {
        auto self(shared_from_this());
        response_ = "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n";
        response_ += "Content-Type: application/json\r\n";
        response_ += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response_ += "Connection: close\r\n\r\n";
        response_ += body;

        deadline_.cancel();
        asio::async_write(socket_, asio::buffer(response_),
            [this, self](std::error_code, std::size_t) {
                std::error_code ignored;
                socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
                socket_.close(ignored);
            });
    }

    asio::ip::tcp::socket socket_;
    std::string token_;
    asio::steady_timer deadline_;
    std::string request_;
    std::string response_;
    char buf_[4096];
    bool headers_done_ = false;
    size_t content_length_ = 0;
};

} // namespace

PlayerSettingsHttpServer::PlayerSettingsHttpServer(asio::io_context& io,
                                                   const std::string& bind_address,
                                                   uint16_t port, std::string token)
    : io_(io), acceptor_(io), token_(std::move(token)), retry_timer_(io) {
    if (port == 0) {
        Logger::Log("main", "[PlayerSettings] settings_api_port=0 -- player settings endpoint disabled\n");
        return;
    }
    if (token_.empty()) {
        // Fail closed. Everything else in this process treats an empty token as
        // "no auth", which is survivable for a read-only overlay and not for an
        // endpoint that can change another account's settings.
        Logger::Log("main",
            "[PlayerSettings] admin_token is empty -- refusing to start the player settings endpoint\n");
        return;
    }

    asio::error_code ec;
    const auto addr = asio::ip::make_address(bind_address, ec);
    if (ec) {
        Logger::Log("main", "[PlayerSettings] invalid settings_api_bind '%s': %s\n",
            bind_address.c_str(), ec.message().c_str());
        return;
    }
    asio::ip::tcp::endpoint endpoint(addr, port);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        Logger::Log("main", "[PlayerSettings] Open failed on port %d: %s\n", port, ec.message().c_str());
        return;
    }
    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        Logger::Log("main", "[PlayerSettings] set_option(reuse_address) failed: %s\n", ec.message().c_str());
        acceptor_.close();
        return;
    }
    acceptor_.bind(endpoint, ec);
    if (ec) {
        Logger::Log("main", "[PlayerSettings] Bind failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_.close();
        return;
    }
    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::Log("main", "[PlayerSettings] Listen failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_.close();
        return;
    }
    listening_ = true;
    Logger::Log("main", "[PlayerSettings] Listening on %s:%d (token required)\n",
        bind_address.c_str(), port);
    do_accept();
}

void PlayerSettingsHttpServer::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
        if (!listening_) return;
        if (!ec) {
            accept_errors_ = 0;
            std::make_shared<PlayerSettingsHttpSession>(std::move(sock), token_)->start();
            do_accept();
            return;
        }
        if (ec == asio::error::operation_aborted) return;
        ++accept_errors_;
        if (accept_errors_ >= 10) {
            Logger::Log("main", "[PlayerSettings] %d consecutive accept failures (%s) -- stopping listener\n",
                accept_errors_, ec.message().c_str());
            listening_ = false;
            std::error_code ignored;
            acceptor_.close(ignored);
            return;
        }
        retry_timer_.expires_after(std::chrono::milliseconds(500 * accept_errors_));
        retry_timer_.async_wait([this](std::error_code tec) {
            if (!tec && listening_) do_accept();
        });
    });
}
