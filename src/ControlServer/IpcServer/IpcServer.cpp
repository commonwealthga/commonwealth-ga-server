#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Shared/IpcFraming.hpp"
#include "lib/nlohmann/json.hpp"
#include <memory>
#include <array>
#include <vector>
#include <deque>
#include <string>

// ---------------------------------------------------------------------------
// IpcSession -- handles a single game instance connection (server side)
// ---------------------------------------------------------------------------

class IpcSession : public std::enable_shared_from_this<IpcSession> {
public:
    explicit IpcSession(asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), write_in_progress_(false),
          instance_id_(0), validated_(false) {}

    void start() {
        do_read_header();
    }

private:
    // ── Read pipeline ───────────────────────────────────────────────────────

    void do_read_header() {
        auto self = shared_from_this();
        asio::async_read(
            socket_,
            asio::buffer(header_buf_, 4),
            asio::transfer_exactly(4),
            [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    Logger::Log("ipc", "[IpcServer] Read header error: %s\n",
                        ec.message().c_str());
                    on_disconnect();
                    return;
                }

                uint32_t len =
                      (static_cast<uint32_t>(header_buf_[0])      )
                    | (static_cast<uint32_t>(header_buf_[1]) <<  8)
                    | (static_cast<uint32_t>(header_buf_[2]) << 16)
                    | (static_cast<uint32_t>(header_buf_[3]) << 24);

                do_read_body(len);
            }
        );
    }

    void do_read_body(uint32_t len) {
        body_buf_.resize(len);
        auto self = shared_from_this();
        asio::async_read(
            socket_,
            asio::buffer(body_buf_),
            asio::transfer_exactly(len),
            [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    Logger::Log("ipc", "[IpcServer] Read body error: %s\n",
                        ec.message().c_str());
                    on_disconnect();
                    return;
                }

                dispatch(std::string(body_buf_.begin(), body_buf_.end()));
                do_read_header();
            }
        );
    }

    // ── Disconnect cleanup ──────────────────────────────────────────────────

    void on_disconnect() {
        if (instance_id_ != 0) {
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                g_sessions.erase(instance_id_);
            }
            InstanceRegistry::MarkStopped(instance_id_);
            Logger::Log("ipc", "[IpcServer] Instance %lld disconnected, marked STOPPED\n",
                (long long)instance_id_);
        }
    }

    // ── Dispatch ────────────────────────────────────────────────────────────

    void dispatch(const std::string& json_msg) {
        auto j = nlohmann::json::parse(json_msg, nullptr, false);
        if (j.is_discarded()) {
            Logger::Log("ipc", "[IpcServer] Malformed JSON from game instance\n");
            return;
        }

        std::string type = j.value("type", "");

        if (type == IpcProtocol::MSG_INSTANCE_HELLO) {
            int64_t inst_id = j.value("instance_id", (int64_t)0);
            Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO: instance_id=%lld\n",
                (long long)inst_id);

            // Reject if instance_id is 0 (unassigned).
            if (inst_id == 0) {
                Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO rejected: instance_id=0\n");
                socket_.close();
                return;
            }

            instance_id_ = inst_id;
            validated_ = true;

            // Register in g_sessions.
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                g_sessions[inst_id] = shared_from_this();
            }

            // Send ACK.
            nlohmann::json ack;
            ack["type"]     = IpcProtocol::MSG_INSTANCE_HELLO_ACK;
            ack["accepted"] = true;
            send(ack.dump());
            Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO_ACK sent to instance_id=%lld\n",
                (long long)inst_id);
        }
        else if (type == IpcProtocol::MSG_INSTANCE_READY) {
            int64_t inst_id    = j.value("instance_id", (int64_t)0);
            int max_players    = j.value("max_players", 0);
            Logger::Log("ipc", "[IpcServer] INSTANCE_READY: instance_id=%lld max_players=%d\n",
                (long long)inst_id, max_players);
            InstanceRegistry::MarkReady(inst_id, max_players);
        }
        else if (type == IpcProtocol::MSG_PING) {
            Logger::Log("ipc", "[IpcServer] PING received, sending PONG\n");
            nlohmann::json pong;
            pong["type"] = IpcProtocol::MSG_PONG;
            send(pong.dump());
        }
        else if (type == IpcProtocol::MSG_PLAYER_REGISTER_ACK) {
            std::string guid = j.value("session_guid", "");
            bool success     = j.value("success", false);
            int pawn_id      = j.value("pawn_id", 0);
            Logger::Log("ipc", "[IpcServer] PLAYER_REGISTER_ACK: guid=%s success=%d pawn_id=%d\n",
                guid.c_str(), (int)success, pawn_id);
            IpcServer::DeliverAck(guid, success, pawn_id);
        }
        else if (type == IpcProtocol::MSG_GAME_EVENT) {
            std::string subtype = j.value("subtype", "");
            std::string guid    = j.value("session_guid", "");
            Logger::Log("ipc", "[IpcServer] GAME_EVENT subtype=%s guid=%s\n",
                subtype.c_str(), guid.c_str());
            if (guid.empty()) {
                Logger::Log("ipc", "[IpcServer] GAME_EVENT: empty guid -- dropped\n");
                return;
            }
            TcpSession::DeliverGameEvent(guid, j);
        }
        else if (type == IpcProtocol::MSG_RANDOMSM_RESULT) {
            std::string guid = j.value("session_guid", "");
            Logger::Log("ipc", "[IpcServer] RANDOMSM_RESULT for guid=%s\n", guid.c_str());
            if (guid.empty()) return;

            std::vector<std::string> names;
            if (j.contains("names") && j["names"].is_array()) {
                for (auto& n : j["names"]) {
                    if (n.is_string()) names.push_back(n.get<std::string>());
                }
            }

            TcpSession::DeliverRandomSMResult(guid, std::move(names));
        }
        else {
            Logger::Log("ipc", "[IpcServer] Unknown message type: %s\n", type.c_str());
        }
    }

    // ── Write pipeline (write_in_progress chain) ─────────────────────────────

    void send(const std::string& json_msg) {
        // Encode and enqueue. Kick write chain if not already running.
        write_queue_.push_back(IpcFraming::Encode(json_msg));
        if (!write_in_progress_) {
            write_in_progress_ = true;
            do_write();
        }
    }

    void do_write() {
        if (write_queue_.empty()) {
            write_in_progress_ = false;
            return;
        }

        // Keep a shared reference to the frame buffer alive across the async op.
        auto frame = std::make_shared<std::string>(std::move(write_queue_.front()));
        write_queue_.pop_front();

        auto self = shared_from_this();
        asio::async_write(
            socket_,
            asio::buffer(*frame),
            [this, self, frame](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    Logger::Log("ipc", "[IpcServer] Write error: %s\n",
                        ec.message().c_str());
                    write_in_progress_ = false;
                    return;
                }
                // Chain next write.
                do_write();
            }
        );
    }

    // ── Members ─────────────────────────────────────────────────────────────

    asio::ip::tcp::socket    socket_;
    uint8_t                  header_buf_[4];
    std::vector<uint8_t>     body_buf_;
    std::deque<std::string>  write_queue_;
    bool                     write_in_progress_;
    int64_t                  instance_id_;   // 0 until INSTANCE_HELLO validated
    bool                     validated_;     // true after INSTANCE_HELLO accepted

    // IpcServer::SendToInstance calls IpcSession::send() -- grant access via friend.
    friend class IpcServer;

    // g_sessions and sessions_mutex_ accessed from dispatch() and on_disconnect().
    static std::mutex sessions_mutex_;
    static std::map<int64_t, std::weak_ptr<IpcSession>> g_sessions;
};

// ---------------------------------------------------------------------------
// IpcSession static members (defined here, in IpcServer.cpp TU)
// ---------------------------------------------------------------------------

std::mutex IpcSession::sessions_mutex_;
std::map<int64_t, std::weak_ptr<IpcSession>> IpcSession::g_sessions;

// ---------------------------------------------------------------------------
// IpcServer static members
// ---------------------------------------------------------------------------

std::mutex IpcServer::ack_mutex_;
std::map<std::string, std::function<void(bool, int)>> IpcServer::pending_acks_;

// ---------------------------------------------------------------------------
// IpcServer -- accepts game instance connections on the IPC port
// ---------------------------------------------------------------------------

IpcServer::IpcServer(asio::io_context& io, uint16_t port)
    : io_(io), acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    Logger::Log("ipc", "[IpcServer] Listening on port %d\n", port);
    do_accept();
}

void IpcServer::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
        if (!ec) {
            Logger::Log("ipc", "[IpcServer] Game instance connected from %s\n",
                sock.remote_endpoint().address().to_string().c_str());
            auto session = std::make_shared<IpcSession>(std::move(sock));
            // Sessions are not tracked in g_sessions until INSTANCE_HELLO validates them.
            session->start();
        }
        do_accept();
    });
}

bool IpcServer::SendToInstance(int64_t instance_id, const std::string& json_msg) {
    std::lock_guard<std::mutex> lock(IpcSession::sessions_mutex_);
    auto it = IpcSession::g_sessions.find(instance_id);
    if (it == IpcSession::g_sessions.end()) {
        Logger::Log("ipc", "[IpcServer] SendToInstance: no session for instance_id=%lld\n",
            (long long)instance_id);
        return false;
    }
    auto session = it->second.lock();
    if (!session) {
        IpcSession::g_sessions.erase(it);
        Logger::Log("ipc", "[IpcServer] SendToInstance: expired session for instance_id=%lld\n",
            (long long)instance_id);
        return false;
    }
    session->send(json_msg);
    return true;
}

void IpcServer::RegisterPendingAck(const std::string& session_guid,
                                   std::function<void(bool, int)> callback) {
    std::lock_guard<std::mutex> lock(ack_mutex_);
    pending_acks_[session_guid] = std::move(callback);
}

void IpcServer::ClearPendingAck(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(ack_mutex_);
    pending_acks_.erase(session_guid);
}

void IpcServer::DeliverAck(const std::string& session_guid, bool success, int pawn_id) {
    std::function<void(bool, int)> cb;
    {
        std::lock_guard<std::mutex> lock(ack_mutex_);
        auto it = pending_acks_.find(session_guid);
        if (it == pending_acks_.end()) {
            Logger::Log("ipc", "[IpcServer] DeliverAck: no pending ACK for guid %s (already timed out?)\n",
                session_guid.c_str());
            return;
        }
        cb = std::move(it->second);
        pending_acks_.erase(it);
    }
    cb(success, pawn_id);
}
