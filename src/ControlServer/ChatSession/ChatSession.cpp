#include "src/ControlServer/ChatSession/ChatSession.hpp"

// All chat sessions — safe without mutex since io_context runs single-threaded.
static std::vector<std::weak_ptr<ChatSession>> g_chat_sessions;

ChatSession::ChatSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket)), read_buf_(4096) {
    socket_.set_option(asio::ip::tcp::no_delay(true));
}

void ChatSession::start() {
    g_chat_sessions.push_back(shared_from_this());
    Logger::Log("chat", "[Chat] Client connected, sessions=%zu\n", g_chat_sessions.size());
    do_read();
}

void ChatSession::deliver(const std::vector<uint8_t>& msg) {
    bool idle = write_queue_.empty();
    write_queue_.push_back(msg);
    if (idle) do_write();
}

void ChatSession::do_write() {
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(write_queue_.front()),
        [this, self](std::error_code ec, std::size_t) {
            if (!ec) {
                write_queue_.pop_front();
                if (!write_queue_.empty()) do_write();
            }
        });
}

void ChatSession::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(read_buf_),
        [this, self](std::error_code ec, std::size_t len) {
            if (!ec) {
                handle_packet(read_buf_.data(), len);
                do_read();
            } else {
                // Purge expired entries on disconnect
                g_chat_sessions.erase(
                    std::remove_if(g_chat_sessions.begin(), g_chat_sessions.end(),
                        [](const std::weak_ptr<ChatSession>& w) { return w.expired(); }),
                    g_chat_sessions.end());
                Logger::Log("chat", "[Chat] Client disconnected, sessions=%zu\n", g_chat_sessions.size());
            }
        });
}

void ChatSession::handle_packet(const uint8_t* data, size_t length) {
    // Wire format: [2B length][2B packet_type=0x0073][2B item_count][TLV fields...]
    if (length < 6) return;

    uint16_t packet_type = data[2] | (data[3] << 8);
    if (packet_type != GA_U::CHAT_MESSAGE) {
        Logger::Log("chat", "[Chat] Unknown packet type 0x%04X\n", packet_type);
        return;
    }

    // Distinguish initial handshake (first field = SESSION_GUID 0x0473)
    // from a real chat message (first field = CHAT_CH_TYPE 0x00BE).
    if (length >= 8) {
        uint16_t first_field = data[6] | (data[7] << 8);
        if (first_field == GA_T::SESSION_GUID) {
            // Extract 16-byte GUID and look up the player
            if (length >= 8 + 16) {
                std::ostringstream oss;
                for (int i = 0; i < 16; i++)
                    oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[8 + i];
                std::string guid_hex = oss.str();
                auto info = PlayerSessionStore::GetByGuid(guid_hex);
                if (info) {
                    player_name_ = info->player_name;
                    Logger::Log("chat", "[Chat] Handshake from '%s' (guid=%s)\n",
                        player_name_.c_str(), guid_hex.c_str());
                } else {
                    Logger::Log("chat", "[Chat] Handshake received, guid=%s not found in registry\n", guid_hex.c_str());
                }
            } else {
                Logger::Log("chat", "[Chat] Handshake received (guid too short)\n");
            }
            return;
        }
    }

    Logger::Log("chat", "[Chat] Message received, broadcasting to %zu peer(s)\n",
        g_chat_sessions.size() > 0 ? g_chat_sessions.size() - 1 : 0);
    broadcast(data, length);
}

void ChatSession::broadcast(const uint8_t* data, size_t length) {
    std::vector<uint8_t> response;

    PacketView pkt(data + 6, length - 6);

    uint32_t channel = pkt.Read4B(GA_T::CHAT_CH_TYPE).value_or(4);

    std::string message = player_name_ + ": " + pkt.ReadString(GA_T::MESSAGE).value_or("");

    bool isDM = channel == 8;

    if (isDM) {
        message = "DMs are not supported yet, sorry!";
    }

    Logger::Log("chat", "Broadcasting message '%s' to channel %u\n", message.c_str(), channel);

    uint16_t packet_type = GA_U::CHAT_MESSAGE;
    uint16_t item_count = 2;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    Write4B(response, GA_T::CHAT_CH_TYPE, channel);
    WriteString(response, GA_T::MESSAGE, message);

    std::vector<uint8_t> chunk;
    uint16_t chunk_size = static_cast<uint16_t>(response.size());

    chunk.push_back(chunk_size & 0xFF);
    chunk.push_back(chunk_size >> 8);

    chunk.insert(chunk.end(), response.begin(), response.end());

    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (isDM) {
                if (peer.get() == this) {
                    peer->deliver(chunk);
                }
            } else {
                peer->deliver(chunk);
            }
        }
    }
}
