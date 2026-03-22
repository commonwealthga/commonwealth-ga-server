#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/Logger.hpp"
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
        : socket_(std::move(socket)), write_in_progress_(false) {}

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
                    return;
                }

                dispatch(std::string(body_buf_.begin(), body_buf_.end()));
                do_read_header();
            }
        );
    }

    // ── Dispatch ────────────────────────────────────────────────────────────

    void dispatch(const std::string& json_msg) {
        auto j = nlohmann::json::parse(json_msg, nullptr, false);
        if (j.is_discarded()) {
            Logger::Log("ipc", "[IpcServer] Malformed JSON from game instance\n");
            return;
        }

        std::string type = j.value("type", "");

        if (type == IpcProtocol::MSG_PING) {
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
        // Phase 10: GAME_EVENT, PLAYER_SPAWNED handlers go here
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

    // IpcServer::send() calls IpcSession::send() -- grant access via friend.
    friend class IpcServer;
};

// ---------------------------------------------------------------------------
// IpcServer static members
// ---------------------------------------------------------------------------

std::mutex IpcServer::ack_mutex_;
std::map<std::string, std::function<void(bool, int)>> IpcServer::pending_acks_;

// Tracks the currently connected game instance. Phase 9 will expand to per-instance routing.
static std::weak_ptr<IpcSession> g_active_ipc_session;

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
            g_active_ipc_session = session;
            session->start();
        }
        do_accept();
    });
}

bool IpcServer::SendToInstance(const std::string& json_msg) {
    auto session = g_active_ipc_session.lock();
    if (!session) {
        Logger::Log("ipc", "[IpcServer] SendToInstance: no active IPC session\n");
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
