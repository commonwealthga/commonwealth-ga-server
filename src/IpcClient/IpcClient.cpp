#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcFraming.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Shared/HexUtils.hpp"

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

CRITICAL_SECTION        IpcClient::outbound_cs_;
std::deque<std::string> IpcClient::outbound_queue_;

CRITICAL_SECTION        IpcClient::inbound_cs_;
std::deque<std::string> IpcClient::inbound_queue_;

std::string             IpcClient::host_;
uint16_t                IpcClient::port_         = 0;
int64_t                 IpcClient::instance_id_  = 0;

asio::io_context*       IpcClient::io_ctx_       = nullptr;
bool                    IpcClient::write_in_progress_ = false;
asio::ip::tcp::socket*  IpcClient::socket_       = nullptr;

std::array<uint8_t, 4> IpcClient::header_buf_;

// ---------------------------------------------------------------------------
// RAII guard for CRITICAL_SECTION
// ---------------------------------------------------------------------------
struct IpcCsGuard {
    CRITICAL_SECTION& cs;
    IpcCsGuard(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~IpcCsGuard() { LeaveCriticalSection(&cs); }
};

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void IpcClient::Init(const std::string& host, uint16_t port, int64_t instance_id) {
    host_ = host;
    port_ = port;
    instance_id_ = instance_id;

    InitializeCriticalSection(&outbound_cs_);
    InitializeCriticalSection(&inbound_cs_);

    Logger::Log("ipc", "[IPC] Init: connecting to %s:%d\n", host_.c_str(), (int)port_);

    CreateThread(NULL, 0, IpcThread, NULL, 0, NULL);
}

// ---------------------------------------------------------------------------
// IpcThread -- reconnect loop with exponential backoff
// ---------------------------------------------------------------------------

DWORD WINAPI IpcClient::IpcThread(LPVOID) {
    int backoff_secs = 2;

    while (true) {
        asio::io_context io;
        asio::ip::tcp::socket sock(io);

        io_ctx_ = &io;
        socket_ = &sock;
        write_in_progress_ = false;

        // Resolve and connect synchronously.
        asio::error_code ec;
        asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host_, std::to_string(port_), ec);

        if (ec) {
            Logger::Log("ipc", "[IPC] Resolve failed: %s\n", ec.message().c_str());
        } else {
            asio::connect(sock, endpoints, ec);
            if (ec) {
                Logger::Log("ipc", "[IPC] Connect failed: %s\n", ec.message().c_str());
            } else {
                // Connected successfully -- reset backoff.
                backoff_secs = 2;
                Logger::Log("ipc", "[IPC] Connected to %s:%d\n", host_.c_str(), (int)port_);

                // Send initial PING.
                nlohmann::json ping_msg;
                ping_msg["type"] = IpcProtocol::MSG_PING;
                Send(ping_msg.dump());

                // Send INSTANCE_HELLO to identify this game instance.
                nlohmann::json hello;
                hello["type"]        = IpcProtocol::MSG_INSTANCE_HELLO;
                hello["instance_id"] = instance_id_;
                Send(hello.dump());
                Logger::Log("ipc", "[IPC] Sent INSTANCE_HELLO: instance_id=%lld\n", instance_id_);

                // Arm the first async read.
                do_read_header();

                // Run the ASIO event loop -- blocks until socket error or close.
                io.run();
            }
        }

        // Disconnected or failed to connect.
        io_ctx_ = nullptr;
        socket_ = nullptr;
        write_in_progress_ = false;

        Logger::Log("ipc", "[IPC] Disconnected. Reconnecting in %ds...\n", backoff_secs);
        Sleep(backoff_secs * 1000);
        backoff_secs = backoff_secs < 30 ? backoff_secs * 2 : 30;

        io.restart();
    }

    return 0;
}

// ---------------------------------------------------------------------------
// do_read_header -- async read of 4-byte LE length prefix
// ---------------------------------------------------------------------------

void IpcClient::do_read_header() {
    asio::async_read(
        *socket_,
        asio::buffer(header_buf_),
        asio::transfer_exactly(4),
        [](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Read header error: %s\n", ec.message().c_str());
                return; // io.run() will exit, triggering reconnect.
            }

            // Decode LE uint32 length.
            uint32_t len =
                  (static_cast<uint32_t>(header_buf_[0])      )
                | (static_cast<uint32_t>(header_buf_[1]) <<  8)
                | (static_cast<uint32_t>(header_buf_[2]) << 16)
                | (static_cast<uint32_t>(header_buf_[3]) << 24);

            do_read_body(len);
        }
    );
}

// ---------------------------------------------------------------------------
// do_read_body -- async read of payload
// ---------------------------------------------------------------------------

void IpcClient::do_read_body(uint32_t len) {
    auto body_buf = std::make_shared<std::vector<uint8_t>>(len);

    asio::async_read(
        *socket_,
        asio::buffer(*body_buf),
        asio::transfer_exactly(len),
        [body_buf](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Read body error: %s\n", ec.message().c_str());
                return; // io.run() will exit, triggering reconnect.
            }

            // Push into inbound queue for game-thread drain.
            std::string msg(reinterpret_cast<const char*>(body_buf->data()), body_buf->size());
            {
                IpcCsGuard lock(inbound_cs_);
                inbound_queue_.push_back(std::move(msg));
            }

            // Chain next read.
            do_read_header();
        }
    );
}

// ---------------------------------------------------------------------------
// Send -- game-thread enqueue + write kick
// ---------------------------------------------------------------------------

void IpcClient::Send(const std::string& json_msg) {
    {
        IpcCsGuard lock(outbound_cs_);
        outbound_queue_.push_back(json_msg);
    }

    // Post kick_write onto the ASIO thread if the io_context is live.
    // NOTE: io_ctx_ may be set to nullptr by the ASIO thread on disconnect.
    // This is a benign race -- if we miss the post, the next reconnect
    // will drain the queue via kick_write after send.
    asio::io_context* ctx = io_ctx_;
    if (ctx != nullptr) {
        asio::post(*ctx, kick_write);
    }
}

// ---------------------------------------------------------------------------
// kick_write -- posted to ASIO thread by Send()
// ---------------------------------------------------------------------------

void IpcClient::kick_write() {
    if (!write_in_progress_) {
        write_in_progress_ = true;
        do_write();
    }
}

// ---------------------------------------------------------------------------
// do_write -- ASIO thread only, write_in_progress chain pattern
// ---------------------------------------------------------------------------

void IpcClient::do_write() {
    std::string msg;
    {
        IpcCsGuard lock(outbound_cs_);
        if (outbound_queue_.empty()) {
            write_in_progress_ = false;
            return;
        }
        msg = outbound_queue_.front();
        outbound_queue_.pop_front();
    }

    std::string frame = IpcFraming::Encode(msg);
    auto buf = std::make_shared<std::string>(std::move(frame));

    asio::async_write(
        *socket_,
        asio::buffer(*buf),
        [buf](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Write error: %s\n", ec.message().c_str());
                write_in_progress_ = false;
                return; // io.run() will exit, triggering reconnect.
            }

            // Chain next write.
            do_write();
        }
    );
}

// ---------------------------------------------------------------------------
// DrainInbound -- game-thread only, swap-drain pattern
// ---------------------------------------------------------------------------

void IpcClient::DrainInbound() {
    std::deque<std::string> local;
    {
        IpcCsGuard lock(inbound_cs_);
        local.swap(inbound_queue_);
    }

    for (const std::string& raw : local) {
        auto j = nlohmann::json::parse(raw, nullptr, false);
        if (j.is_discarded()) {
            Logger::Log("ipc", "[IPC] Malformed JSON: %s\n", raw.c_str());
            continue;
        }

        std::string type = j.value("type", "");

        if (type == IpcProtocol::MSG_PONG) {
            Logger::Log("ipc", "[IPC] PONG received\n");
        } else if (type == IpcProtocol::MSG_INSTANCE_HELLO_ACK) {
            bool accepted = j.value("accepted", false);
            Logger::Log("ipc", "[IPC] INSTANCE_HELLO_ACK: accepted=%d\n", (int)accepted);
        } else if (type == IpcProtocol::MSG_PLAYER_REGISTER) {
            std::string guid        = j.value("session_guid", "");
            std::string pname       = j.value("player_name", "");
            int64_t uid             = j.value("user_id", (int64_t)0);
            int64_t char_id         = j.value("character_id", (int64_t)0);
            uint32_t profile_id     = j.value("profile_id", (uint32_t)0);
            // head_asm_id, gender_type_value_id, morph_data available in payload but not stored
            // in PlayerInfo at registration time -- SpawnPlayerCharacter reads morph_data from
            // the DLL's own sqlite DB (inserted at character creation via old flow).

            PlayerInfo info;
            info.session_guid          = guid;
            info.player_name           = pname;
            info.ip_address            = "";  // Not known yet; set by JOIN when UDP connects
            info.user_id               = uid;
            info.selected_character_id = char_id;
            info.selected_profile_id   = profile_id;

            // Convert player_name to wide string for UE3 APIs.
            info.player_name_w = std::wstring(pname.begin(), pname.end());

            PlayerRegistry::Register(info);

            Logger::Log("ipc", "[IPC] PLAYER_REGISTER: guid=%s profile=%u char=%lld user=%lld\n",
                guid.c_str(), profile_id, char_id, uid);

            // Send ACK back to control server.
            // pawn_id is 0 at registration time (pawn not yet spawned -- spawns at JOIN).
            nlohmann::json ack;
            ack["type"]         = IpcProtocol::MSG_PLAYER_REGISTER_ACK;
            ack["session_guid"] = guid;
            ack["success"]      = true;
            ack["pawn_id"]      = 0;
            IpcClient::Send(ack.dump());
        } else {
            Logger::Log("ipc", "[IPC] Unknown message type: %s\n", type.c_str());
        }
    }
}
